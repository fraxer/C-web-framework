#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include "json.h"
#include "moduleloader.h"
#include "dbquery.h"
#include "dbresult.h"
#include "database.h"
#ifdef MySQL_FOUND
    #include "mysql.h"
#endif
#ifdef PostgreSQL_FOUND
    #include "postgresql.h"
#endif
#ifdef Redis_FOUND
    #include "redis.h"
#endif


typedef enum mgactions {
    NONE = 0,
    CREATE,
    UP,
    DOWN
} mgactions_e;

// migrate s1 create create_users_table config.json

// migrate s1 up config.json
// migrate s1 up 2 config.json
// migrate s1 up all config.json

// migrate s1 down config.json
// migrate s1 down 3 config.json
// migrate s1 down all config.json

typedef struct mgconfig {
    int ok;
    char* server;
    mgactions_e action;
    char* migration_name;
    int count_migrations;
    int count_applied_migrations;
    char* config_path;
    db_t* database;
} mgconfig_t;

void mg_config_free(mgconfig_t* config) {
    config->ok = 0;
    config->action = NONE;
    config->count_migrations = 0;
    config->count_applied_migrations = 0;
    if (config->database) db_free(config->database);
    if (config->server) free(config->server);
    if (config->migration_name) free(config->migration_name);
    if (config->config_path) free(config->config_path);
}

int mg_parse_action_create(mgconfig_t* config, int argc, char* argv[]) {
    if (argc != 5) {
        printf("Error: command incorrect\n");
        printf("Example: migrate <server id> create <migration name> <config path>\n");
        return -1;
    }

    config->ok = 1;
    config->migration_name = strdup(argv[3]);
    config->config_path = strdup(argv[4]);

    return 0;
}

int mg_parse_action_up_down(mgconfig_t* config, int pos, int argc, char* argv[]) {
    if (strspn(argv[pos], "0123456789") == strlen(argv[pos])) {
        config->count_migrations = atoi(argv[pos]);
        pos++;
    }
    else if (strcmp(argv[pos], "all") == 0) {
        config->count_migrations = 0;
        pos++;
    }

    if (pos == argc - 1 && strlen(argv[argc - 1]) > 0) {
        config->ok = 1;
        config->config_path = strdup(argv[argc - 1]);
        return 0;
    }

    printf("Error: command incorrect\n");
    printf("Example: migrate <server id> up [number, all] <config path>\n");
    printf("Example: migrate <server id> down [number, all] <config path>\n");

    return -1;
}

mgconfig_t mg_args_parse(int argc, char* argv[]) {
    mgconfig_t config = {
        .ok = 0,
        .server = NULL,
        .action = NONE,
        .migration_name = NULL,
        .count_migrations = 1,
        .config_path = NULL,
        .database = NULL
    };

    if (argc < 4) {
        printf("Error: command incorrect\n");
        printf("Example: migrate <server id> <action> [number, all] <config path>\n");
        return config;
    }

    int pos = 1;
    config.server = strdup(argv[pos++]);

    if (strcmp(argv[pos], "create") == 0) {
        config.action = CREATE;
        if (mg_parse_action_create(&config, argc, argv) == -1) return config;
    }
    else if (strcmp(argv[pos], "up") == 0) {
        config.action = UP;
        if (mg_parse_action_up_down(&config, ++pos, argc, argv) == -1) return config;
    }
    else if (strcmp(argv[pos], "down") == 0) {
        config.action = DOWN;
        if (mg_parse_action_up_down(&config, ++pos, argc, argv) == -1) return config;
    }
    else {
        printf("Error: command incorrect\n");
        return config;
    }

    return config;
}

const char* mg_config_read(const char* config_path) {
    int fd = open(config_path, O_RDONLY);
    if (fd == -1) return NULL;

    size_t filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    char* result = NULL;
    char* buffer = malloc(filesize * sizeof *buffer);

    if (buffer == NULL) goto failed;

    if (read(fd, buffer, filesize) <= 0) goto failed;

    result = buffer;

    failed:

    if (result == NULL) {
        if (buffer) free(buffer);
    }

    close(fd);

    return result;
}

int mg_database_parse(mgconfig_t* config, const jsontok_t* token_object) {
    if (token_object == NULL) return -1;
    if (!json_is_object(token_object)) return -1;

    if (module_loader_databases_load_token(token_object) == -1)
        return -1;

    config->database = database();

    return 0;
}

int mg_sort_asc(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

int mg_sort_desc(const struct dirent **a, const struct dirent **b) {
    return strcoll((*b)->d_name, (*a)->d_name);
}

int mg_migration_table_exist(dbinstance_t* dbinst) {
    dbresult_t result = dbtable_exist(dbinst, "migration");

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    int rows = dbresult_query_rows(&result);

    dbresult_free(&result);

    return rows > 0;
}

int mg_migration_table_create(dbinstance_t* dbinst) {
    dbresult_t result = dbtable_migration_create(dbinst);

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }
    dbresult_free(&result);

    return 0;
}

