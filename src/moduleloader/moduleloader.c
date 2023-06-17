#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../config/config.h"
#include "../log/log.h"
#include "../redirect/redirect.h"
#include "../route/route.h"
#include "../route/routeloader.h"
#include "../domain/domain.h"
#include "../server/server.h"
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
#include "../json/json.h"
#include "moduleloader.h"
    #include <stdio.h>

int module_loader_init_modules();
int module_loader_servers_load(int);
int module_loader_thread_workers_load();
int module_loader_thread_handlers_load();
int module_loader_reload_is_hard();
int module_loader_mimetype_load();
int module_loader_set_http_route(routeloader_lib_t**, routeloader_lib_t**, route_t* route, const jsontok_t*);
int module_loader_set_websockets_route(routeloader_lib_t**, routeloader_lib_t**, route_t* route, const jsontok_t*);
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
    const jsontok_t* token_main = config_get_section("main");
    if (!token_main) {
        log_error("Reload error: Section main not found\n");
        return -1;
    }

    if (!json_is_object(token_main)) {
        log_error("Reload error: Section main must be object type\n");
        return -1;
    }

    const jsontok_t* token_reload = json_object_get(token_main, "reload");
    if (!token_reload) {
        log_error("Reload error: Field reload not found in section main\n");
        return -1;
    }

    if (!json_is_string(token_reload)) {
        log_error("Reload error: Field reload must be string type\n");
        return -1;
    }

    const char* reload = json_string(token_reload);

    if (strcmp(reload, "hard") == 0)
        return 1;
    if (strcmp(reload, "soft") == 0)
        return 0;

    log_error("Reload error: Reload type must be hard or soft\n");

    return -1;
}

route_t* module_loader_http_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return NULL;
    if (!json_is_object(token_object)) goto failed;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* route_path = json_it_key(&it);

        route_t* route = route_create(route_path);

        if (route == NULL) goto failed;

        if (first_route == NULL)
            first_route = route;

        if (last_route)
            last_route->next = route;

        last_route = route;

        if (module_loader_set_http_route(first_lib, &last_lib, route, json_it_value(&it)) == -1) goto failed;
    }

    result = first_route;

    failed:

    if (result == NULL) {
        if (first_route) route_free(first_route);
    }

    return result;
}

int module_loader_set_http_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object) {
    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* method = json_it_key(&it);

        jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) return -1;

        const char* lib_file = json_string(json_array_get(token_array, 0));
        const char* lib_handler = json_string(json_array_get(token_array, 1));

        routeloader_lib_t* routeloader_lib = NULL;

        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib = routeloader_load_lib(lib_file);

            if (routeloader_lib == NULL) return -1;

            if (*first_lib == NULL)
                *first_lib = routeloader_lib;

            if (*last_lib)
                (*last_lib)->next = routeloader_lib;

            *last_lib = routeloader_lib;
        }

        void(*function)(void*, void*);

        *(void**)(&function) = routeloader_get_handler(*first_lib, lib_file, lib_handler);

        if (function == NULL) return -1;

        if (route_set_http_handler(route, method, function) == -1) return -1;
    }

    return 0;
}

redirect_t* module_loader_http_redirects_load(const jsontok_t* token_object) {
    redirect_t* result = NULL;
    redirect_t* first_redirect = NULL;
    redirect_t* last_redirect = NULL;

    if (token_object == NULL) return NULL;
    if (!json_is_object(token_object)) goto failed;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_value = json_it_value(&it);
        if (!token_value) goto failed;
        if (!json_is_string(token_value)) goto failed;

        const char* redirect_path = json_it_key(&it);
        const char* redirect_target = json_string(token_value);

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

route_t* module_loader_websockets_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object) {
    route_t* result = NULL;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return NULL;
    if (!json_is_object(token_object)) goto failed;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* route_path = json_it_key(&it);

        route_t* route = route_create(route_path);

        if (route == NULL) goto failed;

        if (first_route == NULL)
            first_route = route;

        if (last_route)
            last_route->next = route;

        last_route = route;

        if (module_loader_set_websockets_route(first_lib, &last_lib, route, json_it_value(&it)) == -1) goto failed;
    }

    result = first_route;

    failed:

    if (result == NULL) {
        if (first_route) route_free(first_route);
    }

    return result;
}

