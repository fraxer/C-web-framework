#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../config/config.h"
#include "../log/log.h"
#include "../jsmn/jsmn.h"
#include "../redirect/redirect.h"
#include "../route/route.h"
#include "../route/routeloader.h"
#include "../domain/domain.h"
#include "../server/server.h"
#include "../database/database.h"
#ifdef MySQL_FOUND
    #include "../database/mysql.h"
#endif
#ifdef PostgreSQL_FOUND
    #include "../database/postgresql.h"
#endif
#ifdef Redis_FOUND
    #include "../database/redis.h"
#endif
#include "../openssl/openssl.h"
#include "../mimetype/mimetype.h"
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
int module_loader_mimetype_load();
int module_loader_set_http_route(routeloader_lib_t**, routeloader_lib_t**, route_t* route, const jsmntok_t*);
int module_loader_set_websockets_route(routeloader_lib_t**, routeloader_lib_t**, route_t* route, const jsmntok_t*);
server_info_t* module_loader_server_info_load();


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

    if (module_loader_mimetype_load() == -1) return -1;

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

route_t* module_loader_http_routes_load(routeloader_lib_t** first_lib, const jsmntok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return NULL;
    if (token_object->type != JSMN_OBJECT) goto failed;

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

        if (module_loader_set_http_route(first_lib, &last_lib, route, token_path->child) == -1) goto failed;
    }

    result = first_route;

    failed:

    if (result == NULL) {
        if (first_route) route_free(first_route);
    }

    return result;
}

int module_loader_set_http_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsmntok_t* token_object) {
    for (jsmntok_t* token_method = token_object->child; token_method; token_method = token_method->sibling) {
        const char* method = jsmn_get_value(token_method);

        jsmntok_t* token_array = token_method->child;

        const char* lib_file = jsmn_get_array_value(token_array, 0);
        const char* lib_handler = jsmn_get_array_value(token_array, 1);

        routeloader_lib_t* routeloader_lib = NULL;

        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib = routeloader_load_lib(lib_file);

            if (routeloader_lib == NULL) return -1;

            if (*first_lib == NULL) {
                *first_lib = routeloader_lib;
            }

            if (*last_lib) {
                (*last_lib)->next = routeloader_lib;
            }

            *last_lib = routeloader_lib;
        }

        void*(*function)(void*) = (void*(*)(void*))routeloader_get_handler(*first_lib, lib_file, lib_handler);

        if (function == NULL) return -1;

        if (route_set_http_handler(route, method, function) == -1) return -1;
    }

    return 0;
}

redirect_t* module_loader_http_redirects_load(const jsmntok_t* token_object) {
    redirect_t* result = NULL;
    redirect_t* first_redirect = NULL;
    redirect_t* last_redirect = NULL;

    if (token_object == NULL) return NULL;
    if (token_object->type != JSMN_OBJECT) goto failed;

    for (jsmntok_t* token_path = token_object->child; token_path; token_path = token_path->sibling) {
        const char* redirect_path = jsmn_get_value(token_path);
        const char* redirect_target = jsmn_get_value(token_path->child);

        redirect_t* redirect = redirect_create(redirect_path, redirect_target);

        if (redirect == NULL) goto failed;

        if (first_redirect == NULL) {
            first_redirect = redirect;
        }

        if (last_redirect) {
            last_redirect->next = redirect;
        }

        last_redirect = redirect;
    }

    result = first_redirect;

    failed:

    if (result == NULL) {
        if (first_redirect) redirect_free(first_redirect);
    }

    return result;
}

route_t* module_loader_websockets_routes_load(routeloader_lib_t** first_lib, const jsmntok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return NULL;
    if (token_object->type != JSMN_OBJECT) goto failed;

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

        if (module_loader_set_websockets_route(first_lib, &last_lib, route, token_path->child) == -1) goto failed;
    }

    result = first_route;

    failed:

    if (result == NULL) {
        if (first_route) route_free(first_route);
    }

    return result;
}

int module_loader_set_websockets_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsmntok_t* token_object) {
    for (jsmntok_t* token_method = token_object->child; token_method; token_method = token_method->sibling) {
        const char* method = jsmn_get_value(token_method);

        jsmntok_t* token_array = token_method->child;

        const char* lib_file = jsmn_get_array_value(token_array, 0);
        const char* lib_handler = jsmn_get_array_value(token_array, 1);

        routeloader_lib_t* routeloader_lib = NULL;

        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib = routeloader_load_lib(lib_file);

            if (routeloader_lib == NULL) return -1;

            if (*first_lib == NULL) {
                *first_lib = routeloader_lib;
            }

            if (*last_lib) {
                (*last_lib)->next = routeloader_lib;
            }

            *last_lib = routeloader_lib;
        }

        void*(*function)(void*) = (void*(*)(void*))routeloader_get_handler(*first_lib, lib_file, lib_handler);

        if (function == NULL) return -1;

        if (route_set_websockets_handler(route, method, function) == -1) return -1;
    }

    return 0;
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

