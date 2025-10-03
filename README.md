# cron-cpp

## WARNING!: Still in Development expect changes

This Library is a C++20 / C++23 knockoff of [libcron](https://github.com/PerMalmberg/libcron). 

## Differences from libcron

- Proper CMake support
- date lib replaced with std::chrono implementation
- TimeZoneClock
- Automated test workflows

## Build Locally

```bash
cmake -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -Bbuild -H.
      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      Only needed for clangd   
```

```bash
cmake --build build -j32
```

## Adding this library to your project

```cmake
include(FetchContent)

FetchContent_Declare(
    chron-cpp
    GIT_REPOSITORY https://github.com/BestITUserEUW/chron-cpp.git
    GIT_TAG main
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(chron-cpp)

find_package(chron-cpp REQUIRED)

target_link_libraries(my_project PUBLIC
    oryx::chron-cpp
)
```

## IDE Setup VsCode

### Clangd Extension (Recommended)

- Install clangd language server `apt install clangd`
- Go to Extension and install `llvm-vs-code-extensions.vscode-clangd`