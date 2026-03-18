Some helpful notes for our AI friends (and some humans may find this useful as well)

# Test Commands
## Windows:
Windows debug tests:

    cd <ROOT_DIR>\exe
    bitfighter_test_debug.exe

Run only ChatHelper tests:

    cd <ROOT_DIR>\exe
    bitfighter_test_debug.exe --gtest_filter=ChatHelperTest.*

# Building on WSL
Make a copy of the source code in <ROOT_DIR>.

# Running Valgrind
## WSL
### Build tests:
    cd <ROOT_DIR>
    cmake <ROOT_DIR> -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -Wno-error"
    cmake --build . --target bitfighter_test -j 1

### Run valgrind:
    cd <ROOT_DIR>/exe
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=../lua/luajit/src/lj.supp --log-file=valgrind.log ./bitfighter_test --gtest_filter="GameUserInterfaceTest.Engineer"