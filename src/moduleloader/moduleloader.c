#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "../config/config.h"
#include "../log/log.h"
#include "../route/route.h"
#include "../route/routeloader.h"
#include "../domain/domain.h"
#include "../server/server.h"
// #include "../database/database.h"
// #include "../openssl/openssl.h"
// #include "../mimetype/mimetype.h"
#include "../thread/threadhandler.h"
#include "../thread/threadworker.h"
#include "../jsmn/jsmn.h"
#include "moduleloader.h"
    #include <stdio.h>

typedef struct server_destroy {
    
} server_destroy_t;

int module_loader_load_thread_workers();

int module_loader_load_thread_handlers();

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
    int reload_is_hard = module_loader_reload_is_hard();

    // если что-то пойдет не так, изменения не должны примениться

    if (reload_is_hard == -1) return -1;

    log_reinit();

    if (module_loader_load_servers(reload_is_hard)) return -1;

    if (module_loader_load_thread_workers() == -1) return -1;

    if (module_loader_load_thread_handlers() == -1) return -1;

    return 0;
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

route_t* module_loader_load_routes(const jsmntok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;

    for (jsmntok_t* token_path = token_object->child; token_path; token_path = token_path->sibling) {
        const char* route_path = jsmn_get_value(token_path);

        route_t* route = route_create(route_path, last_route);

        if (route == NULL) goto failed;

        if (first_route == NULL) first_route = route;

        last_route = route;

        for (jsmntok_t* token_method = token_path->child->child; token_method; token_method = token_method->sibling) {
            const char* method = jsmn_get_value(token_method);

            jsmntok_t* token_array = token_method->child;

            const char* lib_file = jsmn_get_array_value(token_array, 0);
            const char* lib_handler = jsmn_get_array_value(token_array, 1);

            if (routeloader_load_lib(lib_file) == -1) goto failed;

            void*(*function)(void*) = (void*(*)(void*))routeloader_get_handler(lib_file, lib_handler);

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

    return result;
}

domain_t* module_loader_load_domains(const jsmntok_t* token_array) {
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

int module_loader_load_servers(int reload_is_hard) {
    const jsmntok_t* token_array = config_get_section("servers");

    if (token_array->type != JSMN_ARRAY) return -1;

    server_t* first_server = NULL;
    server_t* last_server = NULL;

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

                server->domain = module_loader_load_domains(token_key->child);

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

                server->route = module_loader_load_routes(token_key->child);

                if (server->route == NULL) {
                    log_error("Error: Can't load routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "database") == 0) {
                finded_fields[DATABASE] = 1;
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

    if (server_chain_append(first_server, reload_is_hard) == -1) goto failed;

    result = 0;

    failed:

    if (result == -1) {
        server_free(first_server);
    }

    return result;
}

int module_loader_parse_database(void* moduleStruct) {

    // database::database_t* structure = (database::database_t*)moduleStruct;

    const jsmntok_t* document = config_get_section("database");

    // const auto& object = document->GetObject();

    // if (module_loader_set_string(&object, &structure->host, "host") == -1) return -1;
    // if (module_loader_set_string(&object, &structure->dbname, "dbname") == -1) return -1;
    // if (module_loader_set_string(&object, &structure->user, "user") == -1) return -1;
    // if (module_loader_set_string(&object, &structure->password, "password") == -1) return -1;
    // if (module_loader_set_ushort(&object, &structure->port, "port") == -1) return -1;
    // if (module_loader_set_ushort(&object, &structure->connections_timeout, "connections_timeout") == -1) return -1;
    // if (module_loader_set_database_driver(&object, &structure->driver, "driver") == -1) return -1;
    // if (module_loader_set_database_connections(&object, &structure->connections, "connections_count") == -1) return -1;

    // printf("driver: %ld\n", sizeof(*structure->connections));

    return 0;
}

int module_loader_load_thread_workers() {
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

int module_loader_load_thread_handlers() {
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