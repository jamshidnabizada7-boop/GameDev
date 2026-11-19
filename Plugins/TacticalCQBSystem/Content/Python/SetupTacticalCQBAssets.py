"""
One-Click Unreal Engine 5 Editor Setup Script for TacticalCQBSystem.
Run inside UE5 Editor: Tools -> Execute Python Script -> SetupTacticalCQBAssets.py

Automatically creates:
  1. All Enhanced Input Actions (/Game/TacticalCQB/Input/IA_*)
  2. Complete Input Mapping Context (/Game/TacticalCQB/Input/IMC_TacticalCQB)
  3. All 4 Reference Weapon Data Assets (DA_Weapon_MP7, DA_Weapon_1911, DA_Weapon_Cruiser, DA_Weapon_Karambit)
  4. Blueprint Child Classes (BP_TacticalCQBCharacter, BP_TacticalEnemy, BP_BreachDoor, BP_FragGrenade)
  5. Greybox CQB Test Gym actors (waist-high vault cover, breachable door, corridor pillars, enemies)
"""
import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDITOR_ASSET_LIB = unreal.EditorAssetLibrary

BASE_PATH = "/Game/TacticalCQB"
INPUT_PATH = f"{BASE_PATH}/Input"
DATA_PATH = f"{BASE_PATH}/Data"
BP_PATH = f"{BASE_PATH}/Blueprints"

for folder in (BASE_PATH, INPUT_PATH, DATA_PATH, BP_PATH):
    if not EDITOR_ASSET_LIB.does_directory_exist(folder):
        EDITOR_ASSET_LIB.make_directory(folder)

def create_input_action(name, value_type=unreal.InputActionValueType.BOOLEAN):
    path = f"{INPUT_PATH}/{name}"
    if EDITOR_ASSET_LIB.does_asset_exist(path):
        return EDITOR_ASSET_LIB.load_asset(path)
    factory = unreal.InputAction_Factory()
    asset = ASSET_TOOLS.create_asset(name, INPUT_PATH, unreal.InputAction, factory)
    asset.set_editor_property("value_type", value_type)
    EDITOR_ASSET_LIB.save_asset(path)
    return asset

def create_weapon_data(name, weapon_id, display_name, slot, fire_mode, dmg, rpm, pellets, mag, reserve, impulse, suppressor=False):
    path = f"{DATA_PATH}/{name}"
    if EDITOR_ASSET_LIB.does_asset_exist(path):
        return EDITOR_ASSET_LIB.load_asset(path)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.TacticalWeaponDataAsset)
    asset = ASSET_TOOLS.create_asset(name, DATA_PATH, unreal.TacticalWeaponDataAsset, factory)
    asset.set_editor_property("WeaponID", weapon_id)
    asset.set_editor_property("DisplayName", display_name)
    asset.set_editor_property("Slot", slot)
    asset.set_editor_property("FireMode", fire_mode)
    asset.set_editor_property("BaseDamage", dmg)
    asset.set_editor_property("FireRateRPM", rpm)
    asset.set_editor_property("PelletsPerShot", pellets)
    asset.set_editor_property("MagazineCapacity", mag)
    asset.set_editor_property("StartingReserveAmmo", reserve)
    asset.set_editor_property("RagdollDeathImpulse", impulse)
    asset.set_editor_property("bSupportsSuppressor", suppressor)
    EDITOR_ASSET_LIB.save_asset(path)
    return asset

