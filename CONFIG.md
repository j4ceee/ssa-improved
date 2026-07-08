<p align="center">
  <img width="50%" align="center" src="https://jacee.dev/img/mods/ssa_improved.png" alt="">
</p>
<h1 align="center">
  SSA Improved - Configuration
</h1>
<p align="center">
  Adjust the game to your liking
</p>

<details>
<summary><b>📖 Table of Contents (click to expand)</b></summary>

- [Game settings](#game-settings)
  - [Emulated Portal](#emulatedportal)
  - [Level loading](#levelloading)
- [Difficulty](#difficulty)
  - [Enemy HP multiplier](#hpmult)
  - [Enemy damage multiplier](#dmgmult)
  - [Enemy hit reaction](#hitreact)
  - [Heroic Challenge HP ceiling](#challengehp)
  - [Heroic Challenge damage ceiling](#challengedmg)
  - [XP multiplier](#xp)
- [Camera settings](#camera-settings)
  - [Free Cam](#freecam)
  - [Free Cam FOV](#fcfov)
- [Magic item settings](#magic-item-settings)
  - [Infinite item duration](#infitems)
  - [Reusable items](#reuseitems)
- [Skylander player settings](#skylander-player-settings)
  - [HP](#hp)
  - [God mode](#god)
  - [No knockback](#nonockback)
  - [No hit reaction](#nohitreaction)
  - [Team](#team)
- [Mod settings](#mod-settings)
  - [Font Size](#fontscale)
  - [Texture Mods](#texturemods)
  - [Dump Textures](#texturedump)
- [Game window](#game-window)
  - [Windowed Mode](#windowed)
  - [Borderless windowed mode](#borderless)
  - [Resolution](#resolution)
- [Graphics](#graphics)
  - [High-Res Rendering](#renderres)
  - [Supersampling](#ss)
  - [VSync](#vsync)
  - [FPS Cap](#fps)
  - [Anisotropic Filtering](#af)
  - [Texture Sharpness](#sharpness)
- [Performance](#performance)
  - [Disable Grass](#grass)

</details>

---

## Game settings

| Setting                                                                   | UI Location                         | Description                                                                                                                                                             | Default        | Config                                     |
|---------------------------------------------------------------------------|-------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------|--------------------------------------------|
| <h4><a id="emulatedportal" name="emulatedportal"></a>Emulated Portal</h4> | `Portal` → `Enable Emulated Portal` | Enables the emulated Portal of Power.<br>For a more detailed description see [EMULATED_PORTAL.md](https://github.com/j4ceee/ssa-improved/blob/main/EMULATED_PORTAL.md). | Disabled (`0`) | Key: `EmulatedPortal`<br>Values: `0` / `1` |
| <h4><a id="levelloading" name="levelloading"></a>Level loading</h4>       | `Game` → `Level`                    | Directly load into any challenge or campaign level.<br>Select a level from the dropdown & hit `Load`.                                                                   | -              | -                                          |

## Difficulty

| Setting                                                                               | UI Location                                               | Description                                                                                                                                                                                                                                                                                                         | Default       | Config                                            |
|---------------------------------------------------------------------------------------|-----------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------|---------------------------------------------------|
| <h4><a id="hpmult" name="hpmult"></a>Enemy HP multiplier</h4>                         | `Game` → `Difficulty` → `Enemy HP multiplier`             | Scales enemy HP. Enemies will be tougher to defeat when this value is increased.<br>This multiplier is applied to the base HP values of the enemies in the current level.                                                                                                                                           | `1.0`         | Key: `HpMult`<br>Values: `0.1` - `10.0`           |
| <h4><a id="dmgmult" name="dmgmult"></a>Enemy damage multiplier</h4>                   | `Game` → `Difficulty` → `Enemy damage multiplier`         | Scales enemy damage. Enemies will deal more damage when this value is increased.<br>This multiplier is applied to the base damage values of the enemies in the current level.                                                                                                                                       | `1.0`         | Key: `DmgMult`<br>Values: `0.1` - `10.0`          |
| <h4><a id="hitreact" name="hitreact"></a>Enemy hit reaction</h4>                      | `Game` → `Difficulty` → `Enemy hit reaction`              | Enables or disables enemy hit reaction. When enabled, enemies will react to being hit by the player (vanilla behaviour). When disabled, enemies will still take damage but will not react to being hit (recommended for higher difficulty). Enemies will attack in faster succession when they don't react to hits. | Enabled (`1`) | Key: `EnemyHitReaction`<br>Values: `0` / `1`      |
| <h4><a id="challengehp" name="challengehp"></a>Heroic Challenge HP ceiling</h4>       | `Game` → `Difficulty` → `Heroic Challenge HP ceiling`     | Maximum HP enemies can have in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this and [Enemy HP multiplier](#hpmult).                                                                                                                                                      | `3.0`         | Key: `HeroicHpCeiling`<br>Values: `0.1` - `10.0`  |
| <h4><a id="challengedmg" name="challengedmg"></a>Heroic Challenge damage ceiling</h4> | `Game` → `Difficulty` → `Heroic Challenge damage ceiling` | Maximum damage enemies can deal in Heroic Challenges. If you find that enemies in challenges are too weak, try increasing this and [Enemy damage multiplier](#dmgmult).                                                                                                                                             | `0.1`         | Key: `HeroicDmgCeiling`<br>Values: `0.1` - `10.0` |
| <h4><a id="xp" name="xp"></a>XP multiplier</h4>                                       | `Game` → `Experience` → `XP multiplier`                   | Multiplies the amount of XP gained from defeating enemies.                                                                                                                                                                                                                                                          | `1.0`         | Key: `XpMult`<br>Values: `0.1` - `10.0`           |

## Camera settings

| Setting                                              | UI Location                        | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | Default |
|------------------------------------------------------|------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------|
| <h4><a id="freecam" name="freecam"></a>Free Cam</h4> | `Game` → `Camera` → `Free Cam`     | Toggles free camera mode. Allows you to move the camera freely around the scene.<br><br>_Note: You can use this without restrictions, but beware that the game may crash if you use free cam during camera transitions (e.g. when the game switches from normal gameplay into a cutscene). To be safe, only use free cam during normal gameplay (where the camera follows your Skylander) and exit it before state changes (like level changes, entering a cutscene, entering Magic Moment)._ | -       |
| <h4><a id="fcfov" name="fcfov"></a>Free Cam FOV</h4> | `Game` → `Camera` → `Free Cam FOV` | Adjusts the field of view for the Free Cam camera.                                                                                                                                                                                                                                                                                                                                                                                                                                            | `53`    |

## Magic item settings

| Setting                                                              | UI Location                                 | Description                                                                                                                                                                             | Default        | Config                                           |
|----------------------------------------------------------------------|---------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------|--------------------------------------------------|
| <h4><a id="infitems" name="infitems"></a>Infinite item duration</h4> | `Game` → `Items` → `Infinite item duration` | All normal timed magic items will stay active forever.<br>_Note: Only affects newly placed items. Remove and re-place any currently attached magic items for the change to take effect_ | Disabled (`0`) | Key: `InfiniteItemDuration`<br>Values: `0` / `1` |
| <h4><a id="reuseitems" name="reuseitems"></a>Reusable items</h4>     | `Game` → `Items` → `Reusable items`         | Items can be used multiple times per level.<br>_Note: Only affects newly placed items. Remove and re-place any currently attached magic items for the change to take effect_            | Disabled (`0`) | Key: `ReusableItems`<br>Values: `0` / `1`        |

## Skylander player settings
_Note: These settings can be changed in the current play session. Each player has their own settings_.

| Setting                                                                 | UI Location                                                         | Description                                                                                    | Default     |
|-------------------------------------------------------------------------|---------------------------------------------------------------------|------------------------------------------------------------------------------------------------|-------------|
| <h4><a id="hp" name="hp"></a>HP</h4>                                    | `Game` → `Skylanders` → `Player 1` / `Player 2` → `HP`              | Change the current health of your Skylander. Drag the slider to change the HP.                 | -           |
| <h4><a id="god" name="god"></a>God mode</h4>                            | `Game` → `Skylanders` → `Player 1` / `Player 2` → `God mode`        | Makes the Skylander immune to all damage.                                                      | Disabled    |
| <h4><a id="nonockback" name="nonockback"></a>No knockback</h4>          | `Game` → `Skylanders` → `Player 1` / `Player 2` → `No knockback`    | Prevents the Skylander from being knocked back by attacks.                                     | Disabled    |
| <h4><a id="nohitreaction" name="nohitreaction"></a>No hit reaction</h4> | `Game` → `Skylanders` → `Player 1` / `Player 2` → `No hit reaction` | Prevents attack animations from being interrupted by hits.                                     | Disabled    |
| <h4><a id="team" name="team"></a>Team</h4>                              | `Game` → `Skylanders` → `Player 1` / `Player 2` → `Team`            | Changes the faction of the Skylander. Members of the same faction cannot attack the Skylander. | `Skylander` |

## Mod settings

| Setting                                                           | UI Location                                | Description                                                                                                                                                                                                                                                                               | Default        | Config                                    |
|-------------------------------------------------------------------|--------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------|-------------------------------------------|
| <h4><a id="fontscale" name="fontscale"></a>Font Size</h4>         | `Settings` → `Mod` → `UI Font Scale`       | Adjusts the scale of the mod UI font.                                                                                                                                                                                                                                                     | `1.0`          | Key: `FontScale`<br>Values: `0.5` - `3.0` |
| <h4><a id="texturemods" name="texturemods"></a>Texture Mods</h4>  | `Settings` → `Mod` → `Enable Texture Mods` | Enables texture mods. Texture mods can be placed into the `ssa-improved/textures/` subfolder and will be loaded from there.                                                                                                                                                               | Enabled (`1`)  | Key: `TextureMods`<br>Values: `0` / `1`   |
| <h4><a id="texturedump" name="texturedump"></a>Dump Textures</h4> | `Settings` → `Mod` → `Dump Textures`       | Dumps textures to the `ssa-improved/dumps/textures/` directory as they are loaded in-game. Useful for modders to create new texture mods. Want to learn how? [Click here](https://github.com/j4ceee/ssa-improved#create-tex).<br>**Recommended:** keep disabled (`0`) unless you need it. | Disabled (`0`) | Key: `TextureDump`<br>Values: `0` / `1`   |

## Game window

| Setting                                                                    | UI Location                                      | Description                                                                                                                                                                | Default          | Config                                                           |
|----------------------------------------------------------------------------|--------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------|------------------------------------------------------------------|
| <h4><a id="windowed" name="windowed"></a>Windowed Mode</h4>                | `Settings` → `Further options` → `Windowed Mode` | Controls whether the game is displayed in windowed mode or fullscreen.<br>_Note: Requires an immediate game restart_                                                       | Windowed (`1`)   | Key: `Windowed`<br>Values: `0` (Fullscreen) / `1` (Windowed)     |
| <h4><a id="borderless" name="borderless"></a>Borderless windowed mode</h4> | `Settings` → `Further options` → `Borderless`    | Hides the Windows title bar & displays the game in borderless windowed mode.<br>**Required:** [Windowed Mode](#windowed) to be active.                                     | Borderless (`1`) | Key: `Borderless`<br>Values: `0` / `1`                           |
| <h4><a id="resolution" name="resolution"></a>Resolution</h4>               | `Settings` → `Further options` → `Resolution`    | Change the resolution of the game (window). <br>**Recommended:** keep at `0` to use your desktop resolution. <br>**Recommended:** enable [High-Res Rendering](#renderres). | `0`              | Key: `ResolutionW` & `ResolutionH`<br>Values: Positive #s or `0` |

## Graphics

| Setting                                                            | UI Location                                           | Description                                                                                                                                                                                                                                                                | Default        | Config                                                                          |
|--------------------------------------------------------------------|-------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------|---------------------------------------------------------------------------------|
| <h4><a id="renderres" name="renderres"></a>High-Res Rendering</h4> | `Settings` → `Further options` → `High-Res Rendering` | The game internally always renders at a resolution of `1120x704`, this causes low image quality and lots of artifacting. Enabling this option allows the game to render at higher resolutions internally.<br>_Note: Requires an immediate game restart_                    | Enabled (`1`)  | Key: `RenderRes`<br>Values: `0` / `1`                                           |
| <h4><a id="ss" name="ss"></a>Supersampling</h4>                    | `Settings` → `Further options` → `Supersampling`      | Renders the scene at a higher internal resolution and downscales. Increases image quality & acts as antialiasing. Higher values cost more GPU. <br>**Required:** [High-Res Rendering](#renderres) needs to be enabled (`1`).<br>_Note: Requires an immediate game restart_ | `1.0`          | Key: `Supersampling`<br>Values: `1.0`, `1.5`, `2.0`, `2.5`, `3.0`, `3.5`, `4.0` |
| <h4><a id="vsync" name="vsync"></a>VSync</h4>                      | `Settings` → `Further options` → `VSync`              | Controls whether [Vertical Synchronisation](https://www.howtogeek.com/853225/what-is-vsync-and-should-you-enable-it/) is enabled.<br>_Note: Requires an immediate game restart_                                                                                            | Enabled (`1`)  | Key: `VSync`<br>Values: `0` / `1`                                               |
| <h4><a id="fps" name="fps"></a>FPS Cap</h4>                        | `Settings` → `Graphics` → `FPS Cap`                   | Controls the maximum frames per second the game will display.                                                                                                                                                                                                              | `0` (Uncapped) | Key: `FpsCap`<br>Values: Positive #s or `0`                                     |
| <h4><a id="af" name="af"></a>Anisotropic Filtering</h4>            | `Settings` → `Graphics` → `Anisotropic Filtering`     | Improves the clarity of textures viewed at oblique angles. Higher levels provide better quality but may reduce performance on older GPUs.                                                                                                                                  | `8`            | Key: `Anisotropy`<br>Values: `1`, `2`, `4`, `8`, `16`                           |
| <h4><a id="sharpness" name="sharpness"></a>Texture Sharpness</h4>  | `Settings` → `Graphics` → `Texture Sharpness`         | Adjusts the sharpness of distant textures. Higher levels may cause shimmering or aliasing on certain textures.                                                                                                                                                             | `10`           | Key: `TextureSharpness`<br>Values: `0` - `20`                                   |

## Performance

| Setting                                               | UI Location                                  | Description                                                                                                                                                                                                              | Default          | Config                                   |
|-------------------------------------------------------|----------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------|------------------------------------------|
| <h4><a id="grass" name="grass"></a>Disable Grass</h4> | `Settings` → `Performance` → `Disable Grass` | The game renders grass quite inefficiently which causes major performance drops. This setting prevents grass patches from rendering, which will bring major performance improvements in areas with high foliage density. | `0` (No changes) | Key: `DisableGrass`<br>Values: `0` / `1` |