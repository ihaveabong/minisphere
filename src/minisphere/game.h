#ifndef MINISPHERE__GAME_H__INCLUDED
#define MINISPHERE__GAME_H__INCLUDED

#include "fs.h"

typedef struct game game_t;

game_t*      game_open   (const char* pathname);
void         game_close  (game_t* game);
duk_context* game_dukptr (const game_t* game);
fs_t*        game_fs     (const game_t* game);

#endif // MINISPHERE__GAME_H__INCLUDED
