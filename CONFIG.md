<p align="center">
  <img width="50%" align="center" src="https://jacee.dev/img/mods/ssa_improved.png" alt="">
</p>
<h1 align="center">
  SSA Improved - Configuration
</h1>
<p align="center">
  Adjust the game to your liking
</p>

## Game

### <a id="emulatedportal" name="emulatedportal"></a>Emulated Portal
  - Enables the emulated Portal of Power
  - For a more detailed description see [EMULATED_PORTAL.md](https://github.com/j4ceee/ssa-improved/blob/main/EMULATED_PORTAL.md)
#### Config
  - Key: `EmulatedPortal`
  - Valid values: `0` = disabled (default), `1` = enabled
#### UI
  - Location: `Portal` → `Enable Emulated Portal`

---

### <a id="levelloading" name="levelloading"></a>Level loading
  - Directly load into any challenge or campaign level
#### UI
  - Location: `Game` → `Level`
  - Select a level from the dropdown & hit `Load`

<br>
<br>

## Difficulty

### <a id="hpmult" name="hpmult"></a>Enemy HP multiplier
  - Scales enemy HP
  - Enemies will be tougher to defeat when this value is increased
  - This multiplier is applied to the base HP values of the enemies in the current level
#### Config
  - Key: `HpMult`
  - Valid values: any number between `0.1` and `10.0`, `1.0` = default
#### UI
  - Location: `Game` → `Difficulty` → `Enemy HP multiplier`

---

### <a id="dmgmult" name="dmgmult"></a>Enemy damage multiplier
  - Scales enemy damage
  - Enemies will deal more damage when this value is increased
  - This multiplier is applied to the base damage values of the enemies in the current level
#### Config
  - Key: `DmgMult`
  - Valid values: any number between `0.1` and `10.0`, `1.0` = default
#### UI
  - Location: `Game` → `Difficulty` → `Enemy damage multiplier`

---

### <a id="hitreact" name="hitreact"></a>Enemy hit reaction
  - Enables or disables enemy hit reaction
  - When enabled, enemies will react to being hit by the player (vanilla behaviour)
  - When disabled, enemies will still take damage but will not react to being hit (recommended for higher difficulty)
  - Enemies will attack in faster succession when they don't react to hits
#### Config
  - Key: `EnemyHitReaction`
  - Valid values: `0` = enemies don't react to hits, `1` = enemies react to hits (default)
#### UI
  - Location: `Game` → `Difficulty` → `Enemy hit reaction`

---

### <a id="challengehp" name="challengehp"></a>Heroic Challenge HP ceiling
  - Maximum HP enemies can have in Heroic Challenges
  - If you find that enemies in challenges are too weak, try increasing this and [Enemy HP multiplier](#hpmult)
#### Config
  - Key: `HeroicHpCeiling`
  - Valid values: any number between `0.1` and `10.0`, `3.0` = default
#### UI
  - Location: `Game` → `Difficulty` → `Heroic Challenge HP ceiling`

---

### <a id="challengedmg" name="challengedmg"></a>Heroic Challenge damage ceiling
  - Maximum damage enemies can deal in Heroic Challenges
  - If you find that enemies in challenges are too weak, try increasing this and [Enemy damage multiplier](#dmgmult)
#### Config
  - Key: `HeroicDmgCeiling`
  - Valid values: any number between `0.1` and `10.0`, `0.1` = default
#### UI
  - Location: `Game` → `Difficulty` → `Heroic Challenge damage ceiling`

---

### <a id="xp" name="xp"></a>XP multiplier
  - Multiplies the amount of XP gained from defeating enemies
#### Config
  - Key: `XpMult`
  - Valid values: any number between `0.1` and `10.0`, `1.0` = default
#### UI
  - Location: `Game` → `Experience` → `XP multiplier`

<br>
<br>

## Skylander player settings
  - these settings can be changed in the current play session
  - each player has their own settings

### <a id="hp" name="hp"></a>HP
  - Change the current health of your Skylander
#### UI
  - Location: `Game` → `Skylanders` → `Player 1` / `Player 2` → `HP`
  - Drag the slider to change the HP

---

### <a id="god" name="god"></a>God mode
  - Makes the Skylander immune to all damage
#### UI
  - Location: `Game` → `Skylanders` → `Player 1` / `Player 2` → `God mode`

---

### <a id="nonockback" name="nonockback"></a>No knockback
  - Prevents the Skylander from being knocked back by attacks
#### UI
  - Location: `Game` → `Skylanders` → `Player 1` / `Player 2` → `No knockback`

---

### <a id="nohitreaction" name="nohitreaction"></a>No hit reaction
  - Prevents attack animations from being interrupted by hits
#### UI
  - Location: `Game` → `Skylanders` → `Player 1` / `Player 2` → `No hit reaction`

---

### <a id="team" name="team"></a>Team
  - Changes the faction of the Skylander
  - Members of the same faction cannot attack the Skylander
#### UI
  - Location: `Game` → `Skylanders` → `Player 1` / `Player 2` → `Team`

<br>
<br>

## Mod settings

### <a id="fontscale" name="fontscale"></a>Font Size
  - Adjusts the scale of the mod UI font
#### Config
  - Key: `FontScale`
  - Valid values: any number between `0.5` and `3.0`, `1.0` = default
#### UI
  - Location: `Settings` → `Mod` → `UI Font Scale`

---

### <a id="texturemods" name="texturemods"></a>Texture Mods
  - Enables texture mods
  - Texture mods can be placed into the `ssa-improved/textures/` subfolder and will be loaded from there
#### Config
  - Key: `TextureMods`
  - Valid values: `0` = disabled, `1` = enabled (default)
#### UI
  - Location: `Settings` → `Mod` → `Enable Texture Mods`

---

### <a id="texturedump" name="texturedump"></a>Dump Textures
  - Dumps textures to the `ssa-improved/dumps/textures/` directory as they are loaded in-game
  - Useful for modders to create new texture mods. Want to learn how? [Click here](https://github.com/j4ceee/ssa-improved#create-tex)
  - Recommended: keep disabled (`0`) unless you need it
#### Config
  - Key: `TextureDump`
  - Valid values: `0` = disabled (default), `1` = enabled
#### UI
  - Location: `Settings` → `Mod` → `Dump Textures`


<br>
<br>

## Game window

### <a id="windowed" name="windowed"></a>Windowed Mode
  - controls whether the game is displayed in windowed mode or fullscreen
#### Config
  - Key: `Windowed`
  - Valid values: `0` = fullscreen, `1` = windowed mode (default)
#### UI
  - Location: `Settings` → `Further options` → `Windowed Mode`
  - Note: Requires an immediate game restart

---

### <a id="borderless" name="borderless"></a>Borderless windowed mode
  - hides the Windows title bar & displays the game in borderless windowed mode
  - Required: [Windowed Mode](#windowed) to be active
#### Config
  - Key: `Borderless`
  - Valid values: `0` = normal windowed mode, `1` = borderless windowed mode (default)
#### UI
  - Location: `Settings` → `Further options` → `Borderless`

---

### <a id="resolution" name="resolution"></a>Resolution
  - change the resolution of the game (window)
  - Recommended: keep at `0` to use your desktop resolution
  - Recommended: enable [High-Res Rendering](#renderres)
#### Config
  - Key:
    - `ResolutionW` for screen width
    - `ResolutionH` for screen height
  - Valid values: any (positive) number, `0` = use desktop resolution
#### UI
  - Location: `Settings` → `Further options` → `Resolution`

<br>
<br>

## Graphics

### <a id="renderres" name="renderres"></a>High-Res Rendering
  - The game internally always renders at a resolution of `1120x704`, this causes low image quality and lots of artifacting
  - Enabling this option allows the game to render at higher resolutions internally
#### Config
  - Key: `RenderRes`
  - Valid values: `0` = disabled, `1` = enabled (default)
#### UI
  - Location: `Settings` → `Further options` → `High-Res Rendering`
  - Note: Requires an immediate game restart

---

### <a id="ss" name="ss"></a>Supersampling
  - Renders the scene at a higher internal resolution and downscales
  - Increases image quality & acts as antialiasing
  - Higher values cost more GPU
  - Required: [High-Res Rendering](#renderres) needs to be enabled (`1`)
#### Config
  - Key: `Supersampling`
  - Valid values: `1.0` = disabled (default), `1.5`, `2.0`, `2.5`, `3.0`, `3.5`, `4.0`
#### UI
  - Location: `Settings` → `Further options` → `Supersampling`
  - Note: Requires an immediate game restart

---

### <a id="vsync" name="vsync"></a>VSync
  - Controls whether [Vertical Synchronisation](https://www.howtogeek.com/853225/what-is-vsync-and-should-you-enable-it/) is enabled
#### Config
  - Key: `VSync`
  - Valid values: `0` = disabled, `1` = enabled (default)
#### UI
  - Location: `Settings` → `Further options` → `VSync`
  - Note: Requires an immediate game restart

---

### <a id="fps" name="fps"></a>FPS Cap
  - Controls the maximum frames per second the game will display
#### Config
  - Key: `FpsCap`
  - Valid values: any positive number (e.g., `60` = 60 FPS cap), `0` = uncapped (default)
#### UI
  - Location: `Settings` → `Graphics` → `FPS Cap`

---

### <a id="af" name="af"></a>Anisotropic Filtering
  - Improves the clarity of textures viewed at oblique angles
  - Higher levels provide better quality but may reduce performance on older GPUs
#### Config
  - Key: `Anisotropy`
  - Valid values: `1` = off, `2`, `4`, `8` (default), `16`
#### UI
  - Location: `Settings` → `Graphics` → `Anisotropic Filtering`

---

### <a id="sharpness" name="sharpness"></a>Texture Sharpness
  - Adjusts the sharpness of distant textures
  - Higher levels may cause shimmering or aliasing on certain textures
#### Config
  - Key: `TextureSharpness`
  - Valid values: any number between `0` (off) and `20`, `10` = default
#### UI
  - Location: `Settings` → `Graphics` → `Texture Sharpness`

<br>
<br>

## Performance

### <a id="grass" name="grass"></a>Disable Grass
  - The game renders grass quite inefficiently which causes major performance drops
  - This setting prevents grass patches from rendering, which will bring major performance improvements in areas with high foliage density
#### Config
  - Key: `DisableGrass`
  - Valid values: `1` = grass not rendered, `0` = no changes (default)
#### UI
  - Location: `Settings` → `Performance` → `Disable Grass`