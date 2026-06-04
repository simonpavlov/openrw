# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

OpenRW is a cross-platform, open-source re-implementation of the Grand Theft Auto III game engine in C++17. It does not ship game content: a legitimate copy of the original PC game data is required to actually run the game. Most code paths and the unit tests work without game data; the data-dependent tests are gated behind a CMake option (see below).

## Build

The project uses CMake (≥3.8). Dependencies are expected to be provided by the system (Boost, Bullet, GLM, FFmpeg, SDL2, OpenAL, OpenGL; Qt5 + Freetype additionally for the viewer/tools), or installed via Conan with `-DUSE_CONAN=ON`. Submodules (`imgui`, `microprofile`) must be checked out — clone with `--recursive` or run `git submodule update --init --recursive`.

```bash
# Configure + build (Ninja recommended; Release is the default build type)
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON -DBUILD_TOOLS=ON -DBUILD_VIEWER=ON
cmake --build build
```

Key CMake options (defined in `cmake_options.cmake`): `BUILD_TESTS`, `BUILD_TOOLS`, `BUILD_VIEWER`, `TEST_DATA` (enable tests needing real game data), `ENABLE_SANITIZERS` (`address`/`leak`/`thread`/`undefined`), `TEST_COVERAGE`, `ENABLE_SCRIPT_DEBUG`, `USE_CONAN`, `SEPARATE_TEST_SUITES`.

The canonical CI build is driven by CTest scripts, not raw cmake — this is what GitHub Actions runs and the surest way to reproduce CI locally (configures with sanitizers, builds, and runs tests in one step):

```bash
mkdir build && cd build && ctest -VV -S ../cmake/ctest/script_ci.ctest
```

## Tests

Tests use the Boost.Test framework. All suites compile into a single `rwtests` executable; the per-suite list lives in `tests/CMakeLists.txt` (add a new `test_Foo.cpp` to the `TESTS` list there to register it).

```bash
ctest --test-dir build                      # run via CTest
./build/tests/rwtests                       # run the whole binary directly
./build/tests/rwtests -t VehicleTests       # run one suite (suite name = <Name>Tests)
./build/tests/rwtests --run_test=@data-test # run only data-dependent tests (needs TEST_DATA + game data)
```

Configure with `-DSEPARATE_TEST_SUITES=ON` to expose each suite as an individual CTest entry instead of one `UnitTests` entry.

## Running the game

The `rwgame` executable needs a path to the original GTA III install. It can be passed on the command line or set in the config file (`game.path`). Options are declared once in `rwgame/RWConfig.inc` and generated into both the CLI parser and the config-file reader.

```bash
./build/rwgame/rwgame --gamedata /path/to/gta3   # set data path
./build/rwgame/rwgame --newgame                  # start a new game
./build/rwgame/rwgame --test                      # start in a test location (dev)
./build/rwgame/rwgame --help                      # full option list
```

## Code style

C++17, formatted with clang-format using the config in `.clang-format` (Google base, 4-space indent, 80-column limit, `-Wold-style-cast` enforced via warnings). CI builds with `-Wall -Wextra -Wpedantic` and treats them strictly. `scripts/verify-commit` checks a diff against clang-format and can be symlinked as a git `pre-commit` hook:

```bash
ln -s ../../scripts/verify-commit .git/hooks/pre-commit
scripts/verify-commit HEAD~1 HEAD   # or check staged changes with no args
```

## Architecture

The codebase is layered into four libraries plus optional tools, each its own CMake subdirectory. The dependency direction is strictly upward: `rwcore` → `rwengine` → `rwgame`.

- **`rwcore/`** — foundation layer with no game logic. File-format loaders for the RenderWare/GTA formats (`LoaderDFF` models, `LoaderTXD` textures, `LoaderIMG` archives, `LoaderSDT` sound, `RWBinaryStream`), the OpenGL abstraction (`gl/`), platform/filesystem code (`platform/`), fonts, and base types/debug macros (`rw/`, including the `RW_CHECK` assertion macros whose behavior is controlled by the `FAILED_CHECK_ACTION` CMake option).

- **`rwengine/`** — the actual game engine (built as `librwengine`). Important seams:
  - `engine/GameWorld` is the central hub holding all live objects, the physics world, and game state; `engine/GameData` owns loaded asset data; `engine/GameState` is the serializable game state.
  - `objects/` holds the runtime object hierarchy — `GameObject` is the base, with `CharacterObject`, `VehicleObject`, `InstanceObject`, `PickupObject`, `ProjectileObject`, `CutsceneObject` subclasses.
  - `data/` holds parsed *type/definition* data (model definitions, zones, weapons, AI paths, weather) as distinct from the live `objects/`.
  - `script/` is the SCM bytecode virtual machine: `ScriptMachine` executes `SCMFile` bytecode; the opcode implementations live in `script/modules/` (`GTA3Module`). This is how the original game's mission logic runs.
  - `ai/` (character/traffic AI), `dynamics/` (Bullet physics integration), `render/` (the GL renderer built on `rwcore/gl`), `audio/`, `items/`.

- **`rwgame/`** — the player-facing executable (`librwgame` + `rwgame`). Owns the window/input/SDL loop, the HUD, ImGui debug UI, config (`RWConfig` + the generated `RWConfig.inc`), and a **state-machine frontend**: `StateManager` runs a stack of `State`s (`LoadingState`, `MenuState`, `IngameState`, `PauseState`, `DebugState`, `BenchmarkState`) found in `rwgame/states/`. The unit tests link against `librwgame`, so test code can reach the full stack.

- **`rwviewer/`** (Qt5 GUI for inspecting archives/DFF models) and **`rwtools/`** (e.g. `rwfont`) are optional, gated behind `BUILD_VIEWER` / `BUILD_TOOLS`.

`external/` vendors `imgui` and `microprofile` as git submodules. Compiler flags, platform defines (`RW_LINUX`/`RW_WINDOWS`/etc.), and the shared `openrw_target_apply_options()` helper that every target uses for warnings/IWYU/clang-tidy/coverage are all centralized in `cmake_configure.cmake`.
