"""Build the simple casting action in Unreal Editor Python. Ensure these assets have no unsaved changes; restart the main editor afterward."""
import json
import math
import traceback
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools

FOLDER = "/Game/MagicTour/Characters/Animations"
REPORT = "/private/tmp/MagicTourCastBuild.json"


def norm(v):
    length = math.sqrt(sum(x*x for x in v))
    return tuple(x / length for x in v)


def quat(q):
    return (q.x, q.y, q.z, q.w)


def mul(a, b):
    x,y,z,w = a
    X,Y,Z,W = b
    return (w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X,
            w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z)


def inv(q):
    return (-q[0],-q[1],-q[2],q[3])


def between(a, b):
    a,b=norm(a),norm(b)
    cross=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    return norm(cross+(1+sum(x*y for x,y in zip(a,b)),))


def blend(a, b, t):
    # Normalized shortest-path interpolation is sufficient for the short arm motion.
    if sum(x*y for x,y in zip(a,b)) < 0:
        b=tuple(-x for x in b)
    return unreal.Quat(*norm(tuple(x*(1-t)+y*t for x,y in zip(a,b))))


def pos(t):
    return (t.translation.x,t.translation.y,t.translation.z)


def sub(a,b):
    return tuple(x-y for x,y in zip(a,b))


def weight(time):
    if time < 0.25:
        t=time/0.25
    elif time < 0.38:
        return 1.0
    else:
        t=max(0.0,(0.7-time)/0.32)
    return t*t*(3-2*t)


def pin(node, name, is_output=False):
    info=BlueprintTools.get_node_infos([node])[0]
    pins=info.output_pins if is_output else info.input_pins
    return next(p.pin_id for p in pins if str(p.name)==name)


def build():
    idle=unreal.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")
    skeleton=idle.get_editor_property("skeleton")
    pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(idle,0.0,unreal.AnimPoseEvaluationOptions())
    bones=list(unreal.AnimPoseExtensions.get_bone_names(pose))
    local={str(b):unreal.AnimPoseExtensions.get_bone_pose(pose,b,unreal.AnimPoseSpaces.LOCAL) for b in bones}
    world={str(b):unreal.AnimPoseExtensions.get_bone_pose(pose,b,unreal.AnimPoseSpaces.WORLD) for b in bones}
    upper=world['upperarm_r']; lower=world['lowerarm_r']; hand=world['hand_r']
    # Mannequin mesh forward is +Y; align the arm forward with a small elbow bend.
    upper_q=mul(between(sub(pos(lower),pos(upper)),(0,1,-0.12)),quat(upper.rotation))
    lower_q=mul(between(sub(pos(hand),pos(lower)),(0,1,0.12)),quat(lower.rotation))
    targets={
        'upperarm_r':mul(inv(quat(world['clavicle_r'].rotation)),upper_q),
        'lowerarm_r':mul(inv(upper_q),lower_q)
    }
    for bone in bones:
        name=str(bone)
        if name.endswith('_r') and any(name.startswith(s) for s in ('index_','middle_','ring_','pinky_','thumb_')):
            targets[name]=quat(unreal.AnimPoseExtensions.get_ref_bone_pose(pose,bone,unreal.AnimPoseSpaces.LOCAL).rotation)
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    sequence=unreal.load_asset(FOLDER+'/A_Cast')
    if not sequence:
        factory=unreal.AnimSequenceFactory()
        factory.set_editor_property('target_skeleton',skeleton)
        sequence=tools.create_asset('A_Cast',FOLDER,unreal.AnimSequence,factory)
    controller=sequence.get_editor_property('controller')
    controller.open_bracket('Build simple raised-hand cast',False)
    controller.set_frame_rate(unreal.FrameRate(30,1),False)
    controller.set_number_of_frames(unreal.FrameNumber(21),False)
    controller.remove_all_bone_tracks(False)
    for bone in bones:
        name=str(bone); base=local[name]
        controller.add_bone_track(bone,False)
        rotations=[blend(quat(base.rotation),targets.get(name,quat(base.rotation)),weight(frame/30.0)) for frame in range(22)]
        controller.set_bone_track_keys(bone,[base.translation]*22,rotations,[base.scale3d]*22,False)
    controller.close_bracket(False)
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)

    montage=unreal.load_asset(FOLDER+'/AM_Cast')
    if not montage:
        factory=unreal.AnimMontageFactory()
        factory.set_editor_property('target_skeleton',skeleton)
        factory.set_editor_property('source_animation',sequence)
        montage=tools.create_asset('AM_Cast',FOLDER,unreal.AnimMontage,factory)
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(montage)
    unreal.AnimationLibrary.add_animation_notify_track(montage,'Cast')
    notify_class=unreal.load_class(None,'/Script/MagicTour.CastNotify')
    assert notify_class, 'Native cast notify must be compiled first'
    unreal.AnimationLibrary.add_animation_notify_event(montage,'Cast',0.27,notify_class)
    for prop in ('blend_in','blend_out'):
        settings=montage.get_editor_property(prop)
        settings.set_editor_property('blend_time',0.1)
        montage.set_editor_property(prop,settings)
    unreal.EditorAssetLibrary.save_loaded_asset(montage)

    bp=unreal.load_asset(FOLDER+'/ABP_MagicTour')
    graph=BlueprintTools.get_graph(bp,'AnimGraph')
    nodes=list(BlueprintTools.find_nodes(graph,'',None,False))
    slot=next(n for n in nodes if n.get_name()=='AnimGraphNode_Slot_0')
    source=next(n for n in nodes if n.get_name()=='AnimGraphNode_StateMachine_1')
    rig=next(n for n in nodes if n.get_name()=='AnimGraphNode_ControlRig_0')
    layers=[n for n in nodes if isinstance(n,unreal.AnimGraphNode_LayeredBoneBlend)]
    layer=layers[0] if layers else BlueprintTools.create_node(graph,'Animation|Blends|Layeredblendperbone',unreal.IntPoint(-420,350))
    settings=layer.get_editor_property('node')
    branch=unreal.BranchFilter()
    branch.set_editor_property('bone_name','spine_03')
    branch.set_editor_property('blend_depth',3)
    mask=unreal.InputBlendPose()
    mask.set_editor_property('branch_filters',[branch])
    settings.set_editor_property('layer_setup',[mask])
    settings.set_editor_property('mesh_space_rotation_blend',True)
    layer.set_editor_property('node',settings)
    BlueprintTools.connect_pins(pin(source,'Pose',True),pin(layer,'BasePose'))
    BlueprintTools.connect_pins(pin(slot,'Pose',True),pin(layer,'BlendPoses_0'))
    BlueprintTools.connect_pins(pin(layer,'Pose',True),pin(rig,'Source'))
    BlueprintTools.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    return {'status':'success','sequence':sequence.get_path_name(),'montage':montage.get_path_name(),
            'duration':0.7,'release_time':0.27,'tracks':len(bones),'slot':'DefaultSlot','blend_bone':'spine_03'}


try:
    result=build()
except Exception:
    result={'status':'error','traceback':traceback.format_exc()}
with open(REPORT,'w') as report:
    json.dump(result,report,indent=2)
unreal.log('CAST_BUILD_RESULT: '+json.dumps(result))
