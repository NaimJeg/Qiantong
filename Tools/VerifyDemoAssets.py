"""Editor-only asset audit; does not write assets."""
import json
import os
import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
results = []
for name in ['BP_UnitView', 'BP_DemoDirector', 'BP_DemoHUD', 'BP_DemoController', 'BP_DemoGameMode']:
    path = '/Game/Demo/Blueprints/' + name
    bp = unreal.load_asset(path)
    assert bp, 'Missing blueprint ' + path
    assert unreal.BlueprintEditorLibrary.compile_blueprint(bp), 'Compile failed: ' + path
    data = registry.get_asset_by_object_path(path + '.' + name)
    data_only = data.get_tag_value('IsDataOnly')
    assert str(data_only).lower() == 'true', 'Blueprint contains logic: ' + path + ': ' + str(data_only)
    results.append({'asset': path, 'compiled': True, 'dataOnly': True})
mode = unreal.get_default_object(unreal.load_class(None, '/Game/Demo/Blueprints/BP_DemoGameMode.BP_DemoGameMode_C'))
assert mode.get_editor_property('hud_class').get_name() == 'BP_DemoHUD_C'
assert mode.get_editor_property('player_controller_class').get_name() == 'BP_DemoController_C'
host = unreal.get_default_object(unreal.load_class(None, '/Game/Demo/Blueprints/BP_DemoDirector.BP_DemoDirector_C'))
for key in ['unit_view_class', 'stage_material', 'ally_material', 'enemy_material']:
    assert host.get_editor_property(key), 'Missing reference ' + key
path = os.path.join(unreal.Paths.project_saved_dir(), 'Logs', 'Demo-AssetAudit.json')
with open(path, 'w', encoding='utf-8') as output:
    json.dump(results, output, indent=2)
unreal.log('QIANTONG_ASSET_AUDIT_PASS: 5 data-only blueprints compile and reference native classes/assets')
