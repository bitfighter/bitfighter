#include <iostream>
#include <string>
#include <cassert>
#include "../zap/stringUtils.h"

using namespace Zap;

void test_isInteger() {
    std::cout << "Testing isInteger..." << std::endl;
    assert(isInteger("123") == true);
    assert(isInteger("0") == true);
    assert(isInteger("") == false);
    assert(isInteger(NULL) == false);
    assert(isInteger("abc") == false);
    std::cout << "isInteger passed!" << std::endl;
}

void test_extractExtension() {
    std::cout << "Testing extractExtension..." << std::endl;
    assert(extractExtension("file.txt") == "txt");
    assert(extractExtension("file.tar.gz") == "gz");
    assert(extractExtension("file_with_no_extension") == "");
    assert(extractExtension("/path.to/file") == "");
    assert(extractExtension("/path.to/file.txt") == "txt");
    std::cout << "extractExtension passed!" << std::endl;
}

void test_stripExtension() {
    std::cout << "Testing stripExtension..." << std::endl;
    assert(stripExtension("file.txt") == "file");
    assert(stripExtension("file.tar.gz") == "file.tar");
    assert(stripExtension("file_with_no_extension") == "file_with_no_extension");
    assert(stripExtension("/path.to/file") == "/path.to/file");
    assert(stripExtension("/path.to/file.txt") == "/path.to/file");
    std::cout << "stripExtension passed!" << std::endl;
}

int main() {
    try {
        test_isInteger();
        test_extractExtension();
        test_stripExtension();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
