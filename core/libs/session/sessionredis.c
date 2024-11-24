#include "db.h"
#include "log.h"
#include "appconfig.h"
#include "helpers.h"
#include "sessionredis.h"

static char* __create(const char* data);
static char* __get(const char* session_id);
static int __update(const char* session_id, const char* data);
static int __destroy(const char* session_id);
static void __remove_expired(void);

session_t* sessionredis_init() {
    session_t* session = malloc(sizeof * session);
    if (session == NULL) return NULL;

    session->create = __create;
    session->get = __get;
    session->update = __update;
    session->destroy = __destroy;
    session->remove_expired = __remove_expired;

    return session;
}

char* __create(const char* data) {
    char* session_id = session_create_id();
    if (session_id == NULL) {
        log_error("sessionfile__create: alloc memory for id failed\n");
        return NULL;
    }

    if (!__update(session_id, data)) {
        free(session_id);
        session_id = NULL;
    }

    return session_id;
}

char* __get(const char* session_id) {
    dbinstance_t* dbinst = dbinstance(appconfig()->sessionconfig.host_id);
    if (dbinst == NULL) return NULL;

    dbresult_t* result = dbqueryf(dbinst, "GET %s", session_id);

    dbinstance_free(dbinst);
    char* data = NULL;

    if (!dbresult_ok(result))
        goto failed;

    const db_table_cell_t* field = dbresult_field(result, NULL);
    if (field == NULL)
        goto failed;

    if (field->length > 0)
        data = strdup(field->value);

    failed:

    dbresult_free(result);

    return data;
}

int __update(const char* session_id, const char* data) {
    dbinstance_t* dbinst = dbinstance(appconfig()->sessionconfig.host_id);
    if (dbinst == NULL) return 0;

    dbresult_t* result = dbqueryf(dbinst, "SET %s %s EX %d", session_id, data, appconfig()->sessionconfig.lifetime);

    dbinstance_free(dbinst);
    int res = 0;
    const db_table_cell_t* field = NULL;

    if (!dbresult_ok(result))
        goto failed;

    field = dbresult_field(result, NULL);
    if (field == NULL)
        goto failed;
    if (!cmpstr_lower(field->value, "OK"))
        goto failed;

    res = 1;

    failed:

    dbresult_free(result);

    return res;
}

int __destroy(const char* session_id) {
    dbinstance_t* dbinst = dbinstance(appconfig()->sessionconfig.host_id);
    if (dbinst == NULL) return 0;

    dbresult_t* result = dbqueryf(dbinst, "DEL %s", session_id);

    dbinstance_free(dbinst);
    dbresult_free(result);

    return 1;
}

void __remove_expired(void) {}
