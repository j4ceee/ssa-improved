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
- Linux support for the Portal of Power
- WinUSB support for the Portal of Power on Windows (the game can now use the same drivers as emulators like Cemu, RPCS3, ...)
- Windowed & borderless windowed mode on Windows
- Custom resolution support
- VSync toggle
- FPS cap
- Supersampling (up to 2x) (renders the game at a higher resolution and then downscales it to the display resolution)
- Anisotropic filtering (up to 16x)

## Configuration
- after launching the game with the mod installed for the first time, a `ssa_impr_mod.ini` file will be generated in the game directory
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
7. add _Skylanders Spyro's Adventure_ to Steam
   - Steam Deck: right-click on `Skylanders.exe` in the file manager → `Add to Steam`
8. use `Proton 10.0-4` as the compatibility tool for the game (or any other, the mentioned version is confirmed working)
9. add `./portal_launch.sh %command%` to your launch options for _Skylanders Spyro's Adventure_ in Steam
10. Done🎉 You can now launch the game from Steam

## Credits
- Developed by [jacee](https://github.com/j4ceee)

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