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

The duck is interactable with the physics simulation now, but there is no goal or level design yet.

Four music tracks are created and added to the repository, but only "Pixelated Paradise-X" is loaded as background music currently.

## Build

You need to have CMake and a C++23 compatible compiler installed.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo .
cmake --build build --config RelWithDebInfo
```

The dependencies will be automatically downloaded and built. Find the executable in `build` directory.

## Team

- Project Management & Programming: [fang_erj](https://github.com/szdytom)
- Music: stevvven

Look for more team members in the future! Contact me with email szdytom[at]qq.com if you are interested.
