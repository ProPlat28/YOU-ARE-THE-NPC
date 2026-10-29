# YOU ARE THE NPC

You are not the hero. You are the person living in the hero's world.

A 3D open-world simulation in C++ (raylib) where you play an ordinary NPC. The city keeps moving. Someone in red does not follow the rules.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Linux packages (Debian/Ubuntu):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config \
  libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev \
  libxi-dev libxext-dev libxfixes-dev libasound2-dev
```

Then run:

```bash
./build/you_are_the_npc
```

GitHub Actions builds Linux and Windows via `.github/workflows/build.yml`. Raylib 5.5 is fetched by CMake.

## Play

- WASD: walk
- Shift: sprint
- Q/E or right mouse: orbit camera
- Wheel: zoom
- F: talk
- G: warn a nearby NPC
- R: report at the police station
- T: help the stranger
- N: try for an ending
- H: help
- Esc: pause

Blue mark is you. Red mark is the player-controlled stranger.

Endings: Normal Life, Investigator, Hunter, Hero, Escape, Reset.
