#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "../config/config.h"
#include "../log/log.h"
#include "../route/route.h"
#include "../database/database.h"
#include "../openssl/openssl.h"
#include "../mimetype/mimetype.h"
#include "../thread/thread.h"
#include "moduleLoader.h"

namespace moduleLoader {

int init(int argc, char* argv[]) {

    if (config::init(argc, argv) == -1) return -1;

    if (initModules() == -1) return -1;

    if (config::free() == -1) return -1;

    return 0;
}

int reload() {

    if (config::reload() == -1) return -1;

    if (initModules() == -1) return -1;

    if (config::free() == -1) return -1;

    return 0;
}

int initModules() {

    if (initModule(&log::init, &parseLog) == -1) return -1;

    if (initModule(&route::init, &parseRoutes) == -1) return -1;

    if (initModule(&database::init, &parseDatabase) == -1) return -1;

    if (initModule(&openssl::init, &parseOpenssl) == -1) return -1;

    if (initModule(&mimetype::init, &parseMimetype) == -1) return -1;

    if (initModule(&thread::init, &parseThread) == -1) return -1;

    // if (initModule(&thread::init, nullptr) == -1) return -1;

    // if (initModule(&yookassa::init, &parseYookassa) == -1) return -1;

    // if (initModule(&s3::init, &parseS3) == -1) return -1;

    // if (initModule(&mail::init, &parseMail) == -1) return -1;

    // if (initModule(&encryption::init, &parseEncryption) == -1) return -1;

    return 0;
}

int initModule(int (*init)(), int (*parse)()) {

    if (init == nullptr) return -1;

    if (init() == -1) return -1;

    if (parse == nullptr) return 0;

    if (parse() == -1) return -1;

    return 0;
}

int parseLog() {
    return 0;
}

int parseRoutes() {

    // struct stat stat_obj;

    int result = -1;

    // int countProperties = 0;

    // for (auto& prop : content.GetObject()) {

    //     if (strcmp(prop.name.GetString(), "ssl_fullchain_file") == 0) {

    //         if (!prop.value.IsString()) {
    //             // syslogError("[load] Значение ssl_fullchain_file должно быть строкой\n");
    //             goto failed;
    //         }

    //         if (!prop.value.GetStringLength()) {
    //             // syslog(LOG_ERR, "[load] Путь до файла с полной цепочкой сертификатов не указан\n");
    //             goto failed;
    //         }

    //         stat(prop.value.GetString(), &stat_obj);

    //         if (!S_ISREG(stat_obj.st_mode)) {
    //             // syslog(LOG_ERR, "[load] Путь до файла с полной цепочкой сертификатов неверен\n");
    //             goto failed;
    //         }
    //     }

    //     countProperties++;
    // }

    // if (countProperties == 0) {
    //     // syslog(LOG_ERR, "[load] Файл конфигурации пуст\n");
    //     goto failed;
    // }

    result = 0;

    // failed:

    return result;
}

int parseDatabase() {

    const rapidjson::Value* document = config::getSection("database");

    const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };

    for (rapidjson::Value::ConstMemberIterator itr = document->MemberBegin(); itr != document->MemberEnd(); ++itr)
    {
        printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);

        if (itr->value.IsBool()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetBool());
        } else if (itr->value.IsObject()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetObject());
        } else if (itr->value.IsArray()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetArray());
        } else if (itr->value.IsString()) {
            printf("Type of member %s is %s\n", itr->name.GetString(), itr->value.GetString());
            // setParam(itr->name.GetString(), (void*)itr->name.GetString());
        } else if (itr->value.IsDouble()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetDouble());
        } else if (itr->value.IsInt64()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetInt64());
        } else if (itr->value.IsUint64()) {
            // setParam(itr->name.GetString(), (void*)itr->name.GetUint64());
        }
    }

    return 0;
}

int parseOpenssl() {
    return 0;
}

int parseMimetype() {
    return 0;
}

int parseThread() {
    return 0;
}

int parseYookassa() {
    return 0;
}

int parseS3() {
    return 0;
}

int parseMail() {
    return 0;
}

int parseEncryption() {
    return 0;
}

} // namespace
