<p align="center">
  <img width="50%" align="center" src="https://jacee.dev/img/mods/ssa_improved.png" alt="">
</p>
  <h1 align="center">
  Skylanders Spyro's Adventure Improved
</h1>
<p align="center">
    Comprehensive improvement mod for Skylanders: Spyro's Adventure - emulated portal, graphics enhancements, and gameplay tweaks
</p>
<br>

> [!IMPORTANT]
> This mod modifies game memory and may lead to instability or crashes. 
> Use at your own risk!
> 
> Disclaimer: For some features I relied heavily on AI, since I do not yet have the necessary knowledge to implement those myself.

## Features

### General
- In-game mod UI to easily change mod settings & operate the emulated portal (if enabled)
  - press `F1` on your keyboard or `L3` + `R3` on a controller (both sticks pressed simultaneously) to open the mod UI
- Emulated Portal of Power support (allows you to use Skylanders without a physical portal)
  - support for Skylanders files from emulators
  - Skylander Creator to directly create new Skylanders from within the UI
  - directly usable Sidekicks, Magic Items & Adventure Packs
  - for more info see [EMULATED_PORTAL.md](https://github.com/j4ceee/ssa-improved/blob/main/EMULATED_PORTAL.md)
- Linux support for the Portal of Power
- WinUSB support for the Portal of Power on Windows (the game can now use the same drivers as emulators like Cemu, RPCS3, ...)

### Gameplay
- HP & Damage multipliers for enemies to adjust difficulty
- Multiplier for experience gained from defeating enemies
- Per-player controls featuring god mode, health modifications, faction swapping and more
- Setting to load into any level at any time
- Reusable Magic Items & infinite duration for Magic Items
- Free cam

### Graphics
- Windowed & borderless windowed mode on Windows
- Custom resolution support
- VSync toggle
- FPS cap
- Supersampling (up to 4x) (renders the game at a higher resolution and then downscales it to the display resolution)
- Anisotropic filtering (up to 16x)
- Texture sharpness control
- Grass rendering toggle (disabling grass rendering brings major performance improvements in areas with high foliage density)

### Texture mods
- Custom textures can be loaded from the `ssa-improved/textures` folder
- For instructions on how to create custom textures, see the [Usage](#create-tex) section below

## <a id="config" name="config"></a>Configuration
- there are 2 ways to configure the mod settings:
  1. through the in-game UI
     - press `F1` on your keyboard or `L3` + `R3` on a controller (both sticks pressed simultaneously) to open the mod UI
     - in the UI you can change the settings and they will automatically be saved to the `ssa_impr_mod.ini` file (some settings require a game restart to take effect, these will be indicated in the UI)
  2. by editing the `ssa_impr_mod.ini` file directly with a text editor
     - after launching the game with the mod installed for the first time, a folder called `ssa-improved` will be created in your game directory (where `Skylanders.exe` is located)
     - inside this folder you will find a `ssa_impr_mod.ini` file
     - open this file with a text editor to configure the mod settings
- for more detailed explanations of what each setting does see [CONFIG.md](https://github.com/j4ceee/ssa-improved/blob/main/CONFIG.md)

## Installation

### SecuROMLoader
_While technically not required for this mod, it is recommended to download the latest version of [SecuROMLoader](https://github.com/nckstwrt/SecuROMLoader)._
1. Download the latest release from the [Releases](https://github.com/nckstwrt/SecuROMLoader/releases/latest)
2. Extract the contents of the ZIP file
3. Move **only** the `version.dll` file from the extracted folder to your _Skylanders Spyro's Adventure_ installation directory (where `Skylanders.exe` is located)
   - e.g. `C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)`

### Windows
1. Ensure you have the latest [Visual C++ Redistributable](https://aka.ms/vc14/vc_redist.x86.exe) installed on your system
2. Download the latest release from the [Releases](https://github.com/j4ceee/ssa-improved/releases/latest)
3. Extract the contents of the ZIP file
4. You will find a folder named `SSA Improved` containing 2 folders (`Windows` & `Linux`), this `README.md` and a license file
5. Open the `Windows` folder. You will find the following files:
   - `xinput1_3.dll`
   - `version.json`
6. Move all the files mentioned above to your _Skylanders Spyro's Adventure_ installation directory (where `Skylanders.exe` is located)
   - e.g. `C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)`
7. Done🎉 Run the game as administrator at least once to allow the mod to create necessary files and folders

### Linux / Steam Deck
_Note: you need to have Python installed on your system (most Linux distributions come with Python pre-installed)_
1. Download the latest release from the [Releases](https://github.com/j4ceee/ssa-improved/releases/latest)
2. Extract the contents of the ZIP file
3. You will find a folder named `SSA Improved` containing 2 folders (`Windows` & `Linux`), this `README.md` and a license file
4. Open the `Linux` folder. You will find the following files:
   - `xinput1_3.dll`
   - `portal_launch.sh`
   - `portal_proxy.py`
   - `version.json`
5. Move all the files mentioned above to your _Skylanders Spyro's Adventure_ installation directory (where `Skylanders.exe` is located)
6. Inside your _Skylanders Spyro's Adventure_ installation directory make `portal_launch.sh` executable 
   - Steam Deck: right-click the file in the file manager → `Properties` → `Permissions` tab → check `Allow executing file as program` → click `OK`
7. Add _Skylanders Spyro's Adventure_ to Steam
   - Steam Deck: right-click on `Skylanders.exe` in the file manager → `Add to Steam`
8. Use `Proton 10.0-4` as the compatibility tool for the game (or any other, the mentioned version is confirmed working)
9. Add `./portal_launch.sh %command%` to your launch options for _Skylanders Spyro's Adventure_ in Steam
10. Done🎉 You can now launch the game from Steam

## Usage

### Emulated Portal of Power
- see [EMULATED_PORTAL.md](https://github.com/j4ceee/ssa-improved/blob/main/EMULATED_PORTAL.md) for detailed instructions on how to use the emulated portal and create custom Skylanders for it

### <a id="create-tex" name="create-tex"></a>Creating texture mods
1. Enable texture dumping in the config file or UI to dump in-game textures to the `ssa-improved/dumps/textures` folder, these can be used as a base for creating custom textures
    - Dumped textures follow the naming convention `<HASH>_<W>x<H>_<FMT>.dds`
2. Open the dumped texture you want to replace / edit in an image editing software (e.g. [GIMP](https://www.gimp.org/))
3. Edit the texture as you like, then export it as a `.dds` file named `<HASH>.dds` (e.g. if the dumped texture is `ABC123_512x512_DXT5.dds`, name your file `ABC123.dds`) and place it in the `ssa-improved/textures/` folder
   - **Resolution:** Replacements can be any resolution, but should have the same aspect ratio as the original (otherwise they will appear stretched), and both dimensions **must be multiples of 4** (power-of-two recommended, e.g. 256x256, 512x512, but something like 512x256 also works)
   - **Format:** If you're unsure, keep the same compression format as the original (`DXT1`, `DXT5`, etc.). If you want to learn more about these texture formats and which one is the best fit for you, see [this guide](https://episims.tumblr.com/post/743688032850493440/differences-between-dxt1-dxt3-dxt5) ([archived](https://web.archive.org/web/20260213162846/https://episims.tumblr.com/post/743688032850493440/differences-between-dxt1-dxt3-dxt5))
   - **Mipmaps:** These are reduced-size copies of your texture used for distant objects to improve performance and reduce visual artefacts. If your replaced texture will be used for distant objects, it's recommended to generate mipmaps when exporting your texture (in practice: generate mipmaps for everything except UI textures)
   - Texture mods can be grouped in subfolders inside the `ssa-improved/textures`
4. In the in-game UI, click *Reload Textures* to see your changes immediately without restarting the game

## Credits
- Developed by [jacee](https://github.com/j4ceee)
- Portal Emulation based on...
  - [Cemu](https://github.com/cemu-project/Cemu)
  - [RPCS3](https://github.com/RPCS3/rpcs3)
- UI created with...
  - [ImGui](https://github.com/ocornut/imgui)
  - Icons by Toys for Bob
  - [Varela Round](https://fonts.google.com/specimen/Varela+Round) font by The Varela Round Project Authors (SIL Open Font License, 1.1)
  - [Material Design Icons](https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.ttf) by Google (Apache License 2.0)
  - [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) by Juliette Foucaut and Doug Binks (zlib License)

## Building

### Cloning the Repository
```
git clone --recurse-submodules https://github.com/j4ceee/ssa-improved.git
cd ssa-improved
```

### Building with CLion
1. Open the project folder in CLion
2. CLion will automatically detect the CMake configuration and set up the project
3. Configure your CMake profile (if needed):
   - Build type: `Release` (or `Debug` for development)
   - Toolchain: `Visual Studio`
   - Generator: `Visual Studio 17 2022`
4. In the toolbar, select your CMake profile (e.g., `Release-Visual Studio`)
5. Select the build target `xinput1_3`
6. Click the Build button
7. The compiled DLL will be located in `cmake-build-<config>/out/<config>/xinput1_3.dll` and copied to the _Skylanders Spyro's Adventure_ directory as specified in the `CMakeLists.txt`

**Build Types:**
- **Debug**: Includes extra logging and debugging symbols (larger file size, slower)
- **Release**: Optimized build for normal use (smaller, faster)

**Note:** The CMakeLists.txt includes a post-build command that copies the DLL to:
```
C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)/
```
If your game is installed elsewhere, modify line 134 in `CMakeLists.txt` accordingly.


### Building with Command Line (CMake)
1. Create a build directory
    ```
    mkdir build
    ```
2. Configure for Release build
    ```
    cmake -DCMAKE_BUILD_TYPE=Release -B .\build\ -G "Visual Studio 17 2022"
    ```
   ...or for Debug build
    ```
    cmake -DCMAKE_BUILD_TYPE=Debug -B .\build\ -G "Visual Studio 17 2022"
    ```
3. Build
    ```
    cmake --build build --config Release
    ```

The compiled DLL will be in `build/out/Release/` or `build/out/Debug/`
