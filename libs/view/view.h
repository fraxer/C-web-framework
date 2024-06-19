#ifndef __VIEW__
#define __VIEW__

#include "viewcommon.h"

char* render(jsondoc_t* document, const char* storage_name, const char* path_format, ...);
void view_increment(view_t* view);
void view_decrement(view_t* view);

#endif
