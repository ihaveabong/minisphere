#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fs.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "path.h"

struct fs
{
	unsigned int refcount;
	path_t*      root_path;
	path_t*      game_path;
	path_t*      user_path;
};

struct fs_file
{
	fs_t* fs;
	FILE* file_ptr;
};

static const char* resolve_path (const fs_t* fs, const char* filename);

static path_t* system_path;

fs_t*
fs_new(const char* root_filename, const char* game_filename, const char* user_dirname)
{
	fs_t* fs;

	fs = calloc(1, sizeof(fs_t));
	fs->root_path = path_strip(path_new(root_filename));
	fs->game_path = path_strip(path_new(game_filename));
	if (user_dirname != NULL)
		fs->user_path = path_new(user_dirname);
	return fs_ref(fs);
}

fs_t*
fs_ref(fs_t* fs)
{
	++fs->refcount;
	return fs;
}

void
fs_free(fs_t* fs)
{
	if (fs == NULL || --fs->refcount > 0)
		return;
	path_free(fs->root_path);
	path_free(fs->game_path);
	path_free(fs->user_path);
	free(fs);
}

fs_file_t*
fs_fopen(fs_t* fs, const char* filename, const char* mode)
{
	fs_file_t*  file;
	FILE*       fp;
	const char* full_name;

	if (!(full_name = resolve_path(fs, filename)))
		goto on_error;
	if (!(fp = fopen(full_name, mode)))
		goto on_error;
	
	file = calloc(1, sizeof(fs_file_t));
	file->fs = fs_ref(fs);
	file->file_ptr = fp;
	return file;

on_error:
	return NULL;
}

void
fs_fclose(fs_file_t* file)
{
	if (file == NULL)
		return;
	fclose(file->file_ptr);
	fs_free(file->fs);
	free(file);
}

bool
fs_feof(fs_file_t* file)
{
	return feof(file->file_ptr) != 0;
}

bool
fs_fexist(fs_t* fs, const char* filename)
{
	fs_stat_t stats;

	return fs_stat(fs, filename, &stats);
}

bool
fs_fread(fs_file_t* file, void* buffer, size_t size)
{
	if (fread(buffer, 1, size, file->file_ptr) != size)
		return false;
	return true;
}

bool
fs_fseek(fs_file_t* file, int64_t offset, fs_whence_t whence)
{
	if (offset > LONG_MAX) {
		errno = EOVERFLOW;
		return false;
	}

	return fseek(file->file_ptr, (long)offset, whence) == 0;
}

void*
fs_fslurp(fs_t* fs, const char* filename, size_t* p_size)
{
	void*      buffer;
	fs_file_t* file;
	int64_t    file_size;

	if (!(file = fs_fopen(fs, filename, "rb")))
		goto on_error;
	fs_fseek(file, 0, FS_SEEK_END);
	if ((file_size = fs_ftell(file)) < 0 || file_size > SIZE_MAX)
		goto on_error;
	buffer = malloc((size_t)file_size);
	if (!fs_fread(file, buffer, (size_t)file_size))
		goto on_error;
	fs_fclose(file);
	*p_size = (size_t)file_size;
	return buffer;

on_error:
	fs_fclose(file);
	free(buffer);
	return NULL;
}

bool
fs_fspew(fs_t* fs, const char* filename, const void* data, size_t size)
{
	fs_file_t* file;

	if (!(file = fs_fopen(fs, filename, "wb")))
		goto on_error;
	if (!fs_fwrite(file, data, size))
		goto on_error;
	fs_fclose(file);
	return true;

on_error:
	fs_fclose(file);
	return false;
}

int64_t
fs_ftell(fs_file_t* file)
{
	return ftell(file->file_ptr);
}

bool
fs_fwrite(fs_file_t* file, const void* data, size_t size)
{
	if (fwrite(data, 1, size, file->file_ptr) != size)
		return false;
	return true;
}

path_t*
fs_realpath(const char* filename, const char* base_dirname, bool legacy)
{
	// note: fs_realpath() collapses '../' path hops unconditionally, as per SphereFS
	//       spec. this ensures a game can't subvert the sandbox by navigating outside
	//       of its directory via a symbolic link.

	path_t* base_path = NULL;
	path_t* path;
	char*   prefix;

	path = path_new(filename);
	if (path_is_rooted(path))  // absolute path?
		return path;

	if (legacy && path_num_hops(path) >= 1 && path_hop_cmp(path, 0, "~")) {
		path_remove_hop(path, 0);
		path_insert_hop(path, 0, "@");
	}

	base_path = path_new_dir(base_dirname != NULL ? base_dirname : "./");
	if (path_num_hops(path) == 0)
		path_rebase(path, base_path);
	else if (path_hop_cmp(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_collapse(path, true);
	}
	else if (path_hop_cmp(path, 0, "#") || path_hop_cmp(path, 0, "~")) {
		prefix = strdup(path_hop(path, 0));
		path_remove_hop(path, 0);
		path_collapse(path, true);
		path_insert_hop(path, 0, prefix);
		free(prefix);
	}
	else
		path_rebase(path, base_path);
	path_collapse(path, true);
	path_free(base_path);
	return path;
}

bool
fs_stat(fs_t* fs, const char* filename, fs_stat_t* p_stats)
{
	const char* full_name;

	if (!(full_name = resolve_path(fs, filename)))
		return false;
	return stat(full_name, p_stats) == 0;
}

static const char*
resolve_path(const fs_t* fs, const char* filename)
{
	static char* resolved_name = NULL;

	path_t* path;

	path = path_new(filename);
	if (path_is_rooted(path))
		goto on_error;

	if (path_num_hops(path) == 0)
		path_rebase(path, fs->root_path);
	else if (path_hop_cmp(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->game_path);
	}
	else if (path_hop_cmp(path, 0, "#")) {
		path_remove_hop(path, 0);
		path_rebase(path, system_path);
	}
	else if (path_hop_cmp(path, 0, "~"))
		if (fs->user_path == NULL)
			// no user directory, so "~/..." is a sandbox violation.
			goto on_error;
		else {
			path_remove_hop(path, 0);
			path_rebase(path, fs->user_path);
		}
	else
		path_rebase(path, fs->root_path);

	free(resolved_name);
	resolved_name = strdup(path_cstr(path));
	path_free(path);
	return resolved_name;

on_error:
	path_free(path);
	errno = EACCES;
	return NULL;
}
