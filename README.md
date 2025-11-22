# Waveforge

Waveforge is the game I'm developing for [GitHub Game Off 2025](https://itch.io/jam/game-off-2025).

## The concept

Waveforge is a 2D platformer where the player needs to manipulate water waves (and other physical phenomena) to somehow move a rubber duck to the checkpoint in a pixel emulated world with physics simulation (of it's own style, not always realistic though).

For more details about the game physics system, please refer to [Physics System Documentation](docs/physics-system.md).

## Current status

Three demo levels are created and can be played. The settings screen is not implemented yet. The UI for main menu and level complete screens are placeholders. The new art style is being worked on.

### Controls

| Key		 | Action                     |
|--------------|----------------------------|
| Arrow Keys/WASD   | UI navigation |
| Enter/Space  | UI selection               |
| ESC		  | Back |
| R		  | Retry current level       |
| LMB		  | Use current item			|
| Mouse Wheel | Change item brush size		|
| Arrow Up/Down / W S | Change current item		|

## Build instructions

You need to have CMake and a C++23 compatible compiler installed (e.g. GCC 14, Clang 20, MSVC 19.44.35219.0). Then run the following commands in the project root directory:

```bash
# Make sure to enter MSVC environment when on Windows
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo .
cmake --build build --config RelWithDebInfo
```

The direct dependencies will be automatically downloaded and built. Find the executable in `build` directory. The built program can be found at `build/waveforge` (or `build/waveforge.exe` on Windows).

For Linux systems, SFML might requires some additional system libraries. The simplest way is to install SFML via your package manager, so that all those internal dependencies are automatically handled. For example:

```bash
# Debian/Ubuntu
sudo apt install libsfml-dev

# Arch Linux and dirivatives
sudo pacman -S sfml
```

You can also install those dependencies manually if you prefer not to install SFML system-wide. Please refer to SFML's official documentation for more details.

## Implementation notes

This is quite a straightforward C++ project with simple structure:

- `include/wforge`: header files
- `src`: source files
- `assets`: asset files, levels, UI configuration, music, sound effects, textures, etc.
- `CMakeLists.txt`: CMake build script (all in one file for simplicity)

The physics simulation is written from scratch. We use SFML for graphics rendering and audio playback. [proxy](https://github.com/microsoft/proxy) is used to for polymorphism (i.e. fat pointers) instead of traditional virtual functions for simplier and unified lifetime management.

The physics simulation is inspired by Noita's falling everything engine (but we are doing somewhat better at fluid simulation here), which is basically a cellular automaton with some rules for different pixel classes. Performance is not optimal yet, but it's acceptable for now (in Release mode).

The fluid simulation process is devided into two phases: the global update phase and the local update phase. In the global update phase, we abstract the fluid pixels into fluid blocks and build a graph structure representing the connectivity between those blocks. Then the network flow algorithm (Dinic implementation) is used to calculate the fluid distribution among those blocks. In the local update phase, some classic cellular automaton rules are applied to each fluid pixel to simulate local interactions (e.g. water flowing downwards due to gravity). The global update phase is implemented in `fluidflow.cpp` and the local update phase is implemented in `fluids.cpp`.

A thermal simulation system is also implemented, allowing pixels to exchange heat with adjacent pixels and change state when certain temperature thresholds are reached (e.g. oil igniting when heated enough). The heat exchange process is simple (linearly averaging respesting to thermal conductivity as weight), but it works fine for our purpose. The thermal simulation is implemented in `thermal.cpp`.

Pixels can form structures. The structure is stored separately from the pixel 2D array, and each structure can span multiple pixels. The texture of the structure is loaded from an image asset. The map is loaded from a prototype image, where each color represents a different pixel type or structure, more details can be found in [Level Format Documentation](assets/levels/README.md).

The game scene management is implemented as a finite state machine, where each scene is a state. The UI for some scenes (e.g. main menu, level menu, settings) are partially data-driven and can be configured via JSON files in `assets/ui`.

## Team Members

- Code: [fang_erj](https://github.com/szdytom)
- Music & SFX: [stevvven](https://github.com/Stevvven777)
- Pixel Art: [ZTL-UwU](https://github.com/ZTL-UwU/)
- Game Play: zurry

## Acknowledgements

We would also like to thank the following open source projects for making this game possible:

- [microsoft/proxy](https://github.com/microsoft/proxy): A C++ library for "Next Generation Polymorphism"

- [SFML](https://www.sfml-dev.org/): Simple and Fast Multimedia Library for graphics and audio.

- [cpptrace](https://github.com/jeremy-rifkin/cpptrace): A simple & self-contained C++ stack trace library.

- [Aseprite](https://www.aseprite.org/): A pixel art tool used for creating pixel art assets.

Additionally, we would like to acknowledge GitHub and [Lee Reilly](https://leereilly.net/) for organizing the [GitHub Game Off 2025](https://itch.io/jam/game-off-2025) game jam, which provided the inspiration and motivation for this project.
