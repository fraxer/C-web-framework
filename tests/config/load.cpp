#include <cassert>
#include "rapidjson/document.h"
#include "../common.h"
#include "../../src/config/config.h"

bool TEST_load_correctPath() {

    char* path = getFullPath("/tests/config/config.json");

    if (config::load(path) == -1) {
        free(path);
        return false;
    }

    free(path);

    return true;
}

bool TEST_load_incorrectPath() {

    char* path = getFullPath("/tests/config/undefined.txt");

    if (config::load(path) == -1) {
        free(path);
        return false;
    }

    free(path);

    return true;
}

bool TEST_load_nullptr() {

    char* path = nullptr;

    if (config::load(path) == -1) {
        return false;
    }

    return true;
}

bool TEST_load_emptyPath() {

    const char* path = "";

    if (config::load(path) == -1) {
        return false;
    }

    return true;
}

int main() {

    assert( TEST_load_correctPath());
    assert(!TEST_load_incorrectPath());
    assert(!TEST_load_nullptr());
    assert(!TEST_load_emptyPath());

    return 0;
}