int module_loader_set_websockets_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object) {
    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* method = json_it_key(&it);

        jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) return -1;

        const char* lib_file = json_string(json_array_get(token_array, 0));
        const char* lib_handler = json_string(json_array_get(token_array, 1));

        routeloader_lib_t* routeloader_lib = NULL;

        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib = routeloader_load_lib(lib_file);

            if (routeloader_lib == NULL) return -1;

            if (*first_lib == NULL)
                *first_lib = routeloader_lib;

            if (*last_lib)
                (*last_lib)->next = routeloader_lib;

            *last_lib = routeloader_lib;
        }

        void(*function)(void*, void*);

        *(void**)(&function) = routeloader_get_handler(*first_lib, lib_file, lib_handler);

        if (function == NULL) return -1;

        if (route_set_websockets_handler(route, method, function) == -1) return -1;
    }

    return 0;
}

domain_t* module_loader_domains_load(const jsontok_t* token_array) {
    domain_t* result = NULL;
    domain_t* first_domain = NULL;
    domain_t* last_domain = NULL;

    for (jsonit_t it = json_init_it(token_array); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_domain = json_it_value(&it);
        if (!json_is_string(token_domain)) goto failed;

        domain_t* domain = domain_create(json_string(token_domain));

        if (domain == NULL) goto failed;

        if (first_domain == NULL)
            first_domain = domain;

        if (last_domain)
            last_domain->next = domain;

        last_domain = domain;
    }

    result = first_domain;

    failed:

    if (result == NULL) {
        domain_free(first_domain);
    }

    return result;
}

