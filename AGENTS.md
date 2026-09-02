Some helpful notes for our AI friends and humans.

# Building and Testing

> **Important:** Always execute `bitfighter_test` from the `<ROOT_DIR>/exe` directory so resource paths and levels resolve properly.

## macOS

### Prerequisites via Homebrew
On Apple Silicon macOS, LuaJIT 2.1 from Homebrew is required:
```bash
brew install luajit sdl2 libpng libvorbis libogg speex libmodplug openal-soft
```

### Build and Test Commands
```bash
# Configure out-of-source build, add -DMASTER_MINIMAL=ON when MySQL server is not used
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMASTER_MINIMAL=ON

# Build test runner
cmake --build build --target bitfighter_test

# Build game client
cmake --build build --target Bitfighter

# Run test suite
cd exe && ./bitfighter_test

# Run filtered tests such as ChatHelper or StringUtils
cd exe && ./bitfighter_test --gtest_filter=ChatHelperTest.*
```

## Linux and WSL

### Build and Test Commands
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMASTER_MINIMAL=ON

# Build
cmake --build build --target bitfighter_test

# Run tests
cd exe && ./bitfighter_test
```

### Running Valgrind on Linux and WSL
```bash
# Build with debug symbols and no optimization
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -Wno-error" -DMASTER_MINIMAL=ON
cmake --build build --target bitfighter_test -j1

# Run Valgrind
cd exe
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=../lua/luajit/src/lj.supp --log-file=valgrind.log ./bitfighter_test --gtest_filter="GameUserInterfaceTest.Engineer"
```

## Windows

### Test Commands
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
- `master`: Master server daemon for matchmaking and server listings.