# Magic Tour

## Project Overview

Magic Tour is a third-person flight and magic combat prototype built with Unreal Engine 5. Players explore a forest on foot or by broom, follow a sequence of route objectives, and battle enemies in ancient ruins.

The project focuses on responsive movement, exploration, and a compact combat experience. Feature development is complete, and no additional features are planned. It is currently playable in Unreal Editor; standalone demo packaging and validation remain pending.

## Tech Stack

- Unreal Engine 5.8
- C++ and Blueprint
- Enhanced Input
- Animation Blueprints and Montages
- UMG
- NavMesh-based AI
- Lumen
- macOS on Apple Silicon with Xcode

## Main Features

- **Ground movement and free flight** — Walk, jump, fly by broom, and switch between ground and air movement.
- **Forest exploration** — Travel through a meadow, a winding forest path, and a ruins clearing.
- **Route objectives** — Complete three checkpoints in order with live task guidance.
- **Projectile magic** — Aim with a centered crosshair and cast while standing still; moving before release interrupts the spell.
- **Ruins encounters** — Three enemies patrol the ruins and attack when the player enters; they resume patrol when the player leaves.
- **Health and HUD** — Track player health, damage, and the current objective.
- **Death and victory** — Respawn at the starting point with full health after death; defeat every enemy to show a victory prompt, then confirm to mark all tasks complete.

## Gameplay Flow

**Meadow start → Forest entrance → Forest trail → Ruins → Defeat enemies → Victory**

Three ordered checkpoints update the objective as the player travels through the level. Movement and route progress are independent, so walking, flying, and switching between them all follow the same route rules.

Enemies engage the player inside the ruins encounter area. Respawning returns the player to the original spawn with full health while preserving checkpoint progress and enemy state. Defeating all three enemies opens the victory prompt; confirming it updates the objective to `All Tasks Complete`.

## Architecture

C++ owns gameplay rules, state, combat, AI, and HUD behavior. Blueprint assets configure the playable classes, input resources, models, animations, and tuning values. Unreal's Character Movement and navigation systems handle movement and pathfinding.

| Area | Main classes and components | Responsibility |
| --- | --- | --- |
| Game flow | `MagicTourGameMode`, `Checkpoint` | Enforce checkpoint order, publish objective changes, track enemy deaths, and complete the objective after victory confirmation. |
| Player session | `MagicTourPlayerController` | Set up input mappings, manage the persistent HUD and interface input, and respawn the player at the original spawn. |
| Characters | `MagicTourCharacter`, `EnemyCharacter` | Share movement, health, aiming, and casting; specialize player presentation and enemy targeting and death behavior. |
| Movement and health | `FlightComponent`, `HealthComponent` | Coordinate flight transitions, takeoff, and vertical input; store health and publish health-change and death events. |
| Encounter AI | `EnemyAIController`, `CombatArena` | Patrol, pursue, and perform stationary ranged attacks within a bounded navigation area. |
| Combat | `SpellProjectile`, `CastNotify` | Release the shared projectile from the casting animation and handle movement, collision, damage, and lifetime. |
| Presentation | `MagicTourHUDWidget`, `MagicTourCameraManager` | Display health, objectives, crosshair, and result prompts; apply camera pitch limits. |

The systems communicate through gameplay events:

- **Progress:** checkpoint overlaps reach GameMode, which validates route order and notifies the HUD of the new objective.
- **Combat:** player input or AI requests a cast. The animation releases a projectile, and a hit reaches the target's health component through Unreal's damage system.
- **Death and respawn:** health events stop character actions and notify the controller. The HUD requests respawn, and the controller replaces the player pawn and reconnects the health display.
- **Victory:** GameMode counts enemy deaths immediately, independently of their five-second removal delay. The controller handles the prompt and forwards confirmation to GameMode.

## Controls

| Action | Keyboard and mouse | Controller |
| --- | --- | --- |
| Move | `W`, `A`, `S`, `D` or arrow keys | Left stick |
| Look | Mouse | Right stick |
| Jump | `Space` | Bottom face button |
| Toggle flight | `F` | Top face button |
| Ascend | `Space` | Right shoulder |
| Descend | `Left Ctrl` | Left shoulder |
| Cast while standing still | Left mouse button | Right trigger |

## Project Structure

```text
MagicTour/
├── MagicTour.uproject
├── Config/                         # Engine, map, input, and project settings
├── Content/
│   ├── Characters/                 # Shared mannequin meshes and animation assets
│   ├── Input/                      # Enhanced Input actions and mapping contexts
│   ├── MagicTour/
│   │   ├── Characters/             # Enemy Blueprint and project animations
│   │   └── Environment/            # Forest, trail, and ruins blockout assets
│   └── ThirdPerson/                # Player, controller, GameMode Blueprints, and main map
├── Source/MagicTour/
│   ├── AI/                         # Enemy controller
│   ├── Characters/                 # Shared character and enemy behavior
│   ├── Combat/                     # Projectile and animation release notification
│   ├── Components/                 # Flight and health
│   ├── Framework/                  # GameMode, PlayerController, and CameraManager
│   ├── Tests/                      # Editor automation tests
│   ├── UI/                         # Gameplay HUD and result prompts
│   └── World/                      # Checkpoints and combat arena
├── Scripts/                        # Animation asset authoring utilities
└── docs/                           # Internal development notes
```

## Running the Project

The validated development environment is Unreal Engine 5.8 on macOS Apple Silicon with an Xcode C++ toolchain.

1. Open `MagicTour.uproject` in Unreal Editor and compile the project modules if prompted.
2. Open `/Game/ThirdPerson/Maps/Lvl_MagicTour` if it is not already loaded.
3. Select **Play** to start the demo.

The startup map, game map, and GameMode are configured in the project defaults. The player Blueprint uses Quinn with `ABP_MagicTour`, ground locomotion, a fixed flight pose, and a fixed lying death pose.

## Validation and Delivery Status

The latest recorded validation on October 14, 2026 passed the `MagicTourEditor` build and all 22 `MagicTour.*` automation tests. Tests cover flight, ordered checkpoints, health, casting and projectiles, enemy encounters, player death and respawn, and victory confirmation. Animation appearance is checked manually.

The demo uses one map, one playable character, one projectile spell, and three enemies. It retains the current blockout environment and existing resources. Feature development is complete; standalone macOS packaging and full-flow validation remain pending.
