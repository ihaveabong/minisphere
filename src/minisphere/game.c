#include "minisphere.h"
#include "game.h"

#include "fs.h"

struct game
{
	duk_context* dukptr;
	fs_t*        fs;
};

void on_duk_fatal (void* udata, const char* msg);

game_t*
game_open(const char* pathname)
{
	const char* const FILENAMES[] =
	{
		"game.json",
		"game.sgm",
	};
	
	ALLEGRO_CONFIG* conf;
	duk_context*    dukptr = NULL;
	ALLEGRO_FILE*   file;
	fs_t*           fs = NULL;
	game_t*         game;
	size_t          json_size;
	const char*     json_text;
	path_t*         path;

	int i;

	path = path_new(pathname);
	if (!path_resolve(path, NULL))
		goto on_error;
	
	if (!path_is_file(path)) {
		for (i = 0; i < (int)(sizeof FILENAMES) / sizeof(const char*); ++i) {
			path_strip(path);
			path_append(path, FILENAMES[i]);
			if (fs_fexist(fs, path_cstr(path)))
				break;
		}
	}
	if (!fs_fexist(fs, path_cstr(path)))
		goto on_error;

	fs = fs_new(path_cstr(path), path_cstr(path), NULL);
	dukptr = duk_create_heap(NULL, NULL, NULL, NULL, on_duk_fatal);
	
	if (path_filename_cmp(path, "game.json")) {
		json_text = fs_fslurp(fs, "game.json", &json_size);
		duk_push_global_stash(dukptr);
		duk_push_lstring(dukptr, json_text, json_size);
		duk_json_decode(dukptr, -1);
		duk_put_prop_string(dukptr, -2, "gameDescriptor");
	}
	else if (path_filename_cmp(path, "game.sgm")) {
		json_text = fs_fslurp(fs, "game.sgm", &json_size);
		file = al_open_memfile(json_text, (int64_t)json_size, "rb");
		conf = al_load_config_file_f(file);
		
		duk_push_global_stash(dukptr);
		duk_push_object(dukptr);
		duk_push_string(dukptr, al_get_config_value(conf, NULL, "name"));
		duk_put_prop_string(dukptr, -2, "name");
		duk_push_string(dukptr, al_get_config_value(conf, NULL, "author"));
		duk_put_prop_string(dukptr, -2, "author");
		duk_push_string(dukptr, al_get_config_value(conf, NULL, "description"));
		duk_put_prop_string(dukptr, -2, "summary");
		duk_push_string(dukptr, al_get_config_value(conf, NULL, "screen_width"));
		duk_put_prop_string(dukptr, -2, "summary");
		duk_put_prop_string(dukptr, -2, "gameDescriptor");
		duk_pop(dukptr);
		
		al_destroy_config(conf);
		al_fclose(file);
	}

	game = calloc(1, sizeof(game_t));
	game->dukptr = dukptr;
	game->fs = fs;
	return game;

on_error:
	path_free(path);
	if (dukptr != NULL)
		duk_destroy_heap(dukptr);
	fs_free(fs);
	return NULL;
}

void
game_close(game_t* game)
{
	duk_destroy_heap(game->dukptr);
	fs_free(game->fs);
	free(game);
}

duk_context*
game_dukptr(const game_t* game)
{
	return game->dukptr;
}

fs_t*
game_fs(const game_t* game)
{
	return game->fs;
}

void
decode_v1_manifest(game_t* game, const char* filename)
{
	duk_context* dukptr;

	dukptr = game->dukptr;
}

duk_ret_t
decode_v2_manifest(duk_context* ctx)
{
	return 0;
}

void
on_duk_fatal(void* udata, const char* msg)
{
	char* error_text;

	error_text = strnewf("FATAL ERROR: %s\n");

#if defined(MINISPHERE_SPHERUN)
	fprintf("%s\n", error_text);
#else
	al_show_native_message_box(NULL, "Internal Error", "An internal error occurred from which miniSphere couldn't recover.",
		error_text, NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif

	free(error_text);
	exit(EXIT_FAILURE);
}
