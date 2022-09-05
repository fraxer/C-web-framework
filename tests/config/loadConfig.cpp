#include <cassert>
#include "../../src/config/config.h"

bool TEST_loadConfig_correctPath() {

    const char* path = "path";

    try {
        loadConfig(path);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_loadConfig_nullptr() {
    return false;
}

bool TEST_loadConfig_emptyPath() {
    return false;
}

bool TEST_loadConfig_incorrectPath() {
    return false;
}

int main() {

    assert( TEST_loadConfig_correctPath());
    assert(!TEST_loadConfig_nullptr());
    assert(!TEST_loadConfig_emptyPath());
    assert(!TEST_loadConfig_incorrectPath());

    return 0;
}