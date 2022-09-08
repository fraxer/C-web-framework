#include <cassert>
#include "../../src/config/config.h"

bool TEST_reloadConfig() {

    try {
        reloadConfig();
    } catch (const char* message) {
        return false;
    }

    return false;
}

int main() {

    assert( TEST_reloadConfig());

    return 0;
}