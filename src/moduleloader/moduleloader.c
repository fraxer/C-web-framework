#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "../config/config.h"
#include "../log/log.h"
#include "../jsmn/jsmn.h"
#include "../route/route.h"
#include "../route/routeloader.h"
#include "../domain/domain.h"
#include "../server/server.h"
#include "../database/database.h"
// #include "../openssl/openssl.h"
// #include "../mimetype/mimetype.h"
#include "../thread/threadhandler.h"
#include "../thread/threadworker.h"
#include "../jsmn/jsmn.h"
#include "moduleloader.h"
    #include <stdio.h>

int module_loader_init_modules();

int module_loader_servers_load(int);

int module_loader_thread_workers_load();

int module_loader_thread_handlers_load();

int module_loader_reload_is_hard();


int module_loader_init(int argc, char* argv[]) {
    int result = -1;

    if (config_init(argc, argv) == -1) return -1;

    if (module_loader_init_modules() == -1) goto failed;

    result = 0;

    failed:

    config_free();

    return result;
}

int module_loader_reload() {
    int result = -1;

    if (config_reload() == -1) return -1;

    if (module_loader_init_modules() == -1) goto failed;

    result = 0;

    failed:

    config_free();

    return result;
}

int module_loader_init_modules() {
    int result = -1;
    int reload_is_hard = module_loader_reload_is_hard();

    if (reload_is_hard == -1) return -1;

    log_reinit();

    if (module_loader_servers_load(reload_is_hard)) return -1;

    if (module_loader_thread_workers_load() == -1) goto failed;

    if (module_loader_thread_handlers_load() == -1) goto failed;

    result = 0;

    failed:

    if (result == -1) {
        server_chain_destroy(server_chain_last());
    }

    return result;
}

int module_loader_reload_is_hard() {
    const jsmntok_t* token_root = config_get_section("main");

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "reload") == 0) {
            const char* value = jsmn_get_value(token->child);

            if (strcmp(value, "hard") == 0) {
                return 1;
            }
            else if (strcmp(value, "soft") == 0) {
                return 0;
            }
        }
    }

    log_error("Reload error: Reload type must be hard or soft\n");

    return -1;
}

route_t* module_loader_routes_load(routeloader_lib_t** first_lib, const jsmntok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    for (jsmntok_t* token_path = token_object->child; token_path; token_path = token_path->sibling) {
        const char* route_path = jsmn_get_value(token_path);

        route_t* route = route_create(route_path);

        if (route == NULL) goto failed;

        if (first_route == NULL) {
            first_route = route;
        }

        if (last_route) {
            last_route->next = route;
        }

        last_route = route;

        for (jsmntok_t* token_method = token_path->child->child; token_method; token_method = token_method->sibling) {
            const char* method = jsmn_get_value(token_method);

            jsmntok_t* token_array = token_method->child;

            const char* lib_file = jsmn_get_array_value(token_array, 0);
            const char* lib_handler = jsmn_get_array_value(token_array, 1);

            routeloader_lib_t* routeloader_lib = NULL;

            if (!routeloader_has_lib(*first_lib, lib_file)) {
                routeloader_lib = routeloader_load_lib(lib_file);

                if (routeloader_lib == NULL) goto failed;

                if (*first_lib == NULL) {
                    *first_lib = routeloader_lib;
                }

                if (last_lib) {
                    last_lib->next = routeloader_lib;
                }

                last_lib = routeloader_lib;
            }

            void*(*function)(void*) = (void*(*)(void*))routeloader_get_handler(*first_lib, lib_file, lib_handler);

            if (function == NULL) goto failed;

            if (route_set_method_handler(route, method, function) == -1) goto failed;

            // (*function)("Hello");

            // --------------

            // void*(*funion)(void*) = route->method[ROUTE_GET]->run;

            // (*funion)("Hello");

            // --------------

            // route->method[ROUTE_GET]("HELLO");
        }
    }

    result = first_route;

    failed:

    if (result == NULL) {
        route_free(first_route);
        routeloader_free(*first_lib);
    }

    return result;
}

domain_t* module_loader_domains_load(const jsmntok_t* token_array) {
    domain_t* result = NULL;
    domain_t* first_domain = NULL;
    domain_t* last_domain = NULL;

    for (jsmntok_t* token_domain = token_array->child; token_domain; token_domain = token_domain->sibling) {
        if (token_domain->type != JSMN_STRING) goto failed;

        const char* value = jsmn_get_value(token_domain);

        domain_t* domain = domain_create(value);

        if (domain == NULL) goto failed;

        if (first_domain == NULL) first_domain = domain;

        if (last_domain) {
            last_domain->next = domain;
        }

        last_domain = domain;
    }

    result = first_domain;

    failed:

    if (result == NULL) {
        domain_free(first_domain);
    }

    return result;
}

