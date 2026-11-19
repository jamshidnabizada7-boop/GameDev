"""Create the sandbox mixer and route its original sound assets through it."""
import unreal as ue

directory = "/Game/AfghanFPS/Audio/Foundation"
assets = ue.AssetToolsHelpers.get_asset_tools()
def get_or_create(name, cls, factory):
    asset = ue.load_asset(f"{directory}/{name}")
    if asset is None:
        asset = assets.create_asset(name, directory, cls, factory())
    assert asset and isinstance(asset, cls), name
    assert ue.EditorAssetLibrary.save_loaded_asset(asset, False), name
    return asset

effects = get_or_create("SC_Effects", ue.SoundClass, ue.SoundClassFactory)
get_or_create("SC_Music", ue.SoundClass, ue.SoundClassFactory)
get_or_create("Mix_Foundation", ue.SoundMix, ue.SoundMixFactory)
for name in ("Dry", "Explosion", "FoundationUse", "Reload", "Shot", "Whistle"):
    sound = ue.load_asset(f"{directory}/{name}")
    assert isinstance(sound, ue.SoundBase), name
    sound.set_editor_property("sound_class_object", effects)
    assert ue.EditorAssetLibrary.save_loaded_asset(sound, False), name
print("Foundation mixer routes six sound assets; reserved music bus has no track assigned.")
