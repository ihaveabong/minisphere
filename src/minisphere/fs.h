#ifndef MINISPHERE__FS_H__INCLUDED
#define MINISPHERE__FS_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include "path.h"

#define SPHERE_PATH_MAX 1024

typedef
enum fs_whence
{
	FS_SEEK_SET,
	FS_SEEK_CUR,
	FS_SEEK_END,
} fs_whence_t;

typedef struct fs      fs_t;
typedef struct fs_file fs_file_t;
typedef struct stat    fs_stat_t;

fs_t*      fs_new      (const char* root_dirname, const char* game_dirname, const char* user_dirname);
fs_t*      fs_ref      (fs_t* fs);
void       fs_free     (fs_t* fs);

fs_file_t* fs_fopen    (fs_t* fs, const char* filename, const char* mode);
void       fs_fclose   (fs_file_t* file);
bool       fs_feof     (fs_file_t* file);
bool       fs_fexist   (fs_t* fs, const char* filename);
bool       fs_fread    (fs_file_t* file, void* buffer, size_t size);
bool       fs_fseek    (fs_file_t* file, int64_t offset, fs_whence_t whence);
void*      fs_fslurp   (fs_t* fs, const char* filename, size_t* p_size);
bool       fs_fspew    (fs_t* fs, const char* filename, const void* data, size_t size);
int64_t    fs_ftell    (fs_file_t* file);
bool       fs_fwrite   (fs_file_t* file, const void* data, size_t size);
path_t*    fs_realpath (const char* filename, const char* base_dirname, bool legacy);
bool       fs_stat     (fs_t* fs, const char* filename, fs_stat_t* p_stats);

#endif // MINISPHERE__FS_H__INCLUDED
