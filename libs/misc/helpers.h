#ifndef __HELPERS__
#define __HELPERS__

char* helpers_read_file(int fd);
long int helpers_file_size(int fd);
void helpers_free_null(void*);
int helpers_mkdir(const char* path);
int helpers_base_mkdir(const char* base_path, const char* path);
int cmpstr(const char* a, const char* b);
int cmpstr_lower(const char* a, const char* b);
int cmpstrn_lower(const char* a, size_t a_length, const char* b, size_t b_length);
int append_to_filefd(int, const char*, size_t);
char* create_tmppath(const char*);
const char* file_extention(const char* path);
int cmpsubstr_lower(const char* a, const char* b);
int timezone_offset();

#endif