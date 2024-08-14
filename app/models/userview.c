#include "db.h"
#include "userview.h"

static const char* __dbid = "postgresql";

userview_t* userview_get(userview_get_params_t* params) {
    userview_t* user = userview_instance();
    if (user == NULL)
        return NULL;

    dbinstance_t dbinst = dbinstance(__dbid);

    if (!dbinst.ok)
        return user;

    dbresult_t result = dbquery(&dbinst,
        "SELECT "
            "id, "
            "username "
        "FROM "
            "\"user\" "
        "WHERE "
            "id = %d",

        mparam_int(&params->id)
    );

    if (!dbresult_ok(&result))
        goto failed;

    failed:

    dbresult_free(&result);

    return user;
}