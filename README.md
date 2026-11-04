# mod-twow-barbershop: Native Turtle WoW Barbershop Module

A high-performance server-side module for Turtle WoW (1.18.1 / 1.12.1 vMaNGOS) that powers the client's built-in Barbershop interface without requiring any client addons.

![Barbershop Preview](preview.jpg)

---

## Features

- **Native Client Compatibility**: Fully implements the server-side `TW_BARBERSHOP` protocol used by the built-in Barbershop interface (`Turtle_BarbershopUI`). No client addons or MPQ modifications required.
- **Hair and Facial Customization**:
  - Hair style selection with complete race and gender bounds.
  - Hair color selection.
  - Facial hair, piercings, markings, horns, tusks, and features.
  - Supports all custom hairstyle additions in Turtle WoW across all 10 playable races, including Goblins and High Elves.
- **Private Single-Viewer Previews**:
  - Appearance previews are sent exclusively to the seated player's session using targeted values update packets.
  - Prevents network bandwidth overhead and prevents uncommitted preview changes from broadcasting to surrounding players.
  - Changes automatically revert if the player cancels, stands up, dies, moves away, or disconnects.
- **Dynamic Pricing Engine**:
  - Calculates customization costs dynamically using the standard level-squared formula: `(Level ^ 2) * BaseCost * (Modifier / 100)`.
  - Fully configurable cost percentages for hair style, hair color, and facial hair.
- **Decoupled Architecture**:
  - Zero edits to core engine files.
  - Bypasses core guild packet drops for guildless players by inspecting addon messages during packet receive hooks.
  - Directly updates standard character appearance fields (`playerBytes` and `playerBytes2`) without custom tables.

---

## Requirements

- Turtle WoW custom vMaNGOS core (`tortoise-wow` targeting client 1.18.1.7272).
- MariaDB / MySQL.

---

## Installation

From the TortoiseWoW source root, clone or copy the module directly into `modules/mod-twow-barbershop/`.

```bash
git clone https://github.com/Ildourol/mod-twow-barbershop.git modules/mod-twow-barbershop
```

**Linux**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMODULE_MOD_TWOW_BARBERSHOP=static
cmake --build build --parallel
```

**Windows** (PowerShell, Visual Studio 2022 with C++ tools)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMODULE_MOD_TWOW_BARBERSHOP=static
cmake --build build --config Release --parallel
```

Preserve the CMake options for your existing server and modules. Deploy the rebuilt `mangosd` (`mangosd.exe` on Windows). Copy the module’s `conf/mod-twow-barbershop.conf.dist` to `modules/mod-twow-barbershop.conf` beside the active `mangosd.conf` and restart after completing any setup below.

**Database.** On a fresh database, no manual SQL import is needed with `Database.AutoUpdate.Enabled = 1` if `Database.AutoUpdate.AllowedModules` includes this module (or is `all`) and the server can read the module SQL at its build-time source path. The updater applies the world script and runs character cleanup. If you need to preserve the legacy `character_barbershop_look` table, exclude this module from the updater and import only `data/sql/world/001_barbershop_chair.sql` into the world database.

## Configuration Reference

Configuration options live in `mod-twow-barbershop.conf` under the `[Barbershop]` section:

| Option | Type | Default | Description |
|---|---|---|---|
| `Barbershop.Enable` | boolean | `1` | Master toggle to enable or disable the barbershop system. |
| `Barbershop.ChairEntries` | string | `"105180"` | Space or comma-separated list of GameObject template entries acting as barber chairs. |
| `Barbershop.Cost.Base` | integer | `5` | Base multiplier in copper for level-squared cost calculations. |
| `Barbershop.Cost.Style` | integer | `100` | Percentage of base cost applied when changing hair style. |
| `Barbershop.Cost.Color` | integer | `50` | Percentage of base cost applied when changing hair color without changing style. |
| `Barbershop.Cost.FacialHair` | integer | `75` | Percentage of base cost applied when changing facial hair or features. |
| `Barbershop.MaxChairDistance` | float | `5.0` | Maximum distance in yards from the chair before an active session automatically closes. |

---

## In-Game Usage

1. Locate any Barbershop Chair (GameObject entry `105180`), found in salons in major cities such as Stormwind and Orgrimmar.
2. Right-click the chair to sit down.
3. The native Barbershop window will open automatically.
4. Use the arrow selectors to preview hair styles, hair colors, and facial features.
5. Click **Accept** to finalize and purchase your new look, or click **Cancel** or stand up to revert to your original appearance.
