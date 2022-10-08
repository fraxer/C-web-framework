#include <cassert>
#include "rapidjson/document.h"
#include "../../src/config/config.h"

bool TEST_getPath_correctArguments() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../config.json"};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_notFoundFile() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../conf.json"};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_notFoundOptionName() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"../../conf.json", (char*)"n"};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_notFoundProgramName() {

    int argc = 2;

    char* argv[argc] = {(char*)"-c", (char*)"../../conf.json"};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_withProgramName() {

    int argc = 1;

    char* argv[argc] = {(char*)"program"};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_withoutOptions() {

    int argc = 0;

    char* argv[argc] = {};

    return config::getPath(argc, argv) != nullptr;
}

bool TEST_getPath_undefinedOption() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-t", (char*)"../../config.json"};

    return config::getPath(argc, argv) != nullptr;
}

int main() {

    assert( TEST_getPath_correctArguments());
    assert( TEST_getPath_notFoundFile());
    assert(!TEST_getPath_notFoundOptionName());
    assert(!TEST_getPath_notFoundProgramName());
    assert(!TEST_getPath_withProgramName());
    assert(!TEST_getPath_withoutOptions());
    assert(!TEST_getPath_undefinedOption());

    return 0;
}