#include <cassert>
#include "../../src/config/config.h"

bool TEST_getConfigPath_correctArguments() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../config.json"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_notFoundFile() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../conf.json"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_notFoundOptionName() {

    int argc = 2;

    char* argv[argc] = {(char*)"program", (char*)"../../conf.json", (char*)"n"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_notFoundProgramName() {

    int argc = 2;

    char* argv[argc] = {(char*)"-c", (char*)"../../conf.json"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_withProgramName() {

    int argc = 1;

    char* argv[argc] = {(char*)"program"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_withoutOptions() {

    int argc = 0;

    char* argv[argc] = {};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getConfigPath_undefinedOption() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-t", (char*)"../../config.json"};

    try {
        getConfigPath(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

int main() {

    assert( TEST_getConfigPath_correctArguments());
    assert( TEST_getConfigPath_notFoundFile());
    assert(!TEST_getConfigPath_notFoundOptionName());
    assert(!TEST_getConfigPath_notFoundProgramName());
    assert(!TEST_getConfigPath_withProgramName());
    assert(!TEST_getConfigPath_withoutOptions());
    assert(!TEST_getConfigPath_undefinedOption());

    return 0;
}