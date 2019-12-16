# Building Dromaius

## Requirements
- a (mostly) C11 compatible compiler
- CMake (version 3.12 or newer)
- (optional) Emscripten to build the UI-application as a web-application
- any major dependencies are included in the project. On Linux X11 and Mesa development packages have to be installed.

## Building
Dromaius uses CMake, all you need to do to build it is the traditional workflow. On Linux it would be something like:
```bash
git clone --recursive https://github.com/JohanSmet/dromaius.git
cd dromaius 
cmake . -B _build -DCMAKE_BUILD_TYPE=Release
cmake --build _build 
```
