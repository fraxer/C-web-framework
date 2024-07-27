#ifndef __HELPERS__
#define __HELPERS__

int helpers_mkdir(const char* path);
int helpers_base_mkdir(const char* base_path, const char* path);
int cmpstr(const char* a, const char* b);
int cmpstr_lower(const char* a, const char* b);
int cmpstrn_lower(const char* a, size_t a_length, const char* b, size_t b_length);
char* create_tmppath(const char*);
const char* file_extention(const char* path);
int cmpsubstr_lower(const char* a, const char* b);
int timezone_offset();

#endif