"""Create a fixed face-up player death pose without editing the shared Anim Blueprint."""
import json
import traceback
import unreal


def build():
    idle = unreal.load_asset('/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle')
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(
        idle, 0.0, unreal.AnimPoseEvaluationOptions())
    bones = list(unreal.AnimPoseExtensions.get_bone_names(pose))
    folder = '/Game/MagicTour/Characters/Animations'
    sequence = unreal.load_asset(folder + '/A_Death_Fixed')
    if not sequence:
        factory = unreal.AnimSequenceFactory()
        factory.set_editor_property('target_skeleton', idle.get_editor_property('skeleton'))
        sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'A_Death_Fixed', folder, unreal.AnimSequence, factory)
    controller = sequence.get_editor_property('controller')
    controller.open_bracket('Build fixed supine player pose', False)
    controller.set_frame_rate(unreal.FrameRate(30, 1), False)
    controller.set_number_of_frames(unreal.FrameNumber(1), False)
    controller.remove_all_bone_tracks(False)
    for bone in bones:
        transform = unreal.AnimPoseExtensions.get_bone_pose(
            pose, bone, unreal.AnimPoseSpaces.LOCAL)
        if str(bone) == 'root':
            # Mesh forward is +Y: rotate around X so the face points upward.
            transform.rotation = unreal.Rotator(pitch=0.0, yaw=0.0, roll=-90.0).quaternion()
            # Center the torso under the capsule and keep the back above the floor.
            transform.translation = unreal.Vector(0.0, 90.0, 18.0)
        controller.add_bone_track(bone, False)
        controller.set_bone_track_keys(bone, [transform.translation] * 2,
                                       [transform.rotation] * 2, [transform.scale3d] * 2, False)
    controller.close_bracket(False)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    check = unreal.AnimPoseExtensions.get_anim_pose_at_time(
        sequence, 0.0, unreal.AnimPoseEvaluationOptions())
    positions = {str(bone): str(unreal.AnimPoseExtensions.get_bone_pose(
        check, bone, unreal.AnimPoseSpaces.WORLD)) for bone in ('root', 'pelvis', 'head')}
    return {'status': 'success', 'asset': sequence.get_path_name(), 'tracks': len(bones),
            'positions': positions}


try:
    result = build()
except Exception:
    result = {'status': 'error', 'traceback': traceback.format_exc()}
with open('/private/tmp/MagicTourDeathBuild.json', 'w') as report:
    json.dump(result, report, indent=2)
unreal.log('DEATH_BUILD_RESULT: ' + json.dumps(result))
