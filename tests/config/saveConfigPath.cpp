#include <cassert>
#include "../../src/config/config.h"

bool TEST_saveConfigPath_correctPath() {

    const char* path = "path";

    try {
        saveConfigPath(path);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_saveConfigPath_emptyPath() {

    const char* path = "";

    try {
        saveConfigPath(path);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_saveConfigPath_nullptr() {

    const char* path = nullptr;

    try {
        saveConfigPath(path);
    } catch (const char* message) {
        return false;
    }

    return true;
}

int main() {

    assert( TEST_saveConfigPath_correctPath());
    assert(!TEST_saveConfigPath_emptyPath());
    assert(!TEST_saveConfigPath_nullptr());

    return 0;
}