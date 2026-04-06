#include <iostream>
#include <string>
#include <vector>
#include "zap/stringUtils.h"

using namespace std;
using namespace Zap;

int main() {
    cout << "Testing itos(U64)..." << endl;
    U64 val64 = 123456789012345ULL;
    string s64 = itos(val64);
    cout << "itos(123456789012345ULL) = " << s64 << endl;
    if (s64 != "123456789012345") cout << "FAILED itos(U64)" << endl;

    cout << "Testing itos(S64)..." << endl;
    S64 sval64 = -123456789012345LL;
    string ss64 = itos(sval64);
    cout << "itos(-123456789012345LL) = " << ss64 << endl;
    // Note: My previous read showed itos(S64) was very broken, let's see what it does.

    cout << "Testing ftos(0.0f, 2)..." << endl;
    try {
        string sz = ftos(0.0f, 2);
        cout << "ftos(0.0f, 2) = " << sz << endl;
    } catch (...) {
        cout << "ftos(0.0f, 2) CRASHED" << endl;
    }

    cout << "Testing ftos(0.01f, 2)..." << endl;
    try {
        string sz = ftos(0.01f, 2);
        cout << "ftos(0.01f, 2) = " << sz << endl;
    } catch (...) {
        cout << "ftos(0.01f, 2) CRASHED" << endl;
    }

    cout << "Testing extractDirectory(\"myfile.txt\")..." << endl;
    string dir = extractDirectory("myfile.txt");
    cout << "extractDirectory(\"myfile.txt\") = \"" << dir << "\"" << endl;
    if (dir != "") cout << "FAILED extractDirectory" << endl;

    return 0;
}
