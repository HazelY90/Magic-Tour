# Magic Tour

**Flight & Combat Prototype · Unreal Engine 5 · C++ · Blueprint**

Magic Tour is a personal gameplay project exploring arcade-style flight, forest traversal, and projectile-based magic combat. The planned experience follows one character from open-air flight to forest paths and a small combat encounter among ancient ruins.

The technical focus is C++ gameplay programming: movement transitions, reusable components, collision and damage, simple enemy AI, and objective state management.

## Status

**Initial setup — not yet playable.** Repository structure and development planning are in place, and Unreal Engine 5.8.2 is installed. The UE project and gameplay systems have not been created yet.

There is no gameplay demo or downloadable build yet. A 45–60 second gameplay recording is planned once the core experience is complete.

## Planned Gameplay

**Free flight → Forest traversal → Route objectives → Ruins encounter → Victory**

- Fly above the forest, descend between obstacles, and control speed with boost.
- Land on suitable forest ground, walk along roads, and take off again without losing progress.
- Complete 3–5 ordered checkpoints by flying or walking through them. These are route objectives, not save or respawn points.
- Unlock the final encounter after completing the route; ordinary forest landings remain available throughout traversal.
- Use one projectile spell to defeat 2–3 enemies, with basic health, damage, victory, failure, and restart states.

Input bindings will be documented after implementation and testing.

## Planned Technical Approach

| Area | Design intent |
| --- | --- |
| Movement | Keep flight and walking on one `ACharacter`, using `CharacterMovement` for displacement and collision. A flight component will manage direction, speed, and boost. |
| Health and damage | Share a health component between player and enemies, with events for health changes and death. Projectiles will use UE damage handling. |
| Progression | Separate movement state from level progress so landing and taking off do not reset objectives. Validate checkpoint order and prevent duplicate completion. |
| Enemy AI | Use a small set of detection, chase, attack, and death states with navigation in the combat arena. |
| Presentation | Implement gameplay rules in C++; use Blueprint for asset assignment, animation, VFX, audio, UI, and tuning parameters. |

Implementation details, source links, and verified behavior will be added as these systems become playable.

## Planned Architecture

| Class or component | Responsibility |
| --- | --- |
| `MagicTourGameMode` | Objective progress, encounter activation, and victory/failure conditions. |
| `PlayerController` | Input mappings, input mode, and HUD creation. |
| `PlayerCharacter` | Player actions, component coordination, casting, and movement transitions. |
| `FlightComponent` | Flight controls and configurable movement parameters. |
| `HealthComponent` | Health, damage processing, and death notifications. |
| `SpellProjectile` | Projectile movement, collision, damage, and lifetime. |
| `EnemyCharacter` / `EnemyAIController` | Enemy actions and behavior decisions. |
| `Checkpoint` / `LandingZone` | Route triggers and validated entry into the final encounter. |

Project code will live under `Source/MagicTour/`, project assets under `Content/MagicTour/`, and engine configuration under `Config/`.

## Development Environment and Running

The initial target is **macOS on Apple Silicon**.

| Item | Current status |
| --- | --- |
| Unreal Engine | 5.8.2 installed; selected for project development. |
| C++ toolchain | Xcode installed; compatibility with the selected UE version is not yet verified. |
| Code editor | VS Code available. |
| Build and runtime validation | Pending project creation. |

This repository cannot be built or launched yet: `MagicTour.uproject` and the C++ module files are not present.

### Intended Startup Workflow

After the Third Person C++ project has been created and successfully built:

1. Open the repository's `MagicTour.uproject` with Unreal Engine 5.8.2.
2. Allow the editor to finish loading assets and compiling shaders.
3. Open the project's gameplay map and select **Play** to run in the editor.

These steps have not been validated yet. The exact map, compatible Xcode version, and C++ build instructions will be recorded after the first successful build and editor run.

## Scope and Limitations

The MVP is intentionally limited to one map, one playable character, one spell, and one enemy type. Development will prioritize a complete playable loop before environment art and visual polish.

Multiplayer, open-world systems, inventory, skill trees, saves, and complex flight physics are outside the initial scope. No platform compatibility or performance results have been validated yet.

## Contributions and Asset Credits

The intended portfolio contribution is gameplay design, C++ systems, Blueprint integration, level assembly, and testing. Completed implementation work will be documented as development progresses.

Third-party models, textures, animations, and effects may be used. No art assets have been added yet; sources, licenses, and required attribution will be recorded here as assets are introduced.
