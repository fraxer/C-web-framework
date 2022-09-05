#include <cassert>
#include "../../src/config/config.h"

bool TEST_getPathConfigFile_correctArguments() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../config.json"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_notFoundFile() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-c", (char*)"../../conf.json"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_notFoundOptionName() {

    int argc = 2;

    char* argv[argc] = {(char*)"program", (char*)"../../conf.json", (char*)"n"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_notFoundProgramName() {

    int argc = 2;

    char* argv[argc] = {(char*)"-c", (char*)"../../conf.json"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_withProgramName() {

    int argc = 1;

    char* argv[argc] = {(char*)"program"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_withoutOptions() {

    int argc = 0;

    char* argv[argc] = {};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

bool TEST_getPathConfigFile_undefinedOption() {

    int argc = 3;

    char* argv[argc] = {(char*)"program", (char*)"-t", (char*)"../../config.json"};

    try {
        getPathConfigFile(argc, argv);
    } catch (const char* message) {
        return false;
    }

    return true;
}

int main() {

    assert( TEST_getPathConfigFile_correctArguments());
    assert( TEST_getPathConfigFile_notFoundFile());
    assert(!TEST_getPathConfigFile_notFoundOptionName());
    assert(!TEST_getPathConfigFile_notFoundProgramName());
    assert(!TEST_getPathConfigFile_withProgramName());
    assert(!TEST_getPathConfigFile_withoutOptions());
    assert(!TEST_getPathConfigFile_undefinedOption());

    return 0;
}