"""Run in Unreal Editor with -ExecutePythonScript=...; creates only /Game/Demo assets.

All Blueprints are data-only subclasses. No event graphs or runtime Python logic.
Existing demo assets are refused to protect manual edits; use the saved assets thereafter.
"""
import unreal

ROOT = '/Game/Demo'
assets = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary


def material(name, color):
    mat = assets.create_asset(name, ROOT + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        raise RuntimeError('Cannot create ' + name)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    expr = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    expr.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(expr, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    library.save_loaded_asset(mat)
    return mat


def blueprint(name, parent, defaults=None):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    bp = assets.create_asset(name, ROOT + '/Blueprints', unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError('Cannot create ' + name)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cls = bp.generated_class()
    cdo = unreal.get_default_object(cls)
    for key, value in (defaults or {}).items():
        cdo.set_editor_property(key, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    library.save_loaded_asset(bp)
    return cls


def main():
    if library.does_asset_exist(ROOT + '/Maps/L_Demo'):
        raise RuntimeError('L_Demo already exists; refusing to replace edited assets.')
    stage = material('M_Shaft', (0.007, 0.014, 0.024))
    frame = material('M_Frame', (0.024, 0.065, 0.078))
    ally = material('M_Ally', (0.06, 0.70, 0.47))
    enemy = material('M_Enemy', (0.88, 0.075, 0.035))
    unit = blueprint('BP_UnitView', unreal.load_class(None, '/Script/QiantongUE.DemoUnitView'))
    director = blueprint('BP_DemoDirector', unreal.load_class(None, '/Script/QiantongUE.DemoDirector'), {
        'unit_view_class': unit, 'stage_material': stage, 'ally_material': ally, 'enemy_material': enemy})
    hud = blueprint('BP_DemoHUD', unreal.load_class(None, '/Script/QiantongUE.DemoHUD'))
    controller = blueprint('BP_DemoController', unreal.load_class(None, '/Script/QiantongUE.DemoController'))
    mode = blueprint('BP_DemoGameMode', unreal.load_class(None, '/Script/QiantongUE.DemoGameMode'), {
        'hud_class': hud, 'player_controller_class': controller})
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    world.get_world_settings().set_editor_property('default_game_mode', mode)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    host = actor_subsystem.spawn_actor_from_class(director, unreal.Vector(0, 0, 0))
    host.set_actor_label('BP_DemoDirector - native simulation and camera')
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')

    def block(label, x, y, w, h, mat):
        actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(640-y, x-360, 5))
        actor.set_actor_label(label)
        comp = actor.static_mesh_component
        comp.set_static_mesh(cube)
        comp.set_material(0, mat)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_scale3d(unreal.Vector(h/100, w/100, 0.08))
        return actor

    block('Shaft / left rail', 162, 565, 8, 730, frame)
    block('Shaft / right rail', 698, 565, 8, 730, frame)
    block('Mothership / hull', 427, 187, 465, 24, frame)
    block('Mothership / bay light', 427, 204, 138, 4, ally)
    for i in range(8):
        block('Shaft / crossbar %02d' % i, 430, 248+i*84, 526, 2, frame)
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, ROOT + '/Maps/L_Demo'):
        raise RuntimeError('Map save failed')
    unreal.log('QIANTONG_DEMO_ASSETS_CREATED: 5 data-only Blueprints, 4 materials, L_Demo')


main()
