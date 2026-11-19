# TacticalCQBSystem — Ready-to-Run Unreal Engine 5 C++ Plugin

Complete UE 5.4–5.8 C++ gameplay plugin engineered directly from the 25-second tactical CQB reference video, ready to drop into `AfghanistanFPS` (`C:\GameDev\AfghanistanFPS\Plugins\TacticalCQBSystem`) or any Unreal Engine 5 project.

## Included Systems (100% Video Parity)
1. **Tactical Movement & Full-Body Awareness (`UTacticalMovementComponent`, `ATacticalCQBCharacter`)**: Sprint, Crouch, Combat Slide, Max Payne Supine Dive-to-Back Slide-Shoot, Waist-High Parkour Vault/Mantle, Corner Lean, Automatic Off-Hand Wall-Brace IK, and visible lower-body legs/boots.
2. **CQB Takedown & Human Shield (`UCQBTakedownComponent`)**: Grab enemy as living Human Shield (`[F]`), fire one-handed canted around shield, plant live glowing frag grenade onto shield (`[G]`), Spartan Front Kick down stairs (`[E]`/`[V]`), overhead grenade throw, and close-quarters neck-snap execution.
3. **Procedural Weapon Handling (`UTacticalWeaponComponent`, `UTacticalWeaponDataAsset`)**: 4-slot weapon strip (`MP7`/`Cruiser`, `1911`, `Karambit`, `Dual Wield`), procedural ADS/hip/canted transitions, recoil kickback, on-the-fly suppressor attach/detach (`[B]`), tactical reload (`[R]`), world weapon pickup (`[F]`), and physical spinning shell casing ejection (`ATacticalShellCasing`).
4. **Dynamic Door Breach & Bullet-Time (`ATacticalBreachDoor`, `UTacticalTimeDilationComponent`)**: Hand-bash or kick-breach doors with automatic smooth global time dilation, focus energy pool, and recovery.
5. **Gore, Ballistic Impacts & Active Ragdoll (`UBallisticImpactComponent`, `ATacticalEnemyCharacter`)**: Niagara blood bursts, exit-wound raycast blood decals on walls/floors behind targets, surface bullet holes, bone-specific Chaos ragdoll knockback, and enemy world weapon drop on death.

## 5-Minute Ready-to-Run Installation
1. Extract the `TacticalCQBSystem` folder into `<YourUE5Project>/Plugins/TacticalCQBSystem/`.
2. Right-click your `.uproject` file -> **Generate Visual Studio project files** (or open directly in UE5 Editor and click **Yes** when prompted to compile `TacticalCQBSystem`).
3. Inside UE5 Editor, enable **Python Editor Script Plugin** (if not already enabled) and run:
   `Tools -> Execute Python Script -> Plugins/TacticalCQBSystem/Content/Python/SetupTacticalCQBAssets.py`
4. Create a Blueprint child of `ATacticalCQBCharacter` (`BP_TacticalCQBCharacter`), assign the generated `IMC_TacticalCQB`, `IA_*` actions, and `DA_Weapon_*` Data Assets, and set it as your GameMode's Default Pawn Class.
5. Drop `ATacticalEnemyCharacter`, `ATacticalBreachDoor`, and `ATacticalWeaponPickup` actors into a greybox corridor and press **Play (ALT+P)**!