database_t* module_loader_database_load(const jsmntok_t* token_object) {
    database_t* result = NULL;
    database_t* database = database_create();

    enum fields { HOST = 0, PORT, DBNAME, USER, PASSWORD, DRIVER, CONNECTION_TIMEOUT, CONNECTION_COUNT, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "host") == 0) {
            finded_fields[HOST] = 1;

            const char* value = jsmn_get_value(token->child);

            database->host = (char*)malloc(strlen(value) + 1);

            if (database->host == NULL) goto failed;

            strcpy(database->host, value);
        }
        else if (strcmp(key, "port") == 0) {
            finded_fields[PORT] = 1;

            const char* value = jsmn_get_value(token->child);

            database->port = atoi(value);
        }
        else if (strcmp(key, "dbname") == 0) {
            finded_fields[DBNAME] = 1;

            const char* value = jsmn_get_value(token->child);

            database->dbname = (char*)malloc(strlen(value) + 1);

            if (database->dbname == NULL) goto failed;

            strcpy(database->dbname, value);
        }
        else if (strcmp(key, "user") == 0) {
            finded_fields[USER] = 1;

            const char* value = jsmn_get_value(token->child);

            database->user = (char*)malloc(strlen(value) + 1);

            if (database->user == NULL) goto failed;

            strcpy(database->user, value);
        }
        else if (strcmp(key, "password") == 0) {
            finded_fields[PASSWORD] = 1;

            const char* value = jsmn_get_value(token->child);

            database->password = (char*)malloc(strlen(value) + 1);

            if (database->password == NULL) goto failed;

            strcpy(database->password, value);
        }
        else if (strcmp(key, "driver") == 0) {
            finded_fields[DRIVER] = 1;

            const char* value = jsmn_get_value(token->child);

            database->driver = NONE;

            if (strcmp(value, "postgresql") == 0) {
                database->driver = POSTGRESQL;
            }
            else if (strcmp(value, "mysql") == 0) {
                database->driver = MYSQL;
            }

            if (database->driver == NONE) {
                log_error("Error: Database driver incorrect\n");
                goto failed;
            }
        }
        else if (strcmp(key, "connection_timeout") == 0) {
            finded_fields[CONNECTION_TIMEOUT] = 1;

            const char* value = jsmn_get_value(token->child);

            database->connection_timeout = atoi(value);
        }
        else if (strcmp(key, "connection_count") == 0) {
            finded_fields[CONNECTION_COUNT] = 1;

            const char* value = jsmn_get_value(token->child);

            database->port = atoi(value);
        }
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("Error: Fill database config\n");
            goto failed;
        }
    }

    // printf("driver: %ld\n", sizeof(*structure->connections));

    result = database;

    failed:

    if (result == NULL) {
        database_free(database);
    }

    return result;
}

int module_loader_domains_hash_create(server_t* server) {
    if (hcreate_r(domain_count(server->domain) * 2, server->domain_hashes) == 0) {
        log_error("Error: Can't create domain hashes\n");
        return -1;
    }

    domain_t* current = server->domain;

    while (current) {
        domain_t* next = current->next;

        ENTRY entry;
        ENTRY* entry_result;

        entry.key = current->template;
        entry.data = (void*)current->template;

        if (hsearch_r(entry, ENTER, &entry_result, server->domain_hashes) == 0) {
            log_error("Error: hash table entry failed\n");
            return -1;
        }

        current = next;
    }

    return 0;
}

