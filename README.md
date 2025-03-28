# 0verknigh7  [![C/C++ CI](https://github.com/su8/0verknigh7/actions/workflows/c-cpp.yml/badge.svg?branch=main)](https://github.com/su8/0verknigh7/actions/workflows/c-cpp.yml)
Small Roguelike c++ game that I found on reddit - https://www.reddit.com/r/roguelikedev/comments/kxannr/quest_for_the_orb_a_minimal_roguelike_in_317/ , hats off to [@aotdev](https://www.reddit.com/user/aotdev/) who shared the game and I forked it here

![](1snap.png)

### Features:

- [x] Turn-based
- [x] Permadeath
- [x] Infinite procedurally generated dungeon
- [x] Victory condition: retrieve the orb! Legend says it lies somewhere between levels 16-25
- [x] Explore! Walking to a wall or pressing random keys skips a turn
- [x] Collect treasure: just walk over it
- [x] Avoid the rotating disc-blades: They move always towards a random direction except the one they came from, and they bounce off dead-ends
- [x] Destroy neighbouring walls and disc-blades with dynamite. But be careful, you have only 3!
- [x] Find the stairs to the next level, hoping to find the orb there
- [x] You can walk off the edge of the map and come out the other side, if there is a floor there
- [x] Input configuration: map your own keys. Don't use arrow keys on repl.it though, as they misbehave.
- [x] Increasing difficulty: there are more disc-blades deeper into the dungeon
- [x] Welcome screen in glorious ASCII HD
- [x] Victory screen in glorious ASCII HD with derived score from turn count, treasure and dynamites left
- [x] UI showing step count, treasure collected, level and dynamites left

### What my fork has added

- [x] Collect per level additional `life(s)` and `shield(s)`
- [x] In-game colour
- [x] Fixed a bug when 2 enemies meet each other as they swallow the other one
- [x] Windows OS cmd terminal is not flickering anymore
- [x] Added a .bat script for the Windows users, so they can start the game in cmd terminal
- [x] Removed `std` globally
- [x] Uses better randomization mechanism for the maps

---

# Compile

```make
make -j8 # to use 8 cores/threads in parallel compile
sudo make install
```
Now to run the game type `0verknigh7`.

---

## Windows users

Tested with [Visual Studio Code Editor](https://code.visualstudio.com/download), but you need to install [MingW](https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev0/x86_64-12.2.0-release-posix-seh-rt_v10-rev0.7z), once downloaded extract it to **C:\MingW**, then re-open [Visual Studio Code Editor](https://code.visualstudio.com/download), you might want to install C\C++ extensions if you plan to write C\C++ code with the editor. If you plan to contribute to this project go to **File->Preferences->Settings** and type to search for **cppStandard** and set it to c17 to both C++ and C.

I use **One Monokai** theme for the [VScode Editor](https://code.visualstudio.com/download)

In [Visual Studio Code Editor](https://code.visualstudio.com/download), go to **Terminal->Configure Tasks...->Create tasks.json from template** and copy and paste this into it:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
        "type": "cppbuild",
        "label": "C/C++",
        "command": "C:\\MingW\\bin\\g++.exe",
        "args": [
            "-fdiagnostics-color=always",
            "-std=c++17",
            "-ggdb",
            "-lpthread",
            "-Wall",
            "-Wextra",
            "-O2",
            "-pipe",
            "-pedantic",
            "-Wundef",
            "-Wshadow",
            "-W",
            "-Wwrite-strings",
            "-Wcast-align",
            "-Wstrict-overflow=5",
            "-Wconversion",
            "-Wpointer-arith",
            "-Wformat=2",
            "-Wsign-compare",
            "-Wendif-labels",
            "-Wredundant-decls",
            "-Winit-self",
            "${file}",
            "-o",
            "${fileDirname}/${fileBasenameNoExtension}"
        ],
        "options": {
            "cwd": "C:\\MingW\\bin"
        },
        "problemMatcher": [
            "$gcc"
        ],
        "group": {
            "kind": "build",
            "isDefault": true
        },
        "detail": "compiler: C:\\MingW\\bin\\g++.exe"
    }
]
}
```

### To compile the game press **CTRL** + **SHIFT** + **B** , then from the same folder start the `0verknigh7.bat` script.

Optioanlly if you want to play the game from VSCode's console -- wait until it compiles, after that press **CTRL** + **\`** and paste this `cp -r C:\Users\YOUR_USERNAME_GOES_HERE\Desktop\main.exe C:\MingW\bin;cd C:\MingW\bin;.\main.exe`

---

## Uninstall

```make
sudo make uninstall
```