db_t* module_loader_databases_load(const jsmntok_t* token_object) {
    db_t* result = NULL;
    db_t* database_first = NULL;
    db_t* database_last = NULL;

    for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
        const char* database_id = jsmn_get_value(token);
        const char* driver = NULL;

        // printf("%s\n", database_id);

        for (jsmntok_t* token_prop = token->child->child; token_prop; token_prop = token_prop->sibling) {
            const char* key_prop = jsmn_get_value(token_prop);

            if (strcmp(key_prop, "driver") == 0) {
                driver = jsmn_get_value(token_prop->child);
                break;
            }
        }

        if (driver == NULL) {
            log_error("Database driver not found\n");
            goto failed;
        }

        // printf("driver - %s\n", driver);

        db_t* database = NULL;

        #ifdef PostgreSQL_FOUND
        if (strcmp(driver, "postgresql") == 0) {
            database = postgresql_load(database_id, token->child);
        }
        #endif

        #ifdef MySQL_FOUND
        if (strcmp(driver, "mysql") == 0) {
            database = my_load(database_id, token->child);
        }
        #endif

        #ifdef Redis_FOUND
        if (strcmp(driver, "redis") == 0) {
            database = redis_load(database_id, token->child);
        }
        #endif

        if (database == NULL) {
            log_error("Database driver not found\n");
            goto failed;
        }

        if (database_first == NULL) database_first = database;

        if (database_last != NULL) {
            database_last->next = database;
        }
        database_last = database;
    }

    result = database_first;

    failed:

    if (result == NULL) {
        db_free(database_first);
    }

    return result;
}

