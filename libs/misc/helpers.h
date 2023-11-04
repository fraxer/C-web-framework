#ifndef __HELPERS__
#define __HELPERS__

char* helpers_read_file(int fd);
long int helpers_file_size(int fd);
void helpers_free_null(void*);
int helpers_mkdir(const char* path);
int helpers_base_mkdir(const char* base_path, const char* path);
int cmpstr_lower(const char*, size_t, const char*, size_t);
int append_to_filefd(int, const char*, size_t);
char* create_tmppath(const char*);
const char* file_extention(const char* path, size_t length);
const char* absolute_path(char* fullpath, const char* rootdir, const char* filepath);

#endif