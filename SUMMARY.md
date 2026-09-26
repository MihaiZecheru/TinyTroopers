# TinyTroopers — Software & Game Design Requirements Specification (SRS)

**Document Version:** 1.0.0  
**Target Systems:** Windows (MinGW-w64 / MSVC) / Linux POSIX  
**Standard:** IEEE 830-compliant Game Technical Requirements Specification

---

## Executive Summary

This document specifies the complete functional, technical, and operational requirements for **TinyTroopers**, a competitive 2D top-down multiplayer tactical arena shooter. The game is built using modern **C++20**, **Raylib** for hardware-accelerated rendering and audio, and a custom **non-blocking UDP client-server architecture** operating at low latency over local area networks or direct IP connections.

---

## 1. System Overview & Architectural Requirements

### 1.1 Architecture Model
- **REQ-ARCH-001 (Authoritative Server):** The game system SHALL operate on an authoritative client-server architecture. All physics simulations, weapon hit verifications, movement validation, projectile life cycles, collision resolutions, damage calculations, and game-state transitions MUST be executed strictly on the dedicated server.
- **REQ-ARCH-002 (Thin Client):** The client application SHALL function as an interactive visualization and input-sampling terminal. The client samples user input at fixed intervals, transmits discrete input packets to the server, and renders received world state snapshots without predicting unconfirmed authoritative game events.
- **REQ-ARCH-003 (Three-Tier Subsystem Separation):** The codebase SHALL be divided into three decoupled tiers:
  1. `common/`: Shared mathematical libraries (`Vec2`), network sockets (`UdpSocket`), global constants (`Constants.hpp`), and binary serialization protocol definitions (`Protocol.hpp`).
  2. `server/`: Dedicated server orchestration (`GameServer`), room life-cycle management, and authoritative game logic (`Room`).
  3. `client/`: Graphical client orchestration (`GameClient`), network client (`NetworkClient`), asset management (`AssetManager`), audio management (`AudioManager`), screen state machine (`ScreenManager`), and UI screens.

### 1.2 Technology Stack Requirements
- **REQ-TECH-001 (Language Standard):** The entire codebase SHALL be written in standard C++20 (`CMAKE_CXX_STANDARD 20`, extensions disabled).
- **REQ-TECH-002 (Graphics & Windowing):** The client SHALL utilize Raylib (version compatible with CMake config or official Raylib toolchain) interfacing with OpenGL.
- **REQ-TECH-003 (Networking API):** Network communication MUST utilize raw socket APIs via a platform-agnostic wrapper (`UdpSocket`): Winsock2 (`ws2_32`) on Windows and POSIX Sockets on UNIX-like platforms. No heavy third-party networking engines (e.g. RakNet, ENet, Steamworks) SHALL be required.
- **REQ-TECH-004 (Build Automation):** The project SHALL be configured and built via CMake (version 3.20 or newer). Building the client MUST automatically copy runtime assets to the output binary directory.

---

## 2. Functional Requirements: Weapons & Classes