openssl_t* module_loader_openssl_load(const jsmntok_t* token_object) {
    openssl_t* result = NULL;
    openssl_t* openssl = openssl_create();

    enum fields { FULLCHAIN = 0, PRIVATE, CIPHERS, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "fullchain") == 0) {
            finded_fields[FULLCHAIN] = 1;

            const char* value = jsmn_get_value(token->child);

            openssl->fullchain = (char*)malloc(strlen(value) + 1);

            if (openssl->fullchain == NULL) goto failed;

            strcpy(openssl->fullchain, value);
        }
        else if (strcmp(key, "private") == 0) {
            finded_fields[PRIVATE] = 1;

            const char* value = jsmn_get_value(token->child);

            openssl->private = (char*)malloc(strlen(value) + 1);

            if (openssl->private == NULL) goto failed;

            strcpy(openssl->private, value);
        }
        else if (strcmp(key, "ciphers") == 0) {
            finded_fields[CIPHERS] = 1;

            const char* value = jsmn_get_value(token->child);

            openssl->ciphers = (char*)malloc(strlen(value) + 1);

            if (openssl->ciphers == NULL) goto failed;

            strcpy(openssl->ciphers, value);
        }
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("Error: Fill openssl config\n");
            goto failed;
        }
    }

    if (openssl_init(openssl) == -1) goto failed;

    result = openssl;

    failed:

    if (result == NULL) {
        openssl_free(openssl);
    }

    return result;
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

    server_info_t* server_info = module_loader_server_info_load();

    if (server_info == NULL) goto failed;

    for (jsmntok_t* token_item = token_array->child; token_item; token_item = token_item->sibling) {
        enum required_fields { R_DOMAINS = 0, R_IP, R_PORT, R_ROOT, R_FIELDS_COUNT };
        enum fields { DOMAINS = 0, IP, PORT, ROOT, INDEX, HTTP, WEBSOCKETS, DATABASE, OPENSSL, FIELDS_COUNT };

        int finded_fields[FIELDS_COUNT] = {0};

        const jsmntok_t* token_server = token_item;

        server_t* server = server_create();

        if (server == NULL) goto failed;

        server->info = server_info;

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

                size_t value_length = strlen(value);

                if (value[value_length - 1] == '/') {
                    value_length--;
                }
                server->root = (char*)malloc(value_length + 1);

                if (server->root == NULL) {
                    log_error("Error: Can't alloc memory for root path\n");
                    goto failed;
                }

                strncpy(server->root, value, value_length);

                server->root[value_length] = 0;

                server->root_length = value_length;

                struct stat stat_obj;

                stat(server->root, &stat_obj);

                if (!S_ISDIR(stat_obj.st_mode)) {
                    log_error("Error: Webroot dir not found\n");
                    goto failed;
                }
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
            else if (strcmp(key, "http") == 0) {
                if (token_key->child->type != JSMN_OBJECT) goto failed;

                finded_fields[HTTP] = 1;

                server->http.route = module_loader_http_routes_load(&first_lib, jsmn_object_get_field(token_key->child, "routes"));
                server->http.redirect = module_loader_http_redirects_load(jsmn_object_get_field(token_key->child, "redirects"));

                if (server->http.route == NULL) {
                    log_error("Error: Can't load http routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "websockets") == 0) {
                if (token_key->child->type != JSMN_OBJECT) goto failed;

                finded_fields[WEBSOCKETS] = 1;

                server->websockets.route = module_loader_websockets_routes_load(&first_lib, jsmn_object_get_field(token_key->child, "routes"));

                if (server->websockets.route == NULL) {
                    log_error("Error: Can't load websockets routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "databases") == 0) {
                finded_fields[DATABASE] = 1;

                if (token_key->child->type != JSMN_OBJECT) goto failed;

                server->database = module_loader_databases_load(token_key->child);

                if (server->database == NULL) {
                    log_error("Error: Can't load database\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "openssl") == 0) {
                finded_fields[OPENSSL] = 1;

                if (token_key->child->type != JSMN_OBJECT) goto failed;

                server->openssl = module_loader_openssl_load(token_key->child);

                if (server->openssl == NULL) {
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

        if (finded_fields[HTTP] == 0 && finded_fields[WEBSOCKETS] == 0) {
            log_error("Error: Missing section http and websockets\n");
            goto failed;
        }

        if (module_loader_check_unique_domains(first_server) == -1) goto failed;
    }

    if (first_server == NULL) {
        log_error("Error: Section server not found in config\n");
        return -1;
    }

    if (server_chain_append(first_server, first_lib, server_info, reload_is_hard) == -1) goto failed;

    result = 0;

    failed:

    if (result == -1) {
        server_free(first_server);

        routeloader_free(first_lib);
    }

    return result;
}

int module_loader_mimetype_load() {
    const jsmntok_t* token_root = config_get_section("mimetypes");

    if (token_root->type != JSMN_OBJECT) return -1;

    int result = -1;

    jsmntok_t* token_array = token_root->child;

    size_t table_type_size = token_root->size;
    size_t table_ext_size = 0;

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        table_ext_size += token->child->size;
    }

    // + %25
    table_type_size *= 1.25;
    table_ext_size *= 1.25;

    mimetype_lock();

    if (mimetype_init_type(table_type_size) == -1) goto failed;

    if (mimetype_init_ext(table_ext_size) == -1) goto failed;

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        const char* mimetype = jsmn_get_value(token);

        if (token->child->type != JSMN_ARRAY) goto failed;

        jsmntok_t* token_array = token->child;

        int i = 0;

        for (jsmntok_t* token_item = token_array->child; token_item; token_item = token_item->sibling, i++) {
            if (token_item->type != JSMN_STRING) goto failed;

            const char* extension = jsmn_get_value(token_item);

            if (i == 0) {
                if (mimetype_add(mimetype_get_table_type(), mimetype, extension) == -1) goto failed;
            }

            if (mimetype_add(mimetype_get_table_ext(), extension, mimetype) == -1) goto failed;
        }
    }

    result = 0;

    failed:

    if (result == -1) {
        log_error("Error: Mimetype alloc error\n");
        mimetype_destroy(mimetype_get_table_type());
        mimetype_destroy(mimetype_get_table_ext());
    }

    mimetype_unlock();

    return result;
}

server_info_t* module_loader_server_info_load() {
    server_info_t* result = NULL;

    server_info_t* server_info = server_info_alloc();

    if (server_info == NULL) return NULL;

    const jsmntok_t* token_root = config_get_section("main");

    enum fields { HEAD_BUFFER = 0, CLIENT, ENV, TMP, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsmntok_t* token = token_root->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "read_buffer") == 0) {
            finded_fields[HEAD_BUFFER] = 1;

            const char* value = jsmn_get_value(token->child);

            server_info->read_buffer = atoi(value);
        }
        else if (strcmp(key, "client_max_body_size") == 0) {
            finded_fields[CLIENT] = 1;

            const char* value = jsmn_get_value(token->child);

            server_info->client_max_body_size = atoi(value);
        }
        else if (strcmp(key, "environment") == 0) {
            finded_fields[ENV] = 1;

            const char* value = jsmn_get_value(token->child);

            if (strcmp(value, "dev") == 0) {
                server_info->environment = ENV_DEV;
            }
            else if (strcmp(value, "prod") == 0) {
                server_info->environment = ENV_PROD;
            }
            else {
                log_error("Error: Set up environment\n");
                goto failed;
            }
        }
        else if (strcmp(key, "tmp") == 0) {
            finded_fields[TMP] = 1;

            const char* value = jsmn_get_value(token->child);

            server_info->tmp_dir = (char*)malloc(strlen(value) + 1);

            if (server_info->tmp_dir == NULL) goto failed;

            strcpy(server_info->tmp_dir, value);
        }
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("Error: Fill main section\n");
            goto failed;
        }
    }

    result = server_info;

    failed:

    if (result == NULL) {
        server_info_free(server_info);
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