int mg_migration_exist(dbinstance_t* dbinst, const char* filename) {
    dbresult_t result = dbquery(dbinst,
        "SELECT "
            "1 "
        "FROM "
            "migration "
        "WHERE "
            "version = '%s'",
        filename
    );

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return 0;
    }

    int rows = dbresult_query_rows(&result);

    dbresult_free(&result);

    return rows > 0;
}

int mg_migration_commit(dbinstance_t* dbinst, const char* filename) {
    dbresult_t result = dbquery(dbinst,
        "INSERT INTO "
            "migration "
            "(version, apply_time) "
        "VALUES "
            "('%s', '%d') ",
        filename,
        time(0)
    );

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);

    return 0;
}

int mg_migration_rollback(dbinstance_t* dbinst, const char* filename) {
    dbresult_t result = dbquery(dbinst,
        "DELETE FROM "
            "migration "
        "WHERE "
            "version = '%s'",
        filename
    );

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);

    return 0;
}

dbinstance_t mg_dbinstance(db_t* db, const char* dbid) {
    dbinstance_t inst = {
        .ok = 0,
        .hosts = NULL,
        .connection_create = NULL,
        .lock_connection = 0,
        .connection = NULL,
        .table_exist_sql = NULL,
        .table_migration_create_sql = NULL
    };

    while (db) {
        if (strcmp(db->id, dbid) == 0) {
            inst.ok = 1;
            inst.hosts = db->hosts;
            inst.connection_create = db->hosts->connection_create_manual;
            inst.lock_connection = &db->lock_connection;
            inst.connection = &db->connection;
            inst.table_exist_sql = db->hosts->table_exist_sql;
            inst.table_migration_create_sql = db->hosts->table_migration_create_sql;

            return inst;
        }

        db = db->next;
    }

    return inst;
}

int mg_default_up_down(dbinstance_t* instance) {
    (void)instance;
    return 0;
}

int mg_migrate_run(mgconfig_t* config, const char* filename, const char* path) {
    int result = -1;
    void* dlpointer = dlopen(path, RTLD_LAZY);

    if (dlpointer == NULL) {
        printf("Error: can't open file %s\n", path);
        return result;
    }

    int(*function_up)(dbinstance_t*) = NULL;
    int(*function_down)(dbinstance_t*) = NULL;
    const char*(*function_db)() = NULL;

    *(void**)(&function_up) = dlsym(dlpointer, "up");
    if (function_up == NULL) {
        function_up = mg_default_up_down;
    }

    *(void**)(&function_down) = dlsym(dlpointer, "down");
    if (function_down == NULL) {
        function_down = mg_default_up_down;
    }

    *(void**)(&function_db) = dlsym(dlpointer, "db");
    if (function_db == NULL) {
        printf("Error: can't load function db\n");
        goto failed;
    }

    dbinstance_t dbinst = mg_dbinstance(config->database, function_db());

    if (!dbinst.ok) {
        printf("Error: not found database in %s\n", path);
        goto failed;
    }

    dbhost_t* start_host = dbinst.hosts->current_host;
    dbhost_t* host = dbinst.hosts->current_host;
    int make_increment = 0;

    while (host) {
        if (!host->migration) goto next_host;

        if (!mg_migration_table_exist(&dbinst)) {
            if (mg_migration_table_create(&dbinst) == -1) goto failed;
        }

        if (config->action == UP) {
            if (!mg_migration_exist(&dbinst, filename) && function_up(&dbinst) == 0) {
                mg_migration_commit(&dbinst, filename);
                printf("success up %s in %s:%d\n", path, host->ip, host->port);
                make_increment = 1;
            }
        }
        else if (config->action == DOWN) {
            if (mg_migration_exist(&dbinst, filename) && function_down(&dbinst) == 0) {
                mg_migration_rollback(&dbinst, filename);
                printf("success down %s in %s:%d\n", path, host->ip, host->port);
                make_increment = 1;
            }
        }

        next_host:

        db_next_host(dbinst.hosts);

        host = dbinst.hosts->current_host;

        if (host == start_host) break;
    }

    result = 0;

    if (make_increment) config->count_applied_migrations++;

    failed:

    dlclose(dlpointer);

    return result;
}

int mg_migrations_process(mgconfig_t* config) {
    int result = -1;
    const char* migrations_dir = "./migrations/";
    size_t path_length = strlen(migrations_dir) + strlen(config->server);
    char* path = malloc(path_length + 1);
    if (path == NULL) {
        printf("Error: out of memory\n");
        return result;
    }

    strcpy(path, migrations_dir);
    strcat(path, config->server);

    struct dirent **namelist;
    const int count_files = scandir(path, &namelist, NULL, config->action == UP ? mg_sort_asc : mg_sort_desc);
    if (count_files == -1) {
        printf("Error: no such directory\n");
        goto failed;
    }

    const int reg_file = 8;
    for (int i = 0; i < count_files; i++) {
        if (namelist[i]->d_type == reg_file) {
            if (config->count_migrations > 0 &&
                config->count_applied_migrations == config->count_migrations) {
                break;
            }
            size_t length = path_length + strlen(namelist[i]->d_name) + 1; // "/"
            char* filepath = malloc(length + 1);

            if (filepath == NULL) {
                printf("Error: out of memory\n");
                free(filepath);
                goto failed;
            }

            strcpy(filepath, path);
            strcat(filepath, "/");
            strcat(filepath, namelist[i]->d_name);

            if (mg_migrate_run(config, namelist[i]->d_name, filepath) == -1) {
                printf("Error: can't attach file %s\n", filepath);
                free(filepath);
                goto failed;
            }
            free(filepath);
        }
    }

    result = 0;

    failed:

    for (int i = 0; i < count_files; i++) {
        free(namelist[i]);
    }
    free(namelist);
    free(path);

    return result;
}

