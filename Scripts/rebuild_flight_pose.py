"""Snapshot the authored body pose, then rebuild it with neutral distal bones in Unreal Editor."""
import hashlib
import json
import math
from pathlib import Path
import traceback
import unreal

ASSET = '/Game/MagicTour/Characters/Animations/A_Flight_Fixed'
MESH = '/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple'
FOLDER = Path(unreal.Paths.project_saved_dir()).resolve() / 'FlightRebuild'
BODY = {'root', 'pelvis', 'head'}
BODY.update(f'{name}_{side}' for name in ('clavicle', 'upperarm', 'lowerarm', 'thigh', 'calf')
            for side in ('l', 'r'))
BODY.update(f'spine_{index:02d}' for index in range(1, 6))
BODY.update(f'neck_{index:02d}' for index in range(1, 3))


def pack(transform):
    return {'pos': [transform.translation.x, transform.translation.y, transform.translation.z],
            'quat': [transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w],
            'rot': [transform.rotation.rotator().pitch, transform.rotation.rotator().yaw, transform.rotation.rotator().roll],
            'scale': [transform.scale3d.x, transform.scale3d.y, transform.scale3d.z]}


def pose(sequence, time, mesh=None, kind=unreal.AnimDataEvalType.RAW):
    options = unreal.AnimPoseEvaluationOptions()
    options.set_editor_property('evaluation_type', kind)
    options.set_editor_property('should_retarget', False)
    options.set_editor_property('extract_root_motion', False)
    if mesh:
        options.set_editor_property('optional_skeletal_mesh', mesh)
    value = unreal.AnimPoseExtensions.get_anim_pose_at_time(sequence, time, options)
    assert unreal.AnimPoseExtensions.is_valid(value), 'Pose evaluation failed'
    return value


def read(value, is_ref=False):
    getter = unreal.AnimPoseExtensions.get_ref_bone_pose if is_ref else unreal.AnimPoseExtensions.get_bone_pose
    return {str(bone): pack(getter(value, bone, unreal.AnimPoseSpaces.LOCAL))
            for bone in unreal.AnimPoseExtensions.get_bone_names(value)}


def error(actual, expected):
    distance = math.sqrt(sum((a-b)**2 for a, b in zip(actual['pos'], expected['pos'])))
    scale = max(abs(a-b) for a, b in zip(actual['scale'], expected['scale']))
    qa, qb = actual['quat'], expected['quat']
    dot = abs(sum(a*b for a, b in zip(qa, qb))) / math.sqrt(sum(x*x for x in qa) * sum(x*x for x in qb))
    angle = math.degrees(2.0 * math.acos(min(1.0, dot)))
    return {'position_cm': distance, 'angle_deg': angle, 'scale': scale}


def snapshot(sequence):
    FOLDER.mkdir(parents=True, exist_ok=True)
    path = Path(unreal.Paths.project_content_dir()) / 'MagicTour/Characters/Animations/A_Flight_Fixed.uasset'
    mesh = unreal.load_asset(MESH)
    length = sequence.get_play_length()
    samples = {str(time): read(pose(sequence, time)) for time in (0.0, length / 2.0, length)}
    original = samples['0.0']
    defaults = read(pose(sequence, 0.0), True)
    # Prefer the actual player's reference proportions, with skeleton defaults for extra bones.
    defaults.update(read(pose(sequence, 0.0, mesh), True))
    body = {name: transform for name, transform in original.items() if name in BODY}
    assert {'root', 'pelvis', 'upperarm_l', 'upperarm_r', 'thigh_l', 'thigh_r'} <= body.keys()
    data = {'asset': ASSET, 'file_hash': hashlib.sha256(path.read_bytes()).hexdigest(),
            'duration': length, 'frames': sequence.get_editor_property('number_of_sampled_frames'),
            'body': body, 'original': original, 'defaults': defaults, 'samples': samples}
    with open(FOLDER / 'pose-before.json', 'w') as output:
        json.dump(data, output, indent=2)
    return {'status': 'snapshot_saved', 'body_bones': len(body), 'all_bones': len(original),
            'snapshot': str(FOLDER / 'pose-before.json'), 'body': body}


def rebuild(sequence):
    with open(FOLDER / 'pose-before.json') as source:
        saved = json.load(source)
    path = Path(unreal.Paths.project_content_dir()) / 'MagicTour/Characters/Animations/A_Flight_Fixed.uasset'
    assert hashlib.sha256(path.read_bytes()).hexdigest() == saved['file_hash'], 'The saved animation changed after the snapshot'
    target = {bone: saved['body'].get(bone, saved['defaults'][bone]) for bone in saved['original']}
    frames = max(1, saved['frames'])
    controller = sequence.get_editor_property('controller')
    # Bake the authored body once; leaving Set Key curves would apply the offsets a second time.
    controller.open_bracket('Rebuild fixed flight pose with neutral hands and feet', True)
    try:
        controller.remove_all_curves_of_type(unreal.RawCurveTrackTypes.RCT_TRANSFORM, True)
        controller.remove_all_bone_tracks(True)
        controller.set_number_of_frames(unreal.FrameNumber(frames), True)
        for bone, transform in target.items():
            controller.add_bone_track(bone, True)
            controller.set_bone_track_keys(bone,
                [unreal.Vector(*transform['pos'])] * (frames + 1),
                [unreal.Quat(*transform['quat'])] * (frames + 1),
                [unreal.Vector(*transform['scale'])] * (frames + 1), True)
    finally:
        controller.close_bracket(True)
    sequence.set_editor_property('additive_anim_type', unreal.AdditiveAnimationType.AAT_NONE)

    checks = []
    for time in (0.0, saved['duration'] / 2.0, saved['duration']):
        actual = read(pose(sequence, time))
        errors = {bone: error(actual[bone], wanted) for bone, wanted in target.items()}
        failed = {bone: value for bone, value in errors.items()
                  if value['position_cm'] > 0.01 or value['angle_deg'] > 0.1 or value['scale'] > 0.0001}
        checks.append({'time': time, 'failures': failed})
    assert all(not check['failures'] for check in checks), json.dumps(checks)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sequence), 'Animation save failed'
    return {'status': 'success', 'asset': ASSET, 'body_bones': len(saved['body']),
            'default_bones': len(target) - len(saved['body']), 'checks': checks,
            'snapshot': str(FOLDER / 'pose-before.json')}


try:
    sequence = unreal.load_asset(ASSET)
    assert sequence, 'Flight animation not found'
    result = snapshot(sequence) if globals().get('FLIGHT_PHASE', 'snapshot') == 'snapshot' else rebuild(sequence)
except Exception:
    result = {'status': 'error', 'traceback': traceback.format_exc()}
FOLDER.mkdir(parents=True, exist_ok=True)
with open(FOLDER / 'result.json', 'w') as output:
    json.dump(result, output, indent=2)
unreal.log('FLIGHT_REBUILD_RESULT: ' + json.dumps(result))
