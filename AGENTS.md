Some helpful notes for our AI friends (and some humans may find this useful as well)

# Building and Testing

> **Important:** Always execute `bitfighter_test` from the `<ROOT_DIR>/exe` directory so resource paths and levels resolve properly.

## macOS

### Prerequisites (Homebrew)
On Apple Silicon macOS, LuaJIT 2.1 from Homebrew is required:
```bash
brew install luajit sdl2 libpng libvorbis libogg speex libmodplug openal-soft
```

### Build & Test Commands:
```bash
# Configure out-of-source build (use -DMASTER_MINIMAL=ON if MySQL server is not used)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMASTER_MINIMAL=ON

# Build test runner
cmake --build build --target bitfighter_test -j$(sysctl -n hw.ncpu)

# Build game client
cmake --build build --target Bitfighter -j$(sysctl -n hw.ncpu)

# Run test suite
cd exe && ./bitfighter_test

# Run filtered tests (e.g. ChatHelper or StringUtils)
cd exe && ./bitfighter_test --gtest_filter=ChatHelperTest.*
```

## Linux / WSL

### Build & Test Commands:
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMASTER_MINIMAL=ON

# Build
cmake --build build --target bitfighter_test -j$(nproc)

# Run tests
cd exe && ./bitfighter_test
```

### Running Valgrind on Linux / WSL:
```bash
# Build with debug symbols and no optimization
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -Wno-error" -DMASTER_MINIMAL=ON
cmake --build build --target bitfighter_test -j1

# Run Valgrind
cd exe
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=../lua/luajit/src/lj.supp --log-file=valgrind.log ./bitfighter_test --gtest_filter="GameUserInterfaceTest.Engineer"
```

## Windows

### Test Commands:
```cmd
cd <ROOT_DIR>\exe
bitfighter_test_debug.exe

:: Run only specific tests
cd <ROOT_DIR>\exe
bitfighter_test_debug.exe --gtest_filter=ChatHelperTest.*
```

# Project Targets
- `Bitfighter` / `bitfighter`: Main graphical game client.
- `bitfighterd`: Dedicated headless server.
- `bitfighter_test`: GoogleTest unit and integration test suite.
- `master`: Master server daemon (for matchmaking and server listings).