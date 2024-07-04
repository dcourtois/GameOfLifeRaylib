#define NOBS_EXTENSIONS
#include "./nobs.h"


#ifdef CONFIGURED
#	include "./config.h"
#endif


int
main(int argc, char ** argv)
{
#ifndef CONFIGURED
	if (argc != 2) {
		nobs_panic("Missing path to raylib Usage: nobs <path_to_raylib_repository>\n");
	}

	nobs_info("Generating default config.\n");
	FILE * config = fopen("./config.h", "w");
	fprintf(config, "#define RAYLIB_DIR \"%s\"", argv[1]);
	fclose(config);

	nobs_rebuild_args(argc, argv, "-DCONFIGURED");

	return
#else
	if (nobs_file_get_last_write_time("./config.h") > nobs_file_get_last_write_time(nobs_proc_get_filename())) {
		nobs_file_touch(__FILE__);
	}

	nobs_rebuild_args(argc, argv, "-DCONFIGURED");

	NobsTimePoint begin = nobs_time_get_current();

	nobs_file_make_dirs("./build/bin");
	nobs_file_make_dirs("./build/libs");

	NobsArray arguments = { 0 };
	// nobs_array_append(&arguments, "/Ox");
	// nobs_array_append(&arguments, "/Z7", "/DDEBUG");

	NobsArray raylib = nobs_raylib(RAYLIB_DIR, "./build/libs", arguments);

	nobs_array_append(&arguments, "/W4", "/wd4505");

	NobsArray command = { 0 };
	nobs_array_append(&command, NOBS_COMPILER, NOBS_OUT_EXE("./build/bin/game_of_life"), "./game_of_life.c");
	nobs_array_merge(&command, arguments, raylib);

	int result = nobs_proc_run_sync(command);
	nobs_info("Build %s in %s.\n", result ? "failed" : "succeeded", nobs_string_get_elapsed_since(begin));
	return result;
#endif
}