The game MUST provide four distinct playable soldier classes, each bound to a specific primary firearm. Players SHALL have the ability to inspect and select their class during room configuration or while awaiting respawn.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             WEAPON SPECIFICATIONS                           │
├──────────────┬──────────┬──────────┬──────────────┬───────────┬─────────────┤
│ Class / Gun  │ Magazine │ Fire Rate│ Damage/Shot  │ Velocity  │ Range (Life)│
├──────────────┼──────────┼──────────┼──────────────┼───────────┼─────────────┤
│ AK-47        │ 30 rds   │ 0.105 s  │ 18.0 HP      │ 760 u/s   │ 1.17 s(889u)│
│ M1911 Pistol │ 8 rds    │ 0.000 s* │ 14.0 HP      │ 1690 u/s  │ 0.53 s(889u)│
│ KAR98k Sniper│ 5 rds    │ 1.250 s  │ 100.0 HP (KO)│ 2100 u/s  │ 1.73 s      │
│ Mossberg 500 │ 6 rds    │ 0.620 s  │ 17.0 HP x 6  │ 830 u/s   │ 0.51 s      │
└──────────────┴──────────┴──────────┴──────────────┴───────────┴─────────────┘
* M1911 requires discrete semi-automatic mouse trigger releases between shots.
```

### 2.1 Assault Rifle (`PlayerClass::Ak47`)
- **REQ-WEAP-001 (Ammunition Capacity):** The AK-47 magazine capacity SHALL be exactly 30 rounds (`cfg::RifleAmmo`).
- **REQ-WEAP-002 (Fire Rate):** The weapon SHALL fire continuously while the fire input flag is held, enforcing a minimum interval of `0.105 seconds` between consecutive shots.
- **REQ-WEAP-003 (Ballistics & Damage):** Each bullet SHALL travel at `760.0 units/second` for a maximum lifetime of `1.17 seconds` (`889.2 units` range), dealing `18.0 HP` damage upon impact.
- **REQ-WEAP-004 (Class Mobility):** A soldier equipped with the AK-47 SHALL possess a baseline movement speed of `335.0 units/second`.
- **REQ-WEAP-005 (Viewport Zoom):** The camera zoom factor for an AK-47 user SHALL be set to `0.50x` (with smooth right-click aim focus zooming to `0.85x`).

### 2.2 Pistol (`PlayerClass::Pistol` / M1911)
- **REQ-WEAP-006 (Ammunition Capacity):** The pistol magazine capacity SHALL be exactly 8 rounds (`cfg::PistolAmmo`).
- **REQ-WEAP-007 (Semi-Automatic Enforcement):** The pistol SHALL NOT support continuous automatic fire. The server MUST require the player to release the fire button before a subsequent shot can be dispatched (`(player.previousInputFlags & InputFlags::Fire) == 0`).
- **REQ-WEAP-008 (Ballistics & Equalized Range):** Each bullet SHALL travel at `1690.0 units/second` for a lifetime of `0.526 seconds` (matching the AK-47 range of `889.2 units`), dealing `14.0 HP` damage upon impact.
- **REQ-WEAP-009 (High Mobility):** A soldier equipped with the pistol SHALL possess the highest baseline movement speed in the game at `385.0 units/second`.
- **REQ-WEAP-010 (Viewport Zoom):** The camera zoom factor for a pistol user SHALL be set to `0.50x` (with smooth right-click aim focus zooming to `0.85x`).

### 2.3 Sniper Rifle (`PlayerClass::Sniper` / KAR98k)
- **REQ-WEAP-011 (Ammunition Capacity):** The sniper rifle magazine capacity SHALL be exactly 5 rounds (`cfg::SniperAmmo`).
- **REQ-WEAP-012 (Fire Rate & Cooldown):** The bolt-action mechanism MUST enforce a cooldown of `1.250 seconds` between shots.
- **REQ-WEAP-013 (Lethal Damage):** Each bullet SHALL inflict `100.0 HP` damage (equal to `cfg::PlayerMaxHealth`), instantly eliminating a full-health opponent on a single direct hit.
- **REQ-WEAP-014 (High Velocity & Infinite Arena Range):** Bullets SHALL travel at `2100.0 units/second` with a lifetime of `1.725 seconds`, yielding an effective ballistic travel distance of `3622.5 units`.
- **REQ-WEAP-015 (Standard Mobility):** A soldier equipped with the sniper rifle SHALL move at baseline speed (`285.0 units/second`).
- **REQ-WEAP-016 (Full-Map View & Tactical Zoom):** The client camera for a sniper user MUST display the entire arena at `0.45x` stationary zoom without panning when moving, with smooth right-click aim focus zooming to `0.85x`.

### 2.4 Shotgun (`PlayerClass::Shotgun` / Mossberg 500)
- **REQ-WEAP-017 (Ammunition Capacity):** The shotgun magazine capacity SHALL be exactly 6 rounds (`cfg::ShotgunAmmo`).
- **REQ-WEAP-018 (Fire Rate):** The pump-action mechanism MUST enforce a cooldown of `0.620 seconds` between discharges.
- **REQ-WEAP-019 (Pellet Spread):** Each discharge MUST spawn exactly 6 individual pellets distributed symmetrically across a cone spread of `0.28 radians` (~16.04 degrees).
- **REQ-WEAP-020 (Pellet Ballistics & Lethality):** Each pellet SHALL travel at `830.0 units/second` for a lifetime of `0.51 seconds`, dealing `17.0 HP` damage per pellet.
- **REQ-WEAP-021 (Agile Mobility):** A soldier equipped with the shotgun SHALL move at an accelerated speed of `370.0 units/second`.
- **REQ-WEAP-022 (Viewport Zoom):** The camera zoom factor for a shotgun user SHALL be set to `0.50x` (with smooth right-click aim focus zooming to `0.85x`).

### 2.5 Universal Reloading & Dry-Fire
- **REQ-WEAP-023 (Reload Duration):** Reloading any weapon SHALL require an uninterrupted duration of `1.25 seconds` (`cfg::ReloadSeconds`).
- **REQ-WEAP-024 (Reload Trigger & Auto-Reload):** Reloading MUST automatically trigger whenever the weapon's magazine reaches 0 rounds or dry-fire is attempted, and MAY also be manually requested via the `Reload` input flag (`R` key) if current ammo is strictly below magazine capacity.
- **REQ-WEAP-025 (Dry-Fire Indication):** Attempting to fire with an empty magazine (0 rounds) MUST suppress bullet generation and immediately play the dry-fire sound effect (`empty_click.wav`).

---

## 3. Functional Requirements: Combat, Abilities & Player Physics

### 3.1 Health, Damage & Vampirism Mechanic
- **REQ-COMB-001 (Health Limit):** Maximum player health SHALL be `100.0 HP` (`cfg::PlayerMaxHealth`).
- **REQ-COMB-002 (Hitbox Sizing):**
  - Physical collision radius: `18.0 units` (`cfg::PlayerRadius`) for obstacle and arena bounds checking.
  - Bullet hit-detection radius: `46.0 units` (`cfg::PlayerHitRadius`) to provide forgiving hit registration over UDP networks.
- **REQ-COMB-003 (Kill Reward):** Upon landing a killing blow on an opponent, the attacker's health MUST be **restored by 50% max HP** (`attacker->health = min(cfg::PlayerMaxHealth, attacker->health + 0.5F * cfg::PlayerMaxHealth)`), and their grenade and deployable barrier charges MUST be **fully reset to class maximum** (`attacker->grenades = ClassMaxGrenades()`, `attacker->barriers = ClassMaxBarriers()`). Self-inflicted deaths MUST NOT trigger kill rewards.
- **REQ-COMB-004 (Hit Feedback):** The client SHALL detect downward health deltas on opponent players and trigger an audible hitmarker confirmation (`hitmarker.wav`).

### 3.2 Tactical Combat Dash
- **REQ-ABIL-001 (Dash Speed & Duration):** Activating the dash ability SHALL propel the soldier at `860.0 units/second` (`cfg::DashSpeed`) for an active window of `0.14 seconds` (`cfg::DashSeconds`).
- **REQ-ABIL-002 (Dash Cooldown):** The dash ability MUST enforce a cooldown period of `1.15 seconds` (`cfg::DashCooldownSeconds`).
- **REQ-ABIL-003 (Vector Resolution):** If movement keys are currently depressed, the dash direction MUST align with the normalized movement vector. If the soldier is stationary, the dash MUST propel toward the player's aim reticle.

### 3.3 Frag Grenade
- **REQ-ABIL-004 (Supply Limit):** Players SHALL spawn with class-specific frag grenade charges per life: Assault Rifle (3 frags, `cfg::RifleGrenades`), Pistol (5 frags, `cfg::PistolGrenades`), Sniper Rifle (1 frag, `cfg::SniperGrenades`), and Shotgun (2 frags, `cfg::ShotgunGrenades`).
- **REQ-ABIL-005 (Targeting & Velocity):** The grenade SHALL be launched toward the world-coordinate mouse position supplied in the input packet at a velocity of `720.0 units/second` (`cfg::GrenadeThrowSpeed`).
- **REQ-ABIL-006 (Flight Timing & Arc):**
  - Flight duration SHALL scale with distance, clamped strictly between `0.18 seconds` (min) and `0.78 seconds` (max).
  - While airborne, the grenade's visual altitude MUST follow a sinusoidal arc peaking at `46.0 units` (`cfg::GrenadeArcHeight`), accompanied by an offset ground shadow.
- **REQ-ABIL-007 (Fuse & Blast):**
  - Upon landing at the target coordinate, an authoritative fuse of `1.1 seconds` (`cfg::GrenadeFuseSeconds`) MUST initiate.
  - Clients MUST render an expanding yellow blast radius ring during the fuse phase.
  - Upon fuse expiration, the grenade MUST detonate, inflicting `100.0 HP` damage (`cfg::GrenadeDamage = cfg::PlayerMaxHealth`, 1-hit kill) to all live players within an expanded radius of `149.5 units` (`cfg::GrenadeExplosionRadius = 115.0F * 1.30F`).

### 3.4 Deployable Tactical Barrier
- **REQ-ABIL-008 (Supply Limit):** Players SHALL spawn with class-specific deployable barrier charges per life: Assault Rifle (3 barriers, `cfg::RifleBarriers`), Pistol (5 barriers, `cfg::PistolBarriers`), Sniper Rifle (1 barrier, `cfg::SniperBarriers`), and Shotgun (1 barrier, `cfg::ShotgunBarriers`).
- **REQ-ABIL-009 (Placement Geometry):** The barrier MUST spawn `70.0 units` ahead along the player's aim vector, oriented perpendicular to the aim direction (`rotation = atan2(aim.y, aim.x) + π/2`).
- **REQ-ABIL-010 (Dimensions & Health):**
  - Barrier dimensions SHALL be `84.0 units wide` by `28.0 units thick` (`cfg::BarrierWidth x cfg::BarrierHeight`).
  - Baseline barrier durability SHALL be `90.0 HP` (`cfg::BarrierHealth`).
- **REQ-ABIL-011 (Ballistic Absorption):** Incoming bullets colliding with the barrier's bounding box MUST inflict damage to the barrier and be consumed, protecting players behind it. The barrier MUST be destroyed and despawned once its health drops to `0.0 HP` or below.

### 3.5 Death, Respawn & Class Switching
- **REQ-COMB-005 (Death State):** When a player's health drops to 0 or below, their state MUST immediately transition to inactive (`alive = false`), deaths counter MUST increment, and a `2.0-second` respawn delay MUST be enforced (`cfg::RespawnSeconds`).
- **REQ-COMB-006 (Respawn Modal & Invulnerability):**
  - While dead, the client MUST display an interactive **Respawn Modal**.
  - Dead players MUST remain invulnerable and hidden from the active battlefield until they respawn.
  - The modal MUST permit the player to select any of the 4 classes (AK47, M1911, KAR98k, Mossberg 500) prior to spawning.
- **REQ-COMB-007 (Respawn Execution):** Upon pressing Enter or clicking "Respawn" after the 2-second delay has elapsed, the server MUST reset health to 100 HP, refill class ammo, reset abilities (grenade and barrier charges refreshed), and place the soldier at an assigned spawn point.

---

## 4. Functional Requirements: Maps & Environments

### 4.1 Arena Dimensions & Boundaries
- **REQ-MAP-001 (World Boundary):** The game world MUST measure `2200.0 units` in width and `1400.0 units` in height (`cfg::WorldWidth x cfg::WorldHeight`).
- **REQ-MAP-002 (Margin Clamping):** Player movement MUST be clamped within a boundary margin of `90.0 units` from outer arena borders.
- **REQ-MAP-003 (Collision Grid):** The arena floor MUST render with a tactical grid spaced at `80.0 units` per cell.

### 4.2 Spawn Point Allocation
- **REQ-MAP-004 (Spawn Locations):** All maps MUST provide four fixed corner spawn coordinates:
  - **Spawn 0:** `(190.0, 190.0)` — Top-Left
  - **Spawn 1:** `(2010.0, 1210.0)` — Bottom-Right
  - **Spawn 2:** `(190.0, 1210.0)` — Bottom-Left
  - **Spawn 3:** `(2010.0, 190.0)` — Top-Right
- **REQ-MAP-005 (Round-Robin Assignment):** Spawns MUST be assigned deterministically based on player index / ID modulo 4.

### 4.3 Map Layout Specifications

```
Map 0: Monolith Fortress           Map 1: Crossfire Lanes             Map 2: Urban Labyrinth
┌──────────────────────────────┐   ┌──────────────────────────────┐   ┌──────────────────────────────┐
│ [Sp0]    [Bunker]            │   │ [Sp0] [Cover]   [Pillar]     │   │ [Sp0]    [Wall]       [Cover]│
│                              │   │                              │   │                              │
│      [Col] [MONO] [Col]      │   │      |Div|   [CENTER]  |Div| │   │       |Wall|  [Crate]  |Wall|│
│                              │   │                              │   │                              │
│            [Bunker]    [Sp1] │   │             [Pillar]  [Cover]│   │ [Cover]     [Wall]     [Sp1] │
└──────────────────────────────┘   └──────────────────────────────┘   └──────────────────────────────┘
```

- **REQ-MAP-006 (Map 0 — "Monolith Fortress"):** MUST contain exactly 7 rectangular obstacles:
  1. Center Monolith: position `(1100, 700)`, size `(130, 430)`
  2. Top-Left Bunker: position `(560, 330)`, size `(360, 76)`
  3. Bottom-Right Bunker: position `(1640, 1070)`, size `(360, 76)`
  4. Bottom-Left Bunker: position `(560, 1070)`, size `(320, 76)`
  5. Top-Right Bunker: position `(1640, 330)`, size `(320, 76)`
  6. Left Pillar: position `(850, 700)`, size `(90, 240)`
  7. Right Pillar: position `(1350, 700)`, size `(90, 240)`

- **REQ-MAP-007 (Map 1 — "Crossfire Lanes"):** MUST contain exactly 7 rectangular obstacles:
  1. Center Horizontal Barricade: position `(1100, 700)`, size `(560, 90)`
  2. Left Vertical Divider: position `(570, 700)`, size `(90, 540)`
  3. Right Vertical Divider: position `(1630, 700)`, size `(90, 540)`
  4. Upper-Left Mid Pillar: position `(830, 400)`, size `(90, 260)`
  5. Lower-Right Mid Pillar: position `(1370, 1000)`, size `(90, 260)`
  6. Top-Left Corner Cover: position `(360, 360)`, size `(250, 76)`
  7. Bottom-Right Corner Cover: position `(1840, 1040)`, size `(250, 76)`

- **REQ-MAP-008 (Map 2 — "Urban Labyrinth"):** MUST contain exactly 8 rectangular obstacles:
  1. Center Square Crate: position `(1100, 700)`, size `(120, 120)`
  2. Upper-Left Horizontal Wall: position `(760, 480)`, size `(420, 70)`
  3. Lower-Right Horizontal Wall: position `(1440, 920)`, size `(420, 70)`
  4. Bottom-Left Horizontal Wall: position `(410, 1040)`, size `(290, 70)`
  5. Top-Right Horizontal Wall: position `(1790, 360)`, size `(290, 70)`
  6. Lower-Left Vertical Wall: position `(760, 920)`, size `(70, 300)`
  7. Upper-Right Vertical Wall: position `(1440, 480)`, size `(70, 300)`
  8. Bottom-Center Horizontal Wall: position `(1100, 1080)`, size `(360, 70)`

---

## 5. Functional Requirements: Game Modes & Match Progression

### 5.1 Game Modes
- **REQ-MODE-001 (Free-For-All Timed — `FfaTimed`):**
  - Match runs for a configured duration (default: `3.0 minutes`).
  - Friendly fire SHALL be enabled (all players damage each other).
  - The winner SHALL be the player with the highest kill tally upon timer expiration.
- **REQ-MODE-002 (Free-For-All Score Limit — `FfaScore`):**
  - Match terminates immediately when any player attains the score limit (default: `15 kills`).
  - The first player to reach the limit is declared the winner.
- **REQ-MODE-003 (Team Deathmatch Timed — `TdmTimed`):**
  - Players are partitioned into **Red Team** and **Blue Team**.
  - Friendly fire MUST be disabled (teammates take zero damage from friendly bullets/explosions).
  - Total team score equals the sum of member kills. Team with higher score at time expiration wins.
- **REQ-MODE-004 (Team Deathmatch Score Limit — `TdmScore`):**
  - Team mode terminating immediately when any player/team reaches the score ceiling (default: `15 kills`).

### 5.2 Match Flow & Start Enforcement
- **REQ-FLOW-001 (Lobby Capacity):** The room MUST accept between 2 (`cfg::MinPlayers`) and 4 (`cfg::MaxPlayers`) players.
- **REQ-FLOW-002 (Host Authority):** Only the player designated as host (`host == true`) SHALL have authority to configure match parameters or issue start commands.
- **REQ-FLOW-003 (TDM Balance Gate):** The server MUST reject match start requests for TDM modes if the room contains an odd number of players, returning the error: `"TDM requires an even number of players."`.
- **REQ-FLOW-004 (Minimum Player Gate):** The server MUST reject start requests if fewer than 2 players are present, returning: `"Need at least 2 players."`.
- **REQ-FLOW-005 (Start Countdown):** Upon successful match start, an authoritative `3.0-second` countdown (`cfg::MatchCountdownSeconds`) MUST execute. Movement, firing, and abilities MUST be locked until countdown reaches `0.0`.
- **REQ-FLOW-006 (Post-Match Results Modal):**
  - When the win condition is met, `matchRunning` MUST switch to `false`.
  - Clients MUST freeze match state and present the **Match Results Modal** displaying winner announcement, team totals, and a player leaderboard (Player ID, Team, Kills, Deaths, K/D ratio).

---

## 6. Functional Requirements: Dedicated Server & Multi-Room Management

```
Client Connection Flow:
[Client] ─── HelloPacket (Create / Join) ────────▶ [GameServer : 42069]
[Client] ◀── ServerMessagePacket ("Room X created/joined") ─── [GameServer]
[Client] ─── LobbyConfigPacket ──────────────────▶ [GameServer]
[Client] ─── StartMatchPacket (Host) ────────────▶ [GameServer]
[Client] ◀── SnapshotPacket @ 20Hz ─────────────── [GameServer]
[Client] ─── InputPacket @ 30Hz ─────────────────▶ [GameServer]
```

### 6.1 Multi-Room Hosting
- **REQ-SRV-001 (Room Isolation):** The server SHALL support concurrent, isolated rooms managed via an in-memory map keyed by room codes (`std::unordered_map<std::string, Room>`).
- **REQ-SRV-002 (Room Code Generation):** For new room creation requests, the server MUST generate a cryptographically random, unique **4-digit numerical room code** (`cfg::RoomCodeLength = 4`, e.g. `"4082"`).
- **REQ-SRV-003 (Host Assignment & Migration):** The first player joining a newly created room SHALL be assigned host privileges. If the host leaves or times out, host authority MUST automatically migrate to the next active player.
- **REQ-SRV-004 (Empty Room Pruning):** When a room's active player count drops to zero, the room MUST be dismantled and erased from memory immediately.

### 6.2 Session Management & Heartbeat Timeout
- **REQ-SRV-005 (Address Session Tracking):** The server MUST maintain client sessions mapped by IP address string (`std::unordered_map<std::string, ClientSession>`).
- **REQ-SRV-006 (Inactivity Timeout Watchdog):** If the server receives no valid UDP packets from a client session for `8.0 seconds` (`cfg::ClientTimeoutSeconds`), the server MUST disconnect the player, remove them from the active room, and log the timeout event.

### 6.3 Simulation Loop & Continuous Collision Detection (CCD)
- **REQ-SRV-007 (Server Tick Rate):** The server physics simulation MUST update at a fixed step rate of `60 Hz` (`cfg::FixedDt = 1/60s = 0.01667s`).
- **REQ-SRV-008 (Anti-Tunneling CCD):** To prevent high-speed projectiles (e.g. 2100 u/s sniper rounds) from tunneling through obstacles or players between ticks:
  - Bullet-obstacle collision MUST perform 2D Liang-Barsky line-segment clipping against obstacle rectangles (`SegmentIntersectsRect`).
  - Bullet-player hit registration MUST calculate distance from the player center to the continuous trajectory segment (`DistanceToSegment`).

---

## 7. Functional Requirements: Network Protocol & Data Formats

### 7.1 Transport & Wire Framing
- **REQ-NET-001 (Transport Layer):** Communication MUST operate exclusively over connectionless UDP on port `42069` (`cfg::ServerPort`).
- **REQ-NET-002 (Endianness & Packing):** All packets MUST be structured as plain-old-data (POD) binary structs. No dynamic heap allocations or variable-length encodings SHALL be used in packet transmission.
- **REQ-NET-003 (Header Validation):** Every packet MUST begin with an 8-byte `PacketHeader`:
  ```cpp
  struct PacketHeader {
      std::uint32_t magic;    // MUST equal 0x54544F50 ("TTOP")
      std::uint16_t version;  // MUST equal 1
      PacketType type;        // PacketType enum (1-6)
      std::uint16_t size;     // Payload byte size
  };
  ```
  Any packet with invalid magic, mismatched version, or malformed length MUST be dropped immediately.

### 7.2 Packet Catalog

| Type Enum | Identifier | Direction | Max Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `PacketType::Hello` | 1 | Client → Server | 37 B | Handshake packet specifying player name, 4-digit room code, and `createRoom` boolean flag. |
| `PacketType::LobbyConfig` | 2 | Client → Server | 20 B | Configuration packet with game mode, class selection, team ID, map index, and score/time limits. |
| `PacketType::Input` | 3 | Client → Server | 30 B | Input frame packet with sequence ID, bitflags, normalized aim vector, and grenade target coordinates. |
| `PacketType::StartMatch` | 4 | Client → Server | 8 B | Empty payload header triggering match countdown. |
| `PacketType::Snapshot` | 5 | Server → Client | ~1.8 KB | Authoritative snapshot containing players, bullets, grenades, barriers, timer, and scores. |
| `PacketType::ServerMessage` | 6 | Server → Client | 104 B | String notification packet for room confirmations, join rejections, and error popups. |

### 7.3 Tick & Transmission Rates
- **REQ-NET-004 (Snapshot Broadcast Rate):** The server MUST broadcast authoritative `SnapshotPacket` states to all connected clients at a frequency of **20 Hz** (`cfg::SnapshotSeconds = 1/20s = 0.05s`).
- **REQ-NET-005 (Client Input Rate):** The client MUST accumulate and transmit `InputPacket` updates to the server at a frequency of **30 Hz** (`cfg::ClientSendSeconds = 1/30s = 0.0333s`).

---

## 8. Functional Requirements: User Interface & Experience (UI/UX)

### 8.1 Screen State Machine
- **REQ-UI-001 (Screens):** The client SHALL implement four discrete screens managed by `ScreenManager`:
  1. `MainMenu`: Landing screen with "Create Room" and "Join Room" options.
  2. `RoomCreation`: Host lobby screen for selecting Mode, Class, and Map.
  3. `RoomJoining`: Client join screen with Server IP text input and 4-digit room code entry.
  4. `Gameplay`: Active match arena, HUD, camera, and in-game modal overlays.
- **REQ-UI-002 (Escape Navigation):** Pressing `Escape` on secondary menu screens (`RoomCreation`, `RoomJoining`) MUST navigate back to `MainMenu`. In `Gameplay`, pressing `Escape` MUST return to `MainMenu`.

### 8.2 In-Game Heads-Up Display (HUD)
- **REQ-UI-003 (Header Status Bar):** A top banner (`64 pixels` tall) MUST render at all times during gameplay showing:
  - Player combat stats: `Ammo`, `Frags`, `Barriers`, and `K/D` ratio (spaced with clear labels and values; `Health` removed to avoid clutter as it is rendered over player characters in-world; `Game Mode` removed as it is accessible in the pause menu).
  - Team Scoreboard: `Red` score and `Blue` score (rendered only in team-based modes like `TDM`).
  - Match Countdown Clock: `Time` remaining in `MM:SS` format (rendered in timed modes).
  - Connection & room status string (dynamically right-aligned to prevent overlapping buttons or other UI).
  - Host Action Prompt: `[Enter: Start]` visible only to the room host when the match is idle.

### 8.3 Modal Overlays
- **REQ-UI-004 (Help Overlay):**
  - Toggled via `F1` key or by clicking the `?` icon in the top-left HUD corner.
  - Must display all control keybindings in a clear two-column layout.
- **REQ-UI-005 (Message Popup Modal):**
  - Used for displaying server notifications or error alerts (e.g. `"Room is full."`, `"Room 1234 not found."`).
  - Must display title, message body, and dismissible `OK` / `X` buttons.
- **REQ-UI-006 (Respawn Modal):**
  - Displayed exclusively while the local soldier is dead (`alive == false`).
  - Displays 4 interactive class buttons (AK47, M1911, KAR98k, Mossberg 500) and an active `"Respawn"` action button.
- **REQ-UI-007 (End-of-Match Modal):**
  - Automatically triggered upon match completion.
  - Displays match winner, game mode, team score summary, and detailed tabular player statistics (Player ID, Team, Kills, Deaths, K/D ratio).

### 8.4 Input & Keybindings Specification
- **REQ-UI-008 (Default Controls Mapping):**

| Action | Primary Key | Secondary Key | Input Type |
| :--- | :--- | :--- | :--- |
| **Move Up** | `W` | `Up Arrow` | Digital Hold |
| **Move Down** | `S` | `Down Arrow` | Digital Hold |
| **Move Left** | `A` | `Left Arrow` | Digital Hold |
| **Move Right** | `D` | `Right Arrow` | Digital Hold |
| **Aim Direction** | `Mouse Cursor` | — | Analog Coordinate |
| **Focus / Zoom** | `Right Mouse Button` | — | Digital Hold |
| **Fire Weapon** | `Left Mouse Button` | — | Digital Hold / Click |
| **Reload** | `R` | — | Digital Press |
| **Combat Dash** | `Space` | — | Digital Press |
| **Frag Grenade** | `G` | `E` | Digital Press |
| **Deploy Barrier** | `B` | `Q` | Digital Press |
| **Start Match** | `Enter` | Click `Enter: Start` | Digital Press (Host only) |
| **Respawn** | `Enter` | Click `Respawn` | Digital Press (Dead only) |
| **Toggle Help** | `F1` | Click `?` icon | Digital Press |
| **Back / Exit** | `Escape` | — | Digital Press |

---

## 9. Functional Requirements: Assets & Audio

### 9.1 Visual Assets & Rendering
- **REQ-AST-001 (Sprite Textures):**
  - Local soldier sprite: `assets/player_soldier.png`.
  - Opponent soldier sprite: `assets/opponent_soldier.png`.
- **REQ-AST-002 (Point Filtering):** Textures MUST be rendered using nearest-neighbor point filtering (`TEXTURE_FILTER_POINT`) to preserve crisp pixel art aesthetics under scaling (`0.86x` sprite scale).
- **REQ-AST-003 (Aim Rotation):** The player sprite MUST dynamically rotate to match the calculated mouse aim angle in world coordinates.
- **REQ-AST-004 (Health Bars):** Every active player MUST render a 2-tone health bar directly above their soldier sprite (`48 pixels wide`, green health fill on dark background).

### 9.2 Audio Catalog & Fallback Safety
- **REQ-AST-005 (Audio Catalog):**

| Sound File | Trigger Event | Audio Behavior |
| :--- | :--- | :--- |
| `assets/shot_ak47.wav` | Firing AK-47 | Assault rifle gunfire SFX |
| `assets/shot_m1911.wav` | Firing M1911 | Crisp pistol discharge SFX |
| `assets/shot_kar98k.wav` | Firing KAR98k | Heavy sniper rifle discharge SFX |
| `assets/shot_mossberg500.wav` | Firing Mossberg | Multi-pellet shotgun blast SFX |
| `assets/empty_click.wav` | Fire attempted with 0 ammo | Dry-fire mechanical click SFX |
| `assets/hitmarker.wav` | Damage inflicted on opponent | Audio hit confirmation chirp SFX |
| `assets/grenade.wav` | Grenade launch | Pin pull and launch SFX |
| `assets/barrier.wav` | Barrier deployment | Barricade placement SFX |

- **REQ-AST-006 (Audio Device Safety):** If an audio output device is unavailable or fails to initialize, `AudioManager` MUST log a warning and continue executing in silent mode without crashing.
- **REQ-AST-007 (Asset Fallback):** If soldier texture files fail to load, `AssetManager` MUST fall back to drawing solid colored circles (`GREEN` for local player, `YELLOW` for opponents) to ensure the game remains fully playable.

---

## 10. Non-Functional Requirements (NFR)

### 10.1 Performance & Rendering
- **NFR-PERF-001 (Frame Rate):** The game client SHALL render at a stable target of `60 frames per second` (`cfg::TargetFps = 60`) with vertical sync enabled (`FLAG_VSYNC_HINT`).
- **NFR-PERF-002 (Window Geometry):** Default window resolution SHALL be `1280 x 720` pixels (`cfg::WindowWidth x cfg::WindowHeight`) operating in borderless windowed mode.
- **NFR-PERF-003 (Deterministic Physics Step):** The authoritative server physics simulation MUST advance using fixed delta time integration (`dt = 1/60s`) regardless of network packet frequency.

### 10.2 Networking Efficiency
- **NFR-NET-001 (Packet Size Envelope):** Total snapshot packet size MUST stay well below standard MTU limits (1500 bytes) or the configured socket buffer limit (`cfg::PacketBytes = 2048 bytes`), preventing IP packet fragmentation.
- **NFR-NET-002 (Zero Dynamic Allocation in Hot Path):** Neither the client input loop nor the server snapshot broadcast loop SHALL perform heap memory allocations (`malloc` / `new`) during active frame processing.

### 10.3 Portability & Tooling
- **NFR-PORT-001 (Cross-Platform Socket Layer):** Sockets MUST compile cleanly on both Windows (Winsock2) and POSIX platforms (BSD sockets) via conditional compilation flags in `UdpSocket.hpp`.
- **NFR-PORT-002 (Compiler Compatibility):** The codebase MUST compile with zero fatal warnings under GCC 11+, Clang 13+, and MSVC 2019+ targeting C++20.

---

## 11. Verification & Acceptance Criteria (Quality Assurance)

| Test ID | Requirement Trace | Verification Procedure | Expected Outcome |
| :--- | :--- | :--- | :--- |
| **TC-WEAP-01** | `REQ-WEAP-002`, `007` | Hold fire button on AK-47 and M1911. | AK-47 fires continuously at 9.5 rps; M1911 fires exactly once and requires release to fire again. |
| **TC-WEAP-02** | `REQ-WEAP-013`, `016` | Equip KAR98k; check viewport zoom and hit unarmored enemy. | Viewport zooms out to 0.50x; single body shot deals 100 HP, killing opponent instantly. |
| **TC-COMB-01** | `REQ-COMB-003` | Take damage to 20 HP, then kill an enemy player. | Attacker's health instantly rebounds from 20 HP to 100 HP. |
| **TC-COMB-02** | `REQ-ABIL-001`, `002` | Press Space while moving; immediately press Space again. | First press bursts at 860 u/s for 0.14s; second press ignored until 1.15s cooldown elapses. |
| **TC-ABIL-01** | `REQ-ABIL-005` to `007`| Throw grenade; observe trajectory, fuse, and explosion. | Parabolic arc reaches 46u height with shadow; 1.1s fuse indicator appears; 100 HP (1-hit kill) dealt in 149.5u radius. |
| **TC-ABIL-02** | `REQ-ABIL-009` to `011`| Deploy barrier; fire bullets directly into it. | Barrier blocks bullets, health decreases from 90 HP, and barrier disappears when depleted. |
| **TC-MODE-01** | `REQ-FLOW-003` | Attempt to start a TDM match with 3 players in lobby. | Server rejects start request with `"TDM requires an even number of players."`. |
| **TC-NET-01** | `REQ-SRV-006` | Force-terminate a client process while connected to a room. | Server detects packet absence, times out player after 8.0s, and updates remaining clients. |
| **TC-NET-02** | `REQ-SRV-008` | Fire KAR98k through a solid brick obstacle wall at a target behind it. | Bullet is stopped by Liang-Barsky box clipping; player behind cover takes zero damage. |
| **TC-AST-01** | `REQ-AST-006` | Run client on a machine with audio device disabled. | Client displays log warning, loads visual window normally, and runs without crashing. |