int mg_make_directory(const char* server, const char* source_directory) {
    char tmp[512] = {0};

    strcpy(&tmp[0], source_directory);

    size_t len = strlen(tmp);

    if (tmp[len - 1] != '/') {
        strcat(&tmp[0], "/");
    }

    strcat(&tmp[0], server);

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }

    mkdir(tmp, S_IRWXU);

    struct stat st = {0};

    if (stat(&tmp[0], &st) == 0 && S_ISDIR(st.st_mode)) return 0;

    printf("Error: can't create directory %s", &tmp[0]);

    return -1;
}

int mg_create_template(const char* source_directory, mgconfig_t* config) {
    char tmp[512] = {0};

    strcpy(&tmp[0], source_directory);

    size_t len = strlen(tmp);

    if (tmp[len - 1] != '/') {
        strcat(&tmp[0], "/");
    }

    strcat(&tmp[0], config->server);
    strcat(&tmp[0], "/");

    time_t t = time(0);
    struct tm tm = *localtime(&t);
    sprintf(&tmp[strlen(tmp)], "%d-%02d-%02d_%02d-%02d-%02d_", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    strcat(&tmp[0], config->migration_name);
    strcat(&tmp[0], ".c");

    const char* data =
        "#include <stdlib.h>\n"
        "#include <stdio.h>\n\n"
        "#include \"dbquery.h\"\n"
        "#include \"dbresult.h\"\n\n"

        "const char* db() { return \"\"; }\n\n"

        "int up(dbinstance_t* dbinst) {\n"
        "    dbresult_t result = dbquery(dbinst, \"\");\n\n"

        "    if (!dbresult_ok(&result)) {\n"
        "        printf(\"%s\\n\", dbresult_error_message(&result));\n"
        "        dbresult_free(&result);\n"
        "        return -1;\n"
        "    }\n\n"

        "    dbresult_free(&result);\n\n"

        "    return 0;\n"
        "}\n\n"

        "int down(dbinstance_t* dbinst) {\n"
        "    dbresult_t result = dbquery(dbinst, \"\");\n\n"

        "    if (!dbresult_ok(&result)) {\n"
        "        printf(\"%s\\n\", dbresult_error_message(&result));\n"
        "        dbresult_free(&result);\n"
        "        return -1;\n"
        "    }\n\n"

        "    dbresult_free(&result);\n\n"

        "    return 0;\n"
        "}\n"
    ;

    int fd = open(&tmp[0], O_RDWR | O_CREAT, S_IRWXU);

    if (fd <= 0) {
        printf("Error: can't create migration %s", &tmp[0]);
        return -1;
    }

    int r = write(fd, data, strlen(data));
    if (r <= 0) {
        printf("Error: can't write tot file %s", &tmp[0]);
    }
    close(fd);

    return 0;
}

int mg_migration_create(const jsontok_t* token_root, mgconfig_t* config) {
    const jsontok_t* token_migrations = json_object_get(token_root, "migrations");
    if (token_migrations == NULL) return -1;

    const jsontok_t* token_string = json_object_get(token_migrations, "source_directory");
    if (token_string == NULL) return -1;
    if (!json_is_string(token_string)) return -1;

    const char* source_directory = json_string(token_string);

    if (mg_make_directory(config->server, source_directory) == -1) return -1;
    if (mg_create_template(source_directory, config) == -1) return -1;

    return 0;
}

int main(int argc, char* argv[]) {
    const char* data = NULL;
    jsondoc_t* document = NULL;
    mgconfig_t config = mg_args_parse(argc, argv);
    if (!config.ok) goto failed;

    data = mg_config_read(config.config_path);
    if (data == NULL) goto failed;

    document = json_init();
    if (!document) goto failed;

    if (!json_parse(document, data)) goto failed;

    jsontok_t* token_object = json_root(document);
    if (!json_is_object(token_object)) goto failed;

    const jsontok_t* token_databases = json_object_get(token_object, "databases");
    if (token_databases == NULL) goto failed;

    if (mg_database_parse(&config, token_databases) == -1) goto failed;

    if (config.action == CREATE) {
        if (mg_migration_create(token_object, &config) == -1) goto failed;
    }
    else if (mg_migrations_process(&config) == -1) goto failed;

    failed:

    mg_config_free(&config);
    json_free(document);
    if (data) free((void*)data);

    return EXIT_SUCCESS;
}