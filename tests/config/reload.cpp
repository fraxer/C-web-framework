#include <cassert>
#include "rapidjson/document.h"
#include "../common.h"
#include "../../src/config/config.h"

bool TEST_reload_success() {

    char* path = getFullPath("/tests/config/config.json");

    if (config::load(path) == -1) {
        free(path);
        return false;
    }

    free(path);

    config::reload();

    return true;
}

bool TEST_reload_repeat() {

    char* path = getFullPath("/tests/config/config.json");

    if (config::load(path) == -1) {
        free(path);
        return false;
    }

    free(path);

    config::reload();
    config::reload();
    config::reload();
    config::reload();
    config::reload();
    config::reload();

    return true;
}

int main() {

    assert(TEST_reload_success());
    assert(TEST_reload_repeat());

    return 0;
}