int module_loader_check_unique_domains(server_t* first_server) {
    for (server_t* current_server = first_server; current_server; current_server = current_server->next) {
        for (domain_t* current_domain = current_server->domain; current_domain; current_domain = current_domain->next) {

            for (server_t* server = first_server; server; server = server->next) {
                for (domain_t* domain = server->domain; domain; domain = domain->next) {

                    if (current_domain != domain) {
                        if (strcmp(current_domain->template, domain->template) == 0) {
                            log_error("Error: Domains must be unique. %s\n", current_domain->template);
                            return -1;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

int module_loader_servers_load(int reload_is_hard) {
    const jsmntok_t* token_array = config_get_section("servers");

    if (token_array->type != JSMN_ARRAY) return -1;

    server_t* first_server = NULL;
    server_t* last_server = NULL;

    routeloader_lib_t* first_lib = NULL;

    int result = -1;

    for (jsmntok_t* token_item = token_array->child; token_item; token_item = token_item->sibling) {
        enum required_fields { R_DOMAINS = 0, R_IP, R_PORT, R_ROOT, R_FIELDS_COUNT };
        enum fields { DOMAINS = 0, IP, PORT, ROOT, REDIRECTS, INDEX, ROUTES, DATABASE, FIELDS_COUNT };

        int finded_fields[FIELDS_COUNT] = {0};

        const jsmntok_t* token_server = token_item;

        server_t* server = server_create();

        if (first_server == NULL) {
            first_server = server;
        }

        if (last_server) {
            last_server->next = server;
        }

        last_server = server;

        if (token_server->type != JSMN_OBJECT) return -1;

        for (jsmntok_t* token_key = token_server->child; token_key; token_key = token_key->sibling) {
            const char* key = jsmn_get_value(token_key);

            if (strcmp(key, "domains") == 0) {
                finded_fields[DOMAINS] = 1;

                if (token_key->child->type != JSMN_ARRAY) goto failed;

                server->domain = module_loader_domains_load(token_key->child);

                if (server->domain == NULL) {
                    log_error("Error: Can't load domains\n");
                    goto failed;
                }

                if (module_loader_domains_hash_create(server) == -1) {
                    goto failed;
                }
            }
            else if (strcmp(key, "ip") == 0) {
                finded_fields[IP] = 1;

                if (token_key->child->type != JSMN_STRING) goto failed;

                const char* value = jsmn_get_value(token_key->child);

                server->ip = inet_addr(value);
            }
            else if (strcmp(key, "port") == 0) {
                finded_fields[PORT] = 1;

                if (token_key->child->type != JSMN_PRIMITIVE) goto failed;

                const char* value = jsmn_get_value(token_key->child);

                server->port = atoi(value);
            }
            else if (strcmp(key, "root") == 0) {
                finded_fields[ROOT] = 1;

                if (token_key->child->type != JSMN_STRING) goto failed;

                const char* value = jsmn_get_value(token_key->child);

                server->root = (char*)malloc(strlen(value) + 1);

                if (server->root == NULL) {
                    log_error("Error: Can't alloc memory for root path\n");
                    goto failed;
                }

                strcpy(server->root, value);
            }
            else if (strcmp(key, "redirects") == 0) {
                finded_fields[REDIRECTS] = 1;
            }
            else if (strcmp(key, "index") == 0) {
                finded_fields[INDEX] = 1;

                if (token_key->child->type != JSMN_STRING) goto failed;

                const char* value = jsmn_get_value(token_key->child);

                server->index = server_create_index(value);

                if (server->index == NULL) {
                    log_error("Error: Can't alloc memory for index file\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "routes") == 0) {
                finded_fields[ROUTES] = 1;

                if (token_key->child->type != JSMN_OBJECT) goto failed;

                server->route = module_loader_routes_load(&first_lib, token_key->child);

                if (server->route == NULL) {
                    log_error("Error: Can't load routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "database") == 0) {
                finded_fields[DATABASE] = 1;

                if (token_key->child->type != JSMN_OBJECT) goto failed;

                server->database = module_loader_database_load(token_key->child);

                if (server->database == NULL) {
                    log_error("Error: Can't load database\n");
                    goto failed;
                }
            }
        }

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("Error: Fill config\n");
                goto failed;
            }
        }

        if (finded_fields[INDEX] == 0) {
            server->index = server_create_index("index.html");

            if (server->index == NULL) {
                log_error("Error: Can't alloc memory for index file\n");
                goto failed;
            }
        }

        if (module_loader_check_unique_domains(first_server) == -1) goto failed;
    }

    if (first_server == NULL) {
        log_error("Error: Section server not found in config\n");
        return -1;
    }

    if (server_chain_append(first_server, first_lib, reload_is_hard) == -1) goto failed;

    result = 0;

    failed:

    if (result == -1) {
        server_free(first_server);

        routeloader_free(first_lib);
    }

    return result;
}

int module_loader_thread_workers_load() {
    const jsmntok_t* token_root = config_get_section("main");

    int count = 0;

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "workers") == 0) {
            const char* value = jsmn_get_value(token->child);

            count = atoi(value);

            break;
        }
    }

    if (count <= 0) {
        log_error("Thread error: Set the number of workers\n");
        return -1;
    }

    server_chain_t* server_chain = server_chain_last();

    if (server_chain == NULL) return -1;

    if (thread_worker_run(count, server_chain) == -1) return -1;

    return 0;
}

int module_loader_thread_handlers_load() {
    const jsmntok_t* token_root = config_get_section("main");

    int count = 0;

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "threads") == 0) {
            const char* value = jsmn_get_value(token->child);

            count = atoi(value);

            break;
        }
    }

    if (count <= 0) {
        log_error("Thread error: Set the number of threads\n");
        return -1;
    }

    if (thread_handler_run(count) == -1) return -1;

    return 0;
}