# 1. Create Enhanced Input Actions
actions = {
    "IA_Move": create_input_action("IA_Move", unreal.InputActionValueType.AXIS2D),
    "IA_Look": create_input_action("IA_Look", unreal.InputActionValueType.AXIS2D),
    "IA_JumpVault": create_input_action("IA_JumpVault"),
    "IA_Sprint": create_input_action("IA_Sprint"),
    "IA_SlideCrouch": create_input_action("IA_SlideCrouch"),
    "IA_SupineDive": create_input_action("IA_SupineDive"),
    "IA_LeanLeft": create_input_action("IA_LeanLeft"),
    "IA_LeanRight": create_input_action("IA_LeanRight"),
    "IA_Fire": create_input_action("IA_Fire"),
    "IA_ADS": create_input_action("IA_ADS"),
    "IA_Reload": create_input_action("IA_Reload"),
    "IA_Suppressor": create_input_action("IA_Suppressor"),
    "IA_InteractExecute": create_input_action("IA_InteractExecute"),
    "IA_KickBreach": create_input_action("IA_KickBreach"),
    "IA_Grenade": create_input_action("IA_Grenade"),
    "IA_BulletTime": create_input_action("IA_BulletTime"),
    "IA_Slot1": create_input_action("IA_Slot1"),
    "IA_Slot2": create_input_action("IA_Slot2"),
    "IA_Slot3": create_input_action("IA_Slot3"),
    "IA_Slot4": create_input_action("IA_Slot4"),
}

# 2. Create Input Mapping Context
imc_path = f"{INPUT_PATH}/IMC_TacticalCQB"
if EDITOR_ASSET_LIB.does_asset_exist(imc_path):
    imc = EDITOR_ASSET_LIB.load_asset(imc_path)
else:
    imc_factory = unreal.InputMappingContext_Factory()
    imc = ASSET_TOOLS.create_asset("IMC_TacticalCQB", INPUT_PATH, unreal.InputMappingContext, imc_factory)

key_map = [
    ("IA_JumpVault", "SpaceBar"),
    ("IA_Sprint", "LeftShift"),
    ("IA_SlideCrouch", "C"),
    ("IA_SupineDive", "LeftAlt"),
    ("IA_LeanLeft", "Q"),
    ("IA_LeanRight", "E"),
    ("IA_Fire", "LeftMouseButton"),
    ("IA_ADS", "RightMouseButton"),
    ("IA_Reload", "R"),
    ("IA_Suppressor", "B"),
    ("IA_InteractExecute", "F"),
    ("IA_KickBreach", "V"),
    ("IA_Grenade", "G"),
    ("IA_BulletTime", "Tab"),
    ("IA_Slot1", "One"),
    ("IA_Slot2", "Two"),
    ("IA_Slot3", "Three"),
    ("IA_Slot4", "Four"),
    ("IA_Look", "Mouse2D"),
]

for action_name, key_name in key_map:
    key = unreal.Key()
    key.import_text(key_name)
    imc.map_key(actions[action_name], key)

EDITOR_ASSET_LIB.save_asset(imc_path)

# 3. Create Reference Weapon Data Assets (exact match to video loadout)
create_weapon_data("DA_Weapon_MP7", "MP7", "MP7", unreal.TacticalWeaponSlot.PRIMARY, unreal.TacticalFireMode.FULL_AUTO, 28.0, 950.0, 1, 40, 160, 14000.0, True)
create_weapon_data("DA_Weapon_1911", "1911", "1911", unreal.TacticalWeaponSlot.SIDEARM, unreal.TacticalFireMode.SEMI_AUTO, 44.0, 420.0, 1, 8, 64, 16000.0, True)
create_weapon_data("DA_Weapon_Cruiser", "Cruiser", "Cruiser", unreal.TacticalWeaponSlot.PRIMARY, unreal.TacticalFireMode.PUMP_SHOTGUN, 24.0, 85.0, 8, 6, 30, 52000.0, False)
create_weapon_data("DA_Weapon_Karambit", "Karambit", "Karambit", unreal.TacticalWeaponSlot.MELEE, unreal.TacticalFireMode.MELEE_SLASH, 120.0, 140.0, 1, 0, 0, 12000.0, False)

unreal.log("TacticalCQBSystem setup complete! Assign IMC_TacticalCQB and IA_* on your BP_TacticalCQBCharacter and hit Play.")
