# Waveforge

Waveforge is the game I'm developing for [GitHub Game Off 2025](https://itch.io/jam/game-off-2025).

## The concept

Waveforge is a 2D platformer where the player needs to manipulate water waves (and other physical phenomena) to somehow move a rubber duck to the goal in a pixel emulated world with physics simulation (of it's own style, not always realistic though).

## Current status

The codebase for now is a prove of concept. The physics simulation is basic but functional, and the wave manipulation is not fully implemented yet. Launching the game you will get a sandbox to play around with the physics simulation.

- `1`/`2`/`3` keys: change to brush to spawn sand, water, and stone wall pixels
- Left mouse button: use the brush
- Right mouse button: erase pixels
- Scroll wheel: change brush size

The duck is interactable with the physics simulation now, a goal (for testing purpose) is added at the top right corner.

Four music tracks are created and added to the repository, but only "Pixelated Paradise-X" is loaded as background music currently.

## Build

You need to have CMake and a C++23 compatible compiler installed.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo .
cmake --build build --config RelWithDebInfo
```

The dependencies will be automatically downloaded and built. Find the executable in `build` directory.

## Implementation notes

This is quite a straightforward C++ project with simple structure:

- `include/wforge`: header files
- `src`: source files
- `assets`: asset files including music and sound effects
- `CMakeLists.txt`: CMake build script (all in one file for simplicity)

The physics simulation is written from scratch. We use SFML for graphics rendering and audio playback. [proxy](https://github.com/microsoft/proxy) is used to for polymorphism (i.e. fat pointers) instead of traditional virtual functions for simplier and unified lifetime management.

The physics simulation is inspired by Noita's falling everything engine (but we are doing somewhat better at fluid simulation here), which is basically a cellular automaton with some rules for different pixel classes. Performance is not optimal yet, but it's acceptable for now (in Release mode).

The current `main.cpp` (mainly used for testing) is quite messy (partly because it's mostly written by Github Copilot). Instead of drawing bunches of rectangles for each pixel, the rendering system dynamically create a texture representing the pixel world and render it all at once for better performance.

## Team Members

- Project Management & Programming: [fang_erj](https://github.com/szdytom)
- Music: stevvven

Look for more team members in the future! Contact me with email szdytom[at]qq.com  or discord @fang_erj if you are interested.
