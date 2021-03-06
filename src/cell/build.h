#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

typedef struct build build_t;

build_t* build_new     (const path_t* source_path, const path_t* out_path);
void     build_free    (build_t* build);
bool     build_clean   (build_t* build);
bool     build_eval    (build_t* build, const char* filename);
bool     build_package (build_t* build, const char* filename);
bool     build_run     (build_t* build, bool want_debug, bool rebuild_all);

#endif // CELL__BUILD_H__INCLUDED
