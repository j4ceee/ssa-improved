<p align="center">
  <img width="50%" align="center" src="https://jacee.dev/img/mods/ssa_improved.png" alt="">
</p>
  <h1 align="center">
  Skylanders Spyro's Adventure Improved
</h1>
<p align="center">
  A mod for Skylanders Spyro's Adventure on PC that adds better support for the game on Linux and Windows
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
- Emulated Portal of Power support (allows you to use Skylanders without a physical portal, for more info see [EMULATED_PORTAL.md](https://github.com/j4ceee/ssa-improved/blob/main/EMULATED_PORTAL.md)
- Linux support for the Portal of Power
- WinUSB support for the Portal of Power on Windows (the game can now use the same drivers as emulators like Cemu, RPCS3, ...)

### Graphics
- Windowed & borderless windowed mode on Windows
- Custom resolution support
- VSync toggle
- FPS cap
- Supersampling (up to 2x) (renders the game at a higher resolution and then downscales it to the display resolution)
- Anisotropic filtering (up to 16x)
- Texture sharpness control
- Grass rendering toggle (disabling grass rendering brings major performance improvements in areas with high foliage density)

### Texture mods
- Custom textures can be loaded from the `ssa-improved/textures` folder
- For instructions on how to create custom textures, see the [Usage](#create-tex) section below

## Configuration
- after launching the game with the mod installed for the first time, a folder called `ssa-improved` will be created in your game directory (where `Skylanders.exe` is located)
- inside this folder you will find a `ssa_impr_mod.ini` file
- open this file with a text editor to configure the mod settings

### Available settings
| Setting            | Default | Description                                                                                                                                                                                                                                                                                             |
|--------------------|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Windowed`         | `1`     | set to `1` to launch the game in windowed mode, or `0` for fullscreen                                                                                                                                                                                                                                   |
| `Borderless`       | `1`     | set to `1` to launch the game in borderless windowed mode, or `0` for normal windowed mode (requires `Windowed=1`)                                                                                                                                                                                      |
| `ResolutionW`      | `0`     | set the horizontal resolution of the game, set to `0` to use the desktop resolution                                                                                                                                                                                                                     |
| `ResolutionH`      | `0`     | set the vertical resolution of the game, set to `0` to use the desktop resolution                                                                                                                                                                                                                       |
| `VSync`            | `1`     | set to `1` to enable VSync, or `0` to disable it                                                                                                                                                                                                                                                        |
| `FpsCap`           | `0`     | set the maximum FPS the game can run at, set to `0` for unlimited FPS                                                                                                                                                                                                                                   |
| `RenderRes`        | `1`     | allows the game to render at higher resolutions internally (set to `0` to disable)                                                                                                                                                                                                                      |
| `Supersampling`    | `1.0`   | Multiplies the internal render resolution for improved image quality and scales the image down to your chosen / desktop resolution (requires `RenderRes=1`)<br>E.g. at `1920x1080` desktop resolution, `Supersampling=2.0` renders at `3840x2160`, meanwhile `Supersampling=1.0` renders at `1920x1080` |
| `Anisotropy`       | `8`     | set the level of anisotropic filtering (valid values: `1` (off), `2`, `4`, `8`, `16`)                                                                                                                                                                                                                   |
| `TextureSharpness` | `10`    | set the sharpness of textures (valid values: `0` (off) to `20` (max))                                                                                                                                                                                                                                   |
| `DisableGrass`     | `0`     | Disabling grass rendering (value = `1`) brings major performance improvements in areas with high foliage density                                                                                                                                                                                        |
| `EmulatedPortal`   | `0`     | Use a fully emulated portal instead of a physical USB device. When enabled (value = `1`) physical portals are ignored and Skylanders can be chosen over the in-game mod UI.                                                                                                                             |
| `FontScale`        | `1.0`   | Scale of the font of the in-game UI (`1.0` = default size, `2.0` = double size, etc.)                                                                                                                                                                                                                   |
| `TextureMods`      | `1`     | Set to `1` to enable loading of custom textures from the `ssa-improved/textures` folder, or `0` to disable it.                                                                                                                                                                                          |
| `TextureDump`      | `0`     | Set to `1` to enable dumping of in-game textures to the `ssa-improved/dumps` folder, or `0` to disable it. Useful for modders for creating custom textures.                                                                                                                                             |

## Installation

### SecuROMLoader
_While technically not required for this mod, it is recommended to download the latest version of [SecuROMLoader](https://github.com/nckstwrt/SecuROMLoader)._
1. Download the latest release from the [Releases](https://github.com/nckstwrt/SecuROMLoader/releases/latest)
2. Extract the contents of the ZIP file
3. Move **only** the `version.dll` file from the extracted folder to your _Skylanders Spyro's Adventure_ installation directory (where `Skylanders.exe` is located)
   - e.g. `C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)`

### Windows
1. Download the latest release from the [Releases](https://github.com/j4ceee/ssa-improved/releases/latest)
2. Extract the contents of the ZIP file
3. You will find a folder named `SSA Improved` containing 2 folders (`Windows` & `Linux`), this `README.md` and a license file
4. Open the `Windows` folder. You will find the following files:
   - `xinput1_3.dll`
   - `version.json`
5. Move all the files mentioned above to your _Skylanders Spyro's Adventure_ installation directory (where `Skylanders.exe` is located)
   - e.g. `C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)`
6. Done🎉 Run the game as administrator at least once to allow the mod to create necessary files and folders

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
3. Edit the texture as you like, then save it as a `.dds` file with the same hash as the original dumped texture (e.g. if the dumped texture is named `abc123_512x512_DXT5.dds`, name your edited texture `abc123.dds`) and place it in the `ssa-improved/textures` folder
   - Files in the `ssa-improved/textures` folder should be named `<HASH>.dds`
   - Replaced textures can be of a higher or lower resolution than the original but must have the same aspect ratio. If unsure, also keep the original format (`FMT`) (e.g. `DXT1`, `DXT5`, etc.) and generate mipmaps
   - Texture mods can be grouped in subfolders inside the `ssa-improved/textures`
- In the in-game UI you can trigger a hot reload of textures to see your changes immediately in-game without needing to restart the game

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