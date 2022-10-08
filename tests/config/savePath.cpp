#include <cassert>
#include "rapidjson/document.h"
#include "../../src/config/config.h"

bool TEST_savePath_correctPath() {

    const char* path = "path";

    return config::savePath(path) == 0;
}

bool TEST_savePath_emptyPath() {

    const char* path = "";

    return config::savePath(path) == 0;
}

bool TEST_savePath_nullptr() {

    const char* path = nullptr;

    return config::savePath(path) == 0;
}

int main() {

    assert( TEST_savePath_correctPath());
    assert(!TEST_savePath_emptyPath());
    assert(!TEST_savePath_nullptr());

    return 0;
}