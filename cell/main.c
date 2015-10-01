#include "cell.h"

#include "assets.h"
#include "build.h"

static bool parse_cmdline    (int argc, char* argv[]);
static void print_banner     (bool want_copyright, bool want_deps);
static void print_cell_quote (void);

static path_t* s_in_path;
static path_t* s_out_path;
bool           s_want_source_map;
static char*   s_target_name;

int
main(int argc, char* argv[])
{
	int      retval = EXIT_FAILURE;
	build_t* build = NULL;

	srand((unsigned int)time(NULL));
	
	// establish defaults and parse the command line
	s_in_path = path_new("./");
	s_out_path = path_new("dist/");
	s_target_name = strdup("sphere");
	s_want_source_map = false;
	if (!parse_cmdline(argc, argv))
		goto shutdown;

	print_banner(true, false);
	printf("\n");
	if (!(build = new_build(s_in_path, s_out_path, s_want_source_map)))
		goto shutdown;
	if (!evaluate_rule(build, s_target_name)) goto shutdown;
	if (!run_build(build)) goto shutdown;
	retval = EXIT_SUCCESS;

shutdown:
	path_free(s_in_path);
	path_free(s_out_path);
	free(s_target_name);
	free_build(build);
	return retval;
}

static bool
parse_cmdline(int argc, char* argv[])
{
	int         num_targets = 0;
	const char* short_args;

	int i, j;

	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--help") == 0) {
				print_banner(false, false);
				printf("\n");
				printf("USAGE: cell [--out <dir>] [--in <path>] [target]\n");
				printf("       cell -p <filename> [--in <path>] [target]\n");
				printf("       cell [options] [target]\n");
				printf("\nOPTIONS:\n");
				printf("       --in               Sets the input directory, Cell looks for sources in\n");
				printf("                          the current working directory by default\n");
				printf("       --out              Sets the output directory, 'dist' if not provided\n");
				printf("       --version          Prints the Cell compiler version and exits\n");
				printf("   -p, --make-package     Makes a Sphere package (.spk) for the compiled game\n");
				printf("   -m, --source-map       Generates a source map, which maps compiled assets to\n");
				printf("                          their original sources in the input directory; useful\n");
				printf("                          for debugging\n");
				return false;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				return false;
			}
			else if (strcmp(argv[i], "--in") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(s_in_path);
				s_in_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--out") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(s_out_path);
				s_out_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--explode") == 0) {
				print_cell_quote();
				return false;
			}
			else if (strcmp(argv[i], "--make-package") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(s_out_path);
				s_out_path = path_new(argv[i]);
				if (path_filename_cstr(s_out_path) == NULL) {
					printf("cell: error: %s argument cannot be a directory\n", argv[i - 1]);
					return false;
				}
			}
			else if (strcmp(argv[i], "--source-map") == 0) {
				s_want_source_map = true;
			}
			else {
				printf("cell: error: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			short_args = argv[i];
			for (j = strlen(short_args) - 1; j >= 1; --j) {
				switch (short_args[j]) {
				case 'm': s_want_source_map = true; break;
				case 'p':
					if (++i >= argc) goto missing_argument;
					path_free(s_out_path);
					s_out_path = path_new(argv[i]);
					if (path_filename_cstr(s_out_path) == NULL) {
						printf("cell: error: %s argument cannot be a directory\n", short_args);
						return false;
					}
					break;
				default:
					printf("cell: error: unknown option '-%c'\n", short_args[j]);
					return false;
				}
			}
		}
		else {
			free(s_target_name);
			s_target_name = strdup(argv[i]);
		}

	}
	return true;

missing_argument:
	printf("cell: error: no argument provided for '%s'\n", argv[i - 1]);
	return false;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("Cell %s Sphere Game Compiler %s\n", CELL_VERSION, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("A scriptable build engine for Sphere games.\n");
		printf("(c) 2015 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
		printf("       Cell: %s\n", CELL_VERSION);
		printf("    Duktape: %s\n", DUK_GIT_DESCRIBE);
		printf("       zlib: v%s\n", zlibVersion());
	}
}

static void
print_cell_quote(void)
{
	// yes, these are entirely necessary. :o)
	static const char* const MESSAGES[] =
	{
		"This is the end for you!",
		"Very soon, I am going to explode. And when I do...",
		"Careful now! I wouldn't attack me if I were you...",
		"I'm quite volatile, and the slightest jolt could set me off!",
		"One minute left! There's nothing you can do now!",
		"If only you'd finished me off a little bit sooner...",
		"Ten more seconds, and the Earth will be gone!",
		"Let's just call our little match a draw, shall we?",
	};

	printf("Cell seems to be going through some sort of transformation...\n");
	printf("He's pumping himself up like a balloon!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}