db_t* module_loader_databases_load(const jsontok_t* token_object) {
    db_t* result = NULL;
    db_t* database_first = NULL;
    db_t* database_last = NULL;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_array = json_it_value(&it);
        if (json_array_size(token_array) == 0) {
            log_error("Database driver not found\n");
            goto failed;
        }

        jsontok_t* token_item = json_array_get(token_array, 0);
        jsontok_t* token_driver = json_object_get(token_item, "driver");
        if (!json_is_string(token_driver)) {
            log_error("Database driver not found\n");
            goto failed;
        }

        const char* database_id = json_it_key(&it);
        const char* driver = json_string(token_driver);

        db_t* database = NULL;

        #ifdef PostgreSQL_FOUND
        if (strcmp(driver, "postgresql") == 0) {
            database = postgresql_load(database_id, token_array);
        }
        #endif

        #ifdef MySQL_FOUND
        if (strcmp(driver, "mysql") == 0) {
            database = my_load(database_id, token_array);
        }
        #endif

        #ifdef Redis_FOUND
        if (strcmp(driver, "redis") == 0) {
            database = redis_load(database_id, token_array);
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

openssl_t* module_loader_openssl_load(const jsontok_t* token_object) {
    openssl_t* result = NULL;
    openssl_t* openssl = openssl_create();

    enum fields { FULLCHAIN = 0, PRIVATE, CIPHERS, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* key = json_it_key(&it);
        const jsontok_t* token_value = json_it_value(&it);

        if (strcmp(key, "fullchain") == 0) {
            if (!json_is_string(token_value)) {
                log_error("Openssl field fullchain must be string type\n");
                goto failed;
            }

            finded_fields[FULLCHAIN] = 1;

            const char* value = json_string(token_value);

            openssl->fullchain = (char*)malloc(strlen(value) + 1);
            if (openssl->fullchain == NULL) goto failed;

            strcpy(openssl->fullchain, value);
        }
        else if (strcmp(key, "private") == 0) {
            if (!json_is_string(token_value)) {
                log_error("Openssl field private must be string type\n");
                goto failed;
            }

            finded_fields[PRIVATE] = 1;

            const char* value = json_string(token_value);

            openssl->private = (char*)malloc(strlen(value) + 1);
            if (openssl->private == NULL) goto failed;

            strcpy(openssl->private, value);
        }
        else if (strcmp(key, "ciphers") == 0) {
            if (!json_is_string(token_value)) {
                log_error("Openssl field ciphers must be string type\n");
                goto failed;
            }

            finded_fields[CIPHERS] = 1;

            const char* value = json_string(token_value);

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
    const jsontok_t* token_object = config_get_section("servers");
    if (!json_is_object(token_object)) return -1;

    int result = -1;
    server_t* first_server = NULL;
    server_t* last_server = NULL;
    routeloader_lib_t* first_lib = NULL;

    server_info_t* server_info = module_loader_server_info_load();
    if (server_info == NULL) goto failed;

    for (jsonit_t it_servers = json_init_it(token_object); !json_end_it(&it_servers); json_next_it(&it_servers)) {
        enum required_fields { R_DOMAINS = 0, R_IP, R_PORT, R_ROOT, R_FIELDS_COUNT };
        enum fields { DOMAINS = 0, IP, PORT, ROOT, INDEX, HTTP, WEBSOCKETS, DATABASE, OPENSSL, FIELDS_COUNT };

        int finded_fields[FIELDS_COUNT] = {0};

        server_t* server = server_create();
        if (server == NULL) goto failed;

        server->info = server_info;

        if (first_server == NULL)
            first_server = server;

        if (last_server)
            last_server->next = server;

        last_server = server;

        const jsontok_t* token_server = json_it_value(&it_servers);
        if (!json_is_object(token_server)) return -1;

        for (jsonit_t it_server = json_init_it(token_server); !json_end_it(&it_server); json_next_it(&it_server)) {
            const char* key = json_it_key(&it_server);
            const jsontok_t* token_value = json_it_value(&it_server);

            if (strcmp(key, "domains") == 0) {
                finded_fields[DOMAINS] = 1;

                if (!json_is_array(token_value)) goto failed;

                server->domain = module_loader_domains_load(token_value);

                if (server->domain == NULL) {
                    log_error("Error: Can't load domains\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "ip") == 0) {
                finded_fields[IP] = 1;

                if (!json_is_string(token_value)) goto failed;

                server->ip = inet_addr(json_string(token_value));
            }
            else if (strcmp(key, "port") == 0) {
                finded_fields[PORT] = 1;

                if (!json_is_int(token_value)) goto failed;

                server->port = json_int(token_value);
            }
            else if (strcmp(key, "root") == 0) {
                finded_fields[ROOT] = 1;

                if (!json_is_string(token_value)) goto failed;

                const char* value = json_string(token_value);

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

                if (!json_is_string(token_value)) goto failed;

                server->index = server_create_index(json_string(token_value));

                if (server->index == NULL) {
                    log_error("Error: Can't alloc memory for index file\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "http") == 0) {
                if (!json_is_object(token_value)) goto failed;

                finded_fields[HTTP] = 1;

                server->http.route = module_loader_http_routes_load(&first_lib, json_object_get(token_value, "routes"));
                server->http.redirect = module_loader_http_redirects_load(json_object_get(token_value, "redirects"));

                if (server->http.route == NULL) {
                    log_error("Error: Can't load http routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "websockets") == 0) {
                if (!json_is_object(token_value)) goto failed;

                finded_fields[WEBSOCKETS] = 1;

                server->websockets.route = module_loader_websockets_routes_load(&first_lib, json_object_get(token_value, "routes"));

                if (server->websockets.route == NULL) {
                    log_error("Error: Can't load websockets routes\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "databases") == 0) {
                finded_fields[DATABASE] = 1;

                if (!json_is_object(token_value)) goto failed;

                server->database = module_loader_databases_load(token_value);

                if (server->database == NULL) {
                    log_error("Error: Can't load database\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "openssl") == 0) {
                finded_fields[OPENSSL] = 1;

                if (!json_is_object(token_value)) goto failed;

                server->openssl = module_loader_openssl_load(token_value);

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
    const jsontok_t* token_object = config_get_section("mimetypes");
    if (!json_is_object(token_object)) return -1;

    int result = -1;
    size_t table_type_size = json_object_size(token_object);
    size_t table_ext_size = 0;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) return -1;

        table_ext_size += json_array_size(token_array);
    }

    // + %25
    table_type_size *= 1.25;
    table_ext_size *= 1.25;

    mimetype_lock();

    if (mimetype_init_type(table_type_size) == -1) goto failed;
    if (mimetype_init_ext(table_ext_size) == -1) goto failed;

    for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
        const char* mimetype = json_it_key(&it_object);
        const jsontok_t* token_array = json_it_value(&it_object);

        if (!json_is_array(token_array)) goto failed;

        for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
            const int* index = json_it_key(&it_array);
            const jsontok_t* token_value = json_it_value(&it_array);
            if (!json_is_string(token_value)) goto failed;

            const char* extension = json_string(token_value);

            if (*index == 0) {
                if (mimetype_add(mimetype_get_table_type(), mimetype, extension) == -1) goto failed;
            }

            if (mimetype_add(mimetype_get_table_ext(), extension, mimetype) == -1) goto failed;
        }
    }

    result = 0;

    mimetype_update();

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
    server_info_t* server_info = server_info_create();
    if (server_info == NULL) return NULL;

    const jsontok_t* token_main = config_get_section("main");
    if (!token_main) return NULL;
    if (!json_is_object(token_main)) return NULL;

    enum fields { HEAD_BUFFER = 0, CLIENT, ENV, TMP, GZIP, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsonit_t it = json_init_it(token_main); !json_end_it(&it); json_next_it(&it)) {
        const char* key = json_it_key(&it);
        const jsontok_t* token_value = json_it_value(&it);

        if (strcmp(key, "read_buffer") == 0) {
            if (!json_is_int(token_value)) goto failed;

            finded_fields[HEAD_BUFFER] = 1;

            server_info->read_buffer = json_int(token_value);
        }
        else if (strcmp(key, "client_max_body_size") == 0) {
            if (!json_is_int(token_value)) goto failed;

            finded_fields[CLIENT] = 1;

            server_info->client_max_body_size = json_int(token_value);
        }
        else if (strcmp(key, "environment") == 0) {
            if (!json_is_string(token_value)) goto failed;

            finded_fields[ENV] = 1;

            const char* value = json_string(token_value);

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
            if (!json_is_string(token_value)) goto failed;

            finded_fields[TMP] = 1;

            const char* value = json_string(token_value);
            size_t value_length = strlen(value);
            if (value[value_length - 1] == '/') {
                value_length--;
            }

            server_info->tmp_dir = (char*)malloc(value_length + 1);
            if (server_info->tmp_dir == NULL) goto failed;

            strncpy(server_info->tmp_dir, value, value_length);
            server_info->tmp_dir[value_length] = 0;
        }
        else if (strcmp(key, "gzip") == 0) {
            if (!json_is_array(token_value)) goto failed;

            finded_fields[GZIP] = 1;

            gzip_mimetype_t* last_gzip = NULL;

            for (jsonit_t it_gzip = json_init_it(token_value); !json_end_it(&it_gzip); json_next_it(&it_gzip)) {
                const jsontok_t* token_gzip_value = json_it_value(&it_gzip);
                if (!json_is_string(token_gzip_value)) goto failed;

                gzip_mimetype_t* gzip_mimetype = server_gzip_mimetype_create(json_string(token_gzip_value));
                if (gzip_mimetype == NULL) goto failed;

                if (last_gzip == NULL)
                    server_info->gzip_mimetype = gzip_mimetype;
                else
                    last_gzip->next = gzip_mimetype;

                last_gzip = gzip_mimetype;
            }
        }
    }

    if (finded_fields[TMP] == 0) {
        const char* value = "/tmp";

        server_info->tmp_dir = (char*)malloc(strlen(value) + 1);
        if (server_info->tmp_dir == NULL) goto failed;

        strcpy(server_info->tmp_dir, value);
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
    const jsontok_t* token_main = config_get_section("main");
    if (!token_main) {
        log_error("Reload error: Section main not found\n");
        return -1;
    }

    if (!json_is_object(token_main)) {
        log_error("Reload error: Section main must be object type\n");
        return -1;
    }

    const jsontok_t* token_workers = json_object_get(token_main, "workers");
    if (!token_workers) {
        log_error("Reload error: Field workers not found in section main\n");
        return -1;
    }

    if (!json_is_int(token_workers)) {
        log_error("Reload error: Field workers must be int type\n");
        return -1;
    }

    const int count = json_int(token_workers);
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
    const jsontok_t* token_main = config_get_section("main");
    if (!token_main) {
        log_error("Reload error: Section main not found\n");
        return -1;
    }

    if (!json_is_object(token_main)) {
        log_error("Reload error: Section main must be object type\n");
        return -1;
    }

    const jsontok_t* token_threads = json_object_get(token_main, "threads");
    if (!token_threads) {
        log_error("Reload error: Field threads not found in section main\n");
        return -1;
    }

    if (!json_is_int(token_threads)) {
        log_error("Reload error: Field threads must be int type\n");
        return -1;
    }

    const int count = json_int(token_threads);
    if (count <= 0) {
        log_error("Thread error: Set the number of threads\n");
        return -1;
    }

    if (thread_handler_run(count) == -1) return -1;

    return 0;
}