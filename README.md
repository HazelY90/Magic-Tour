# Magic Tour

**Third-Person Flight and Combat Prototype · Unreal Engine 5 · C++ · Blueprint**

Magic Tour is a small gameplay prototype built around a short sequence of free flight, low-altitude forest traversal, landing, and projectile-based magic combat. The project is intended as a focused portfolio piece that demonstrates Unreal Engine gameplay programming in C++ with Blueprint-based presentation and tuning.

## Current Status

The Unreal project is created and the initial third-person and arcade-flight foundations are playable in the editor.

Completed work:

- Created the initial forest map with a meadow, traversal path, and ruins area.
- Added a playable third-person character with basic ground movement and camera controls.
- Added walking/flight switching, vertical flight controls, boost, and configurable flight movement parameters.
- Added a Blueprint-adjustable placeholder broom that is visible only during flight.

The current playable scope includes third-person ground movement and basic free flight. Landing validation, checkpoints, combat, enemies, HUD, victory, and restart systems are still planned.

There is no packaged build or gameplay recording yet.

## Controls

| Action | Keyboard and mouse | Controller |
| --- | --- | --- |
| Move | `W`, `A`, `S`, `D` or arrow keys | Left stick |
| Look | Mouse | Right stick |
| Jump | `Space` | Bottom face button |
| Toggle flight | `F` | Top face button |
| Ascend | `Space` | Right shoulder |
| Descend | `Left Ctrl` | Left shoulder |
| Boost | `Left Shift` | Right trigger |

Landing validation and spell controls will be added with their gameplay systems.

## Planned Gameplay

**Free flight → Forest traversal → Route objectives → Ruins encounter → Victory**

- Fly freely above the forest and descend into the low-altitude route.
- Navigate a curved corridor between trees by flying or walking.
- Land only on valid meadow and path surfaces, then take off again without losing progress.
- Complete 3–5 ordered route checkpoints.
- Enter the ruins encounter after satisfying the route objectives.
- Use one projectile spell to defeat 2–3 enemies.
- Support health, damage, victory, failure, and restart states.

## Architecture

Gameplay rules belong in C++. Blueprint assets assign models, animation, effects, audio, UI, input assets, and tuning values.

| Class or component | Status | Responsibility |
| --- | --- | --- |
| `MagicTourCharacter` | Implemented foundation | Ground movement, flight input coordination, and placeholder broom presentation; later coordinates landing and casting. |
| `MagicTourPlayerController` | Implemented foundation | Adds input mapping contexts and manages the camera manager and optional touch controls. |
| `MagicTourCameraManager` | Implemented foundation | Applies the current camera pitch limits. |
| `MagicTourGameMode` | Implemented foundation | Current project GameMode entry point; later manages objectives and win or failure states. |
| `FlightComponent` | Implemented foundation | Walking/flight transitions, vertical movement, speed, acceleration, braking, and boost. |
| `HealthComponent` | Planned | Shared health, damage processing, and death events. |
| `SpellProjectile` | Planned | Projectile movement, collision, damage, and lifetime. |
| `EnemyCharacter` / `EnemyAIController` | Planned | Enemy detection, pursuit, attack, and death behavior. |
| `Checkpoint` / `LandingZone` | Planned | Ordered route progression and validated entry into the final encounter. |
| HUD Widget | Planned | Health, objectives, crosshair, controls, and result display. |

Movement state and level progress will remain separate. Landing or taking off during traversal must not reset checkpoint progress.

## Project Structure

```text
MagicTour/
├── Config/                         # Engine, map, input, and project settings
├── Content/
│   ├── Characters/                 # Shared mannequin meshes, materials, rigs, and animation
│   ├── Input/                      # Enhanced Input actions and mapping contexts
│   ├── LevelPrototyping/           # Shared blockout assets retained from the UE template
│   ├── MagicTour/
│   │   └── Environment/            # Project-specific forest, ruins, materials, and blockout meshes
│   └── ThirdPerson/
│       ├── Blueprints/             # Character, PlayerController, and GameMode Blueprints
│       └── Maps/                   # Lvl_MagicTour
├── Source/
│   ├── MagicTour.Target.cs
│   ├── MagicTourEditor.Target.cs
│   └── MagicTour/
│       ├── Characters/             # Player and later enemy character classes
│       ├── Framework/              # GameMode, PlayerController, and CameraManager
│       ├── MagicTour.Build.cs
│       ├── MagicTour.cpp
│       └── MagicTour.h
├── docs/                            # Development plan and internal project notes
└── MagicTour.uproject
```

Future C++ systems will use the planned `Components/`, `Combat/`, `AI/`, and `World/` categories described in `docs/project-structure.md` as those systems are implemented.

## Development Environment

| Item | Validated setup |
| --- | --- |
| Unreal Engine | UE 5.8 project association; current development installation is UE 5.8.2. |
| Target platform | macOS on Apple Silicon. |
| C++ toolchain | Xcode toolchain, successfully used to build `MagicTourEditor`. |
| Input system | Enhanced Input. |
| Rendering | Lumen and hardware ray tracing are enabled in the current project configuration. |

## Running the Project

1. Install a compatible Unreal Engine 5.8 release and Xcode toolchain.
2. Open `MagicTour.uproject` from the repository root.
3. Allow Unreal Editor to compile project modules if prompted.
4. Open `/Game/ThirdPerson/Maps/Lvl_MagicTour` if it is not already loaded.
5. Select **Play** in the editor.

The map and project GameMode are already configured as defaults, so normal editor startup should open the correct level and use the third-person character automatically.

## Current Map

`Lvl_MagicTour` is a gameplay blockout rather than final environment art. It currently contains:

- A small meadow used as the starting area.
- A large forest with collision on tree trunks and no collision on tree crowns.
- A curved path sized for walking and later low-altitude flight testing.
- Open air above the forest for the future free-flight phase.
- A ruins clearing surrounded by forest for the future combat encounter.

The current surface tags express the intended landing rules, but the future `FlightComponent` must validate the hit surface, slope, and available character space before landing.

## Scope and Limitations

The MVP is limited to one map, one playable character, one projectile spell, one enemy type, 2–3 enemy instances, 3–5 route checkpoints, and a minimal HUD. Development will prioritize a complete playable loop before final environment art and visual polish.

Multiplayer, open-world systems, inventory, equipment, skill trees, dialogue, saves, complex aerodynamic simulation, and multiple spells are outside the initial scope.

## Asset Credits

The current project uses Unreal Engine template mannequin, animation, input, and level-prototyping assets together with project-created blockout meshes and materials. Sources, licenses, and required attribution for any later third-party environment, character, animation, audio, or VFX assets will be recorded here when those assets are introduced.
