<p align="center">
  <img width="50%" align="center" src="https://jacee.dev/img/mods/ssa_improved.png" alt="">
</p>
<h1 align="center">
  SSA Improved - Emulated Portal of Power
</h1>
<p align="center">
  Play without a physical Portal of Power using fully software-emulated portal support
</p>

## Overview

The Emulated Portal of Power is a feature of SSA Improved that replaces the physical USB Portal of Power with a complete software simulation.
You can place any Skylander from your library onto the portal at any time through the in-game mod UI.
The Emulated Portal is compatible with the same Skylander dump files used by other emulators such as Cemu, RPCS3, and Dolphin.
If you already have a collection of `.sky` or `.bin` files from those tools, they will work here without any conversion.

> [!IMPORTANT]
> A restart is required when toggling the emulated portal on or off. The setting takes effect the next time the game initialises its portal connection.

---

## Enabling the Emulated Portal

There are two ways to enable it:

**Via the mod UI (recommended)**

1. Open the mod UI with `F1` on your keyboard, or press `L3` + `R3` simultaneously on a controller
2. Navigate to the **Portal** tab
3. Check **Enable Emulated Portal**
4. Restart your game

**Via the config file**

Open `ssa_impr_mod.ini` in the `ssa-improved` directory with a text editor and set:
```ini
EmulatedPortal=1
```
Then restart the game.<br>
When the emulated portal is active, any physical Portal of Power that is plugged in is fully ignored.

---

## Using the Portal Tab

Open the mod UI and go to the **Portal** tab.

### Portal Slots

The emulated portal supports **up to 4 simultaneous Skylanders**.
The slot table shows which Skylanders are currently placed on the portal.
Each slot has two buttons:
- **Set** - unlocks the library below and lets you pick a Skylander to place into that slot
- **Clear** - removes the Skylander from that slot (the game will see it as lifted off the portal)

### Assigning a Skylander to a Slot

1. Click **Set** on an empty slot
2. The **Library** section below is unlocked
3. Browse by element, then by character
4. Click the Skylander you want to place and it will be assigned to the slot instantly

If a file was given a nickname when it was created, the nickname is shown next to the display name, e.g. `Spyro ("My Spyro")`.

---

## Creating New Skylanders (Skylander Creator)

To use a Skylander with the emulated portal, you need a corresponding file on disk containing that figure's data.
You can either use dumps from a physical figure or from another emulator or create new files using the mod's built-in Skylander Creator.

1. Open the mod UI (`F1` or `L3+R3`)
2. Go to the **Portal** tab
3. Click **Open Skylander Creator**
4. Choose a Skylander from the list and enter a nickname
5. Click **Create**

The resulting file is saved into the appropriate subdirectory under `ssa-improved/skylanders/` and immediately appears in your library.

---

## Skylander Files

The emulated portal reads Skylander data from files on disk. This is the same format used by other Skylanders emulation tools.<br>
While playing the current state of your Skylander is continuously written back to its file, so any changes you make
to your figure (levelling up, unlocking new abilities, etc.) are saved in the Skylander file.

### File Storage

The mod stores all Skylander files under:

```
<game directory>/ssa-improved/skylanders/
```

This folder is created automatically on first launch. Inside it, each character has its own subdirectory:

```
ssa-improved/
  skylanders/
    spyro/
      spyro-My_Spyro.sky
    lightning_rod/
      lightning_rod-s2-Zeus.sky
    cynder/
      cynder-s2-Default.sky
```
_(character folders only exist if at least one figure of that Skylander has been created)_

### File Naming Convention

Files created by the mod's Skylander Creator follow this naming pattern:

```
<character_name>[-<variant_suffix>]-<nickname>.sky
```

For example:
- `spyro-My_Spyro.sky` - a base Spyro with the nickname "My Spyro"
- `lightning_rod-s2-Zeus.sky` - a Series 2 Lightning Rod with the nickname "Zeus"

The nickname is not the nickname of the Skylander in the game.
If you rename a file in a way that does not match this pattern, the mod will still load it.

### Accepted File Extensions

The mod accepts the following extensions:
- `.sky`
- `.bin`
- `.dump`
- `.dmp`

All of them must be exactly **1024 bytes** in size.