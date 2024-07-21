#pragma once


// Platform.

#ifndef NOBS_PLATFORM
#	if defined(_WIN32)
#		define NOBS_PLATFORM "windows"
#		define NOBS_WINDOWS 1
#		define NOBS_MACOS 0
#		define NOBS_LINUX 0
#	elif defined(__APPLE__)
#		define NOBS_PLATFORM "macos"
#		define NOBS_WINDOWS 0
#		define NOBS_MACOS 1
#		define NOBS_LINUX 0
#	elif defined(__linux__)
#		define NOBS_PLATFORM "linux"
#		define NOBS_WINDOWS 0
#		define NOBS_MACOS 0
#		define NOBS_LINUX 1
#	endif
#endif


// Compiler.

#ifndef NOBS_COMPILER
#	if defined(_MSC_VER)
#		define NOBS_COMPILER "cl", "/nologo"
#		define NOBS_MSVC 1
#		define NOBS_CLANG 0
#		define NOBS_GCC 0
#	elif defined(__clang__)
#		ifdef __cplusplus
#			define NOBS_COMPILER "clang++"
#		else
#			define NOBS_COMPILER "clang"
#		endif
#		define NOBS_MSVC 0
#		define NOBS_CLANG 1
#		define NOBS_GCC 0
#	elif defined(__GNUC__)
#		ifdef __cplusplus
#			define NOBS_COMPILER "g++"
#		else
#			define NOBS_COMPILER "gcc"
#		endif
#		define NOBS_MSVC 0
#		define NOBS_CLANG 0
#		define NOBS_GCC 1
#	endif
#endif


// Headers

#ifndef NOBS_NO_INCLUDES
#	if defined(_WIN32)
#		define NOMINMAX
#		define WIN32_MEAN_AND_LEAN
#		define VC_EXTRALEAN
#		include <Windows.h>
#		include <direct.h>
#	elif defined(__APPLE__) || defined(__linux__)
#		include <errno.h>
#		include <fcntl.h>
#		include <stdarg.h>
#		include <stdlib.h>
#		include <string.h>
#		include <sys/stat.h>
#		include <sys/types.h>
#		include <sys/wait.h>
#		include <time.h>
#		include <unistd.h>
#		include <utime.h>
#		if defined(__APPLE__)
#			include <mach-o/dyld.h>
#			include <copyfile.h>
#		else
#			include <sys/sendfile.h>
#		endif
#	endif
#	include <stdio.h>
#	ifndef __cplusplus
#		include <stdbool.h>
#	endif
#endif


// Types

#if NOBS_WINDOWS
typedef SSIZE_T	NobsTimePoint, ssize_t;
typedef HANDLE	NobsProcessId;
#else
typedef pid_t			NobsProcessId;
typedef struct timespec	NobsTimePoint;
#endif

typedef const char *		NobsString;
typedef struct NobsArray_	NobsArray;

struct NobsArray_
{
	NobsString *	data;
	int				count;
	int				capacity;
};


// Printf like macros.

// TODO: Fix nobs_run_async to properly pipe stdout so that I don't need to flush.
#define nobs_info(...)	{ printf("[INFO] "  __VA_ARGS__); fflush(stdout); } (void)0
#define nobs_error(...)	{ printf("[ERROR] " __VA_ARGS__); fflush(stdout); } (void)0
#define nobs_panic(...)	{ printf("[PANIC] " __VA_ARGS__); fflush(stdout); exit(1); } (void)0


// Helper macros.

#define nobs_foreach_args(param, type, name, stop_cond)	\
	va_list list;										\
	va_start(list, param);								\
	for (type name = va_arg(list, type);				\
		 (stop_cond) ? true : (va_end(list), false);	\
		 name = va_arg(list, type))


// Array functions.

static NobsArray
nobs_array_copy(NobsArray array)
{
	NobsArray copy = { .data = malloc(array.count), .count = array.count, .capacity = array.count };
	memcpy(copy.data, array.data, array.count * sizeof(NobsString));
	return copy;
}

static void
nobs_array_reserve(NobsArray * array, int size)
{
	if (size > array->capacity) {
		array->capacity = array->capacity * 2 > size ? array->capacity * 2 : size;
		array->data     = realloc(array->data, array->capacity * sizeof(NobsString));
	}
}

static void
nobs_array_append_impl(NobsArray * array, ...)
{
	nobs_foreach_args (array, NobsString, arg, arg != 0) {
		nobs_array_reserve(array, array->count + 1);
		array->data[array->count++] = arg;
	}
}

#define nobs_array_append(...) nobs_array_append_impl(__VA_ARGS__, 0)

static void
nobs_array_merge_impl(NobsArray * array, ...)
{
	nobs_foreach_args (array, NobsArray, arg, arg.count != -1) {
		nobs_array_reserve(array, array->count + arg.count);
		memcpy(array->data + array->count, arg.data, arg.count * sizeof(NobsString));
		array->count += arg.count;
	}
}

#define nobs_array_merge(...) nobs_array_merge_impl(__VA_ARGS__, (NobsArray){ .count = -1 })


// Time related utilities.

static NobsTimePoint
nobs_time_get_current(void)
{
#if NOBS_WINDOWS
	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);
	return ticks.QuadPart;
#else
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return time;
#endif
}

static ssize_t
nobs_time_get_elapsed_ns(NobsTimePoint begin, NobsTimePoint end)
{
#if NOBS_WINDOWS
	static ssize_t nobs_ticks_per_seconds = -1;
	if (nobs_ticks_per_seconds == -1) {
		QueryPerformanceFrequency((LARGE_INTEGER *)&nobs_ticks_per_seconds);
	}
	return (end - begin) * 1000000000ll / nobs_ticks_per_seconds;
#else
	return (end.tv_sec - begin.tv_sec) * 1000000000l + (end.tv_nsec - begin.tv_nsec);
#endif
}


// String functions.

static NobsString
nobs_string_join(NobsArray array, NobsString glue)
{
	size_t glue_length = strlen(glue);

	size_t length = 1;
	for (int i = 0; i != array.count; ++i) {
		length += strlen(array.data[i]);
		if (i != array.count - 1) {
			length += glue_length;
		}
	}

	NobsString result = (NobsString)malloc(length);
	char * cursor = (char *)result;
	for (int i = 0; i != array.count; ++i) {
		strcpy(cursor, array.data[i]);
		cursor += strlen(array.data[i]);
		if (i != array.count - 1) {
			strcpy(cursor, glue);
			cursor += glue_length;
		}
	}

	*cursor = '\0';
	return result;
}

static NobsString
nobs_string_concat_impl(NobsString first, ...)
{
	size_t size   = strlen(first);
	char * result = (char *)malloc(size + 1);
	memcpy(result, first, size + 1);
	nobs_foreach_args (first, NobsString, arg, arg != 0) {
		size_t len = strlen(arg);
		result = realloc(result, size + len + 1);
		memcpy(result + size, arg, len + 1);
		size += len;
	}
	return result;
}

#define nobs_string_concat(...) nobs_string_concat_impl(__VA_ARGS__, 0)

static NobsString
nobs_string_format(NobsString format, ...)
{
	char * result;
	size_t size;
	va_list args;

	va_start(args, format);
	size = vsnprintf(0, 0, format, args);
	va_end(args);

	result = (char *)malloc(size + 1);

	va_start(args, format);
	vsnprintf(result, size + 1, format, args);
	va_end(args);

	return result;
}

static NobsString
nobs_string_before(NobsString string, ssize_t index)
{
	ssize_t len = (ssize_t)strlen(string);
	if (index > 0) {
		char * result = malloc(index + 1);
		memcpy(result, string, index);
		result[index] = '\0';
		return result;
	} else if (index < 0) {
		return string;
	}
	return "";
}

static NobsString
nobs_string_after(NobsString string, ssize_t index)
{
	ssize_t len = (ssize_t)strlen(string);
	if (index <= 0) {
		return string;
	} else if (index < len - 1) {
		char * result = malloc(len - index);
		memcpy(result, string + index + 1, len - index - 1);
		result[len - index] = '\0';
		return result;
	}
	return "";
}

static ssize_t
nobs_string_find_last_char(NobsString string, char c)
{
	for (size_t i = strlen(string); i-- != 0; ) {
		if (string[i] == c) {
			return i;
		}
	}
	return -1;
}

static ssize_t
nobs_string_find_first_char(NobsString string, char c)
{
	for (size_t i = 0, iend = strlen(string); i != iend; ++i) {
		if (string[i] == c) {
			return i;
		}
	}
	return -1;
}

#define nobs_string_equal(a, b) strcmp(a, b) == 0

static NobsString
nobs_string_get_elapsed_since(NobsTimePoint begin)
{
#define US_MULT ((ssize_t) 1000)
#define MS_MULT ((ssize_t)(1000 * 1000))
#define S_MULT  ((ssize_t)(1000 * 1000 * 1000))
#define M_MULT  ((ssize_t)(60ll * 1000 * 1000 * 1000))

	ssize_t ns = nobs_time_get_elapsed_ns(begin, nobs_time_get_current());

	if (ns < US_MULT) {
		return nobs_string_format("%d ns", (int)ns);
	} else if (ns < MS_MULT) {
		return nobs_string_format((ns % US_MULT) ? "%.3f us" : "%.0f us", (double)ns / (double)US_MULT);
	} else if (ns < S_MULT) {
		return nobs_string_format((ns % MS_MULT) ? "%.3f ms" : "%.0f ms", (double)ns / (double)MS_MULT);
	} else if (ns < M_MULT) {
		return nobs_string_format((ns % S_MULT) ?  "%.3f s" :  "%.0f s",  (double)ns / (double)S_MULT);
	} else {
		return nobs_string_format((ns % M_MULT) ?  "%.3f m" :  "%.0f m",  (double)ns / (double)M_MULT);
	}

#undef US_MULT
#undef MS_MULT
#undef S_MULT
#undef M_MULT
}


// Filesystem functions.

#if NOBS_WINDOWS
#	define nobs_file_exists(filename) (GetFileAttributesA((filename)) != INVALID_FILE_ATTRIBUTES)
#	define nobs_file_delete(filename) DeleteFileA((filename))
#	define nobs_file_move(from, to)   (MoveFileExA((from), (to), MOVEFILE_REPLACE_EXISTING) != FALSE)
#	define nobs_file_make_dir(path)   (CreateDirectoryA((path), 0) != FALSE)
#else
#	define nobs_file_exists(filename) (access((filename), F_OK) == 0)
#	define nobs_file_delete(filename) unlink((filename))
#	define nobs_file_move(from, to)   (rename((from), (to)) == 0)
#	define nobs_file_make_dir(path)   (mkdir((path), 0775) == 0)
#endif

static void
nobs_file_touch(NobsString path)
{
	if (nobs_file_exists(path)) {
#if NOBS_WINDOWS
		SYSTEMTIME st;
		FILETIME ft;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
		HANDLE file = CreateFile(path, GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING,
								 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		SetFileTime(file, NULL, NULL, &ft);
		CloseHandle(file);
#else
		struct stat st;
		stat(path, &st);
		struct utimbuf tb = { .actime = st.st_atime, .modtime = time(0) };
		utime(path, &tb);
#endif
	} else {
		fclose(fopen(path, "wb"));
	}
}

static NobsString
nobs_file_read(NobsString filename)
{
	NobsString content = { 0 };
	if (nobs_file_exists(filename)) {
		FILE * file = fopen(filename, "rb");
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		content = malloc(size + 1);
		fread((char *)content, 1, size, file);
		((char *)content)[size] = '\0';
		fclose(file);
	}
	return content;
}

static bool
nobs_file_copy(NobsString from, NobsString to)
{
#if NOBS_WINDOWS
	return CopyFile(from, to, FALSE) == TRUE;
#else
	struct stat source_stat = { 0 };
	stat(from, &source_stat);
	int source = open(from, O_RDONLY);
	if (source == -1) {
		return false;
	}
	int destination = creat(to, 0660);
	if (destination == -1) {
		close(source);
		return false;
	}
#if NOBS_MACOS
	int result = fcopyfile(source, destination, 0, COPYFILE_ALL);
#else
	off_t copied = 0;
	int result = sendfile(destination, source, &copied, source_stat.st_size);
#endif
	close(source);
	close(destination);
	chmod(to, source_stat.st_mode);
	return true;
#endif
}

static bool
nobs_file_make_dirs(NobsString directory)
{
	size_t size = strlen(directory);
	char * dir  = (char *)malloc(size + 1);
	memcpy(dir, directory, size + 1);
	for (size_t i = 0; i != size + 1; ++i) {
		if (dir[i] == '/' || dir[i] == '\\' || dir[i] == '\0') {
			char backup = dir[i];
			dir[i] = '\0';
			if (nobs_file_exists(dir) == false && nobs_file_make_dir(dir) == false) {
				return false;
			}
			dir[i] = backup;
		}
	}
	return true;
}

static size_t
nobs_file_get_last_write_time(NobsString filename)
{
#if NOBS_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA attribs;
	BOOL res = GetFileAttributesExA(filename, GetFileExInfoStandard, &attribs);
	return res == FALSE ? 0 : ((size_t)attribs.ftLastWriteTime.dwHighDateTime << 32) +
							   (size_t)attribs.ftLastWriteTime.dwLowDateTime;
#else
	struct stat attr;
	int result = stat(filename, &attr);
	return result < 0 ? 0 : (size_t)attr.st_mtime;
#endif
}


// Process functions.

#if NOBS_WINDOWS
static LPSTR
GetLastErrorAsString(void)
{
	static const DWORD FLAGS = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD error = GetLastError();
	LPSTR message = 0;
	DWORD size = FormatMessageA(FLAGS, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, 0);
	return message;
}
#endif

static int
nobs_proc_wait(NobsProcessId process_id)
{
#if NOBS_WINDOWS
	DWORD result = WaitForSingleObject(process_id, INFINITE);
	if (result == WAIT_FAILED) {
		nobs_panic("Could not wait on child process %p: %s.\n", process_id, GetLastErrorAsString());
	}

	DWORD exit_status;
	if (GetExitCodeProcess(process_id, &exit_status) == FALSE) {
		nobs_panic("Could not get process exit code: %lu.\n", GetLastError());
	}

	CloseHandle(process_id);
	return exit_status;
#else
	int exit_status;
	for (;;) {
		int status = 0;
		if (waitpid(process_id, &status, 0) < 0) {
			nobs_panic("Could not wait on pid %d: %s.\n", process_id, strerror(errno));
		}

		if (WIFEXITED(status)) {
			exit_status = WEXITSTATUS(status);
			break;
		}

		if (WIFSIGNALED(status)) {
			nobs_panic("Command process was terminated by %s.\n", strsignal(WTERMSIG(status)));
		}
	}

	return exit_status;
#endif
}

static NobsProcessId
nobs_proc_run_async(NobsArray command)
{
	NobsString command_line = nobs_string_join(command, " ");
	nobs_info("Executing: %s\n", command_line);

#if NOBS_WINDOWS
	STARTUPINFOA startup_info;
	ZeroMemory(&startup_info, sizeof(startup_info));
	startup_info.cb         = sizeof(STARTUPINFOA);
	startup_info.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startup_info.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
	startup_info.dwFlags   |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION process_info = { 0 };
	BOOL result = CreateProcessA(0, (LPSTR)command_line, 0, 0, TRUE, 0, 0, 0, &startup_info, &process_info);
	if (result == FALSE) {
		nobs_panic("Could not create child process: %s.\n", GetLastErrorAsString());
	}

	CloseHandle(process_info.hThread);
	return process_info.hProcess;
#else
	pid_t pid = fork();
	if (pid < 0) {
		nobs_panic("Could not fork child process: %s.\n", strerror(errno));
	}

	if (pid == 0) {
		// NOTE: Need to copy the incoming command, because we need to mutate it. And the data member
		// of it might contain some immutable memory (like if you're directly using the argc, argv
		// argument of the main)
		NobsArray cmd = { 0 };
		nobs_array_reserve(&cmd, command.count + 1);
		nobs_array_merge(&cmd, command);
		cmd.data[cmd.count] = 0;
		if (execvp(cmd.data[0], (char * const *)cmd.data) < 0) {
			nobs_panic("Could not exec child process: %s.\n", strerror(errno));
		}
	}

	return pid;
#endif
}

static int
nobs_proc_run_sync(NobsArray command)
{
	NobsProcessId process_id = nobs_proc_run_async(command);
	return nobs_proc_wait(process_id);
}

static NobsString
nobs_proc_get_filename(void)
{
#if NOBS_WINDOWS
	char * f;
	_get_pgmptr(&f);
#else
	static char f[2048];
#if NOBS_LINUX
	ssize_t s = readlink("/proc/self/exe", f, sizeof(f));
#else
	unsigned int size = 2048;
	_NSGetExecutablePath(f, &size);
#endif
#endif
	return f;
}


// Environment functions.

#if NOBS_MSVC
#	define nobs_env_get(name)			getenv(name)
#	define nobs_env_set(name, value)	_putenv(nobs_string_format("%s=%s", name, value))
#	define nobs_env_cwd()				_getcwd(0, 0)
#else
#	define nobs_env_get(name)			getenv(name)
#	define nobs_env_set(name, value)	setenv(name, value, 1)
#	define nobs_env_cwd()				getcwd(0, 0)
#endif


// Build stuff

#if NOBS_MSVC
#	define NOBS_DEBUG_SYMBOLS "/Z7"
#	define NOBS_EXE_EXT ".exe"
#	define NOBS_OBJ_EXT ".obj"
#	define NOBS_OUT_EXE(name) nobs_string_concat("/Fe:", name, NOBS_EXE_EXT)
#	define NOBS_OUT_OBJ(name) "/c", nobs_string_concat("/Fo:", name, NOBS_OBJ_EXT)
#else
#	define NOBS_DEBUG_SYMBOLS "-g"
#	define NOBS_EXE_EXT ""
#	define NOBS_OBJ_EXT ".o"
#	define NOBS_OUT_EXE(name) "-o", name
#	define NOBS_OUT_OBJ(name) "-c", "-o", nobs_string_concat(name, NOBS_OBJ_EXT)
#endif

#ifdef NOBS_DEBUG
#	undef NOBS_DEBUG
#	define NOBS_DEBUG NOBS_DEBUG_SYMBOLS
#else
#	define NOBS_DEBUG ""
#endif

#define nobs_needs_rebuild() nobs_needs_rebuild_impl(__FILE__, nobs_proc_get_filename())

static bool
nobs_needs_rebuild_impl(NobsString file, NobsString nobs)
{
	size_t source_time = nobs_file_get_last_write_time(file);
	size_t header_time = nobs_file_get_last_write_time(__FILE__);
	size_t tool_time   = nobs_file_get_last_write_time(nobs);
	return source_time > tool_time || header_time > tool_time;
}

static bool
nobs_out_of_date(ssize_t time, NobsString file)
{
	size_t file_size = nobs_file_get_last_write_time(file);
	if (file_size > time) {
		nobs_info("%s has changed since last build.\n", file);
		return true;
	}
	return false;
}

#define nobs_rebuild(argc, argv)			nobs_rebuild_impl(__FILE__, argc, argv, 0)
#define nobs_rebuild_args(argc, argv, ...)	nobs_rebuild_impl(__FILE__, argc, argv, __VA_ARGS__, 0)

static void
nobs_rebuild_impl(NobsString file, int argc, char ** argv, ...)
{
	NobsTimePoint begin	= nobs_time_get_current();
	NobsString filename	= nobs_proc_get_filename();
	NobsString tool		= nobs_string_before(filename, nobs_string_find_last_char(filename, '.'));
	NobsString tool_exe	= nobs_string_concat(tool, NOBS_EXE_EXT);
	NobsString old_tool	= nobs_string_concat(tool, ".old");

	// Rebuild if necessary.
	ssize_t nobs_time = nobs_file_get_last_write_time(tool_exe);
	if (nobs_out_of_date(nobs_time, file) || nobs_out_of_date(nobs_time, __FILE__)) {
		// Backup the current executable.
		if (nobs_file_move(tool_exe, old_tool) == false) {
			nobs_panic("Couldn't rename %s to %s.\n", tool_exe, old_tool);
		}

		// Create the build command.
		NobsArray command = { 0 };
		nobs_array_append(&command, NOBS_COMPILER, NOBS_DEBUG, NOBS_OUT_EXE(tool), file);

		// Add the optional parameters.
		nobs_foreach_args (argv, NobsString, arg, arg != 0) {
			nobs_array_append(&command, arg);
		}

		// Compile.
		if (nobs_proc_run_sync(command) != 0) {
			nobs_file_move(old_tool, tool_exe);
			nobs_panic("Rebuilding failed. Previous version restored.\n");
		}

		// Log.
		nobs_info("Nobs rebuilt in %s.\n", nobs_string_get_elapsed_since(begin));

		// Cleanup on success.
		nobs_file_delete(old_tool);
#if NOBS_WINDOWS
		nobs_file_delete(nobs_string_concat(tool, ".obj"));
		nobs_file_delete(nobs_string_concat(tool, ".ilk"));
#endif

		// And run the new nobs.
		exit(nobs_proc_run_sync((NobsArray){ .data = (NobsString *)argv, .count = argc }));
	}
}

#if defined(NOBS_EXTENSIONS) || defined(NOBS_RAYLIB_EXTENSION)

// This extension will build Raylib and its GLFW dependency from source as object files,
// and return compiler/linker arguments that can be used to build your executable.
//
// raylib_dir
//		This must point to the directory where Raylib's repository is located. 
//
// output_dir
//		Where Raylib and GLFW will be built.
//
// compile_args
//		List of compile flags that you want to use when building Raylib and GLFW.
//		This can be empty.
//
// The function returns a list of arguments that you can then merge to your executable
// build command line. These arguments include the arguments that will link against
// Raylib and GLFW object files, and their platform dependencies.
//
// Note that on Windows, we have to use a /link argument to do that, so it is advised
// to merge the returned arguments after your compile options, and then append your link
// options after the Raylib's arguments have been merged.
//
// Here is a minimal example:
//
//	int
//	main(int argc, char ** argv)
//	{
//		nobs_rebuild(argc, argv);
//
//		NobsArray compiler_args = { 0 };
//		nobs_array_append(&compiler_args, "-O2");
//
//		NobsArray linker_args = nobs_raylib("./libs/raylib", "./build/libs", compiler_args);
//
//		NobsArray command = { 0 };
//		nobs_array_append(&command, NOBS_OUT_EXE("test_raylib"), "./test_raylib.c");
//
//		nobs_array_merge(&command, compiler_args, linker_args);
//		return nobs_proc_run_sync(command);
//	}
//
static NobsArray
nobs_raylib(NobsString raylib_dir, NobsString output_dir, NobsArray compile_args)
{
	if (nobs_file_exists(raylib_dir) == false) {
		nobs_panic("Raylib directory invalid: '%s' doesn't exist on disk.\n", raylib_dir);
	}

	NobsString glfw_o = nobs_string_format("%s/glfw",   output_dir);
	NobsString rlib_o = nobs_string_format("%s/raylib", output_dir);
	NobsString rlib_c = nobs_string_format("%s/raylib.c", output_dir);
	NobsString args   = nobs_string_concat("// ", nobs_string_join(compile_args, " "));

	bool need_build = nobs_file_exists(rlib_c) == false ||
					  nobs_file_exists(nobs_string_concat(rlib_o, NOBS_OBJ_EXT)) == false ||
					  nobs_file_exists(nobs_string_concat(glfw_o, NOBS_OBJ_EXT)) == false;

	// Even if Raylib's already built, we still want to check if the compile options have changed.
	if (need_build == false) {
		NobsString raylib_unity  = nobs_file_read(rlib_c);
		NobsString previous_args = nobs_string_before(raylib_unity, nobs_string_find_first_char(raylib_unity, '\n'));
		need_build = nobs_string_equal(previous_args, args) == false;
	}

	// Disable all warnings
	compile_args = nobs_array_copy(compile_args);
	nobs_array_append(&compile_args, "-w");

	if (need_build == true) {
		nobs_info("Raylib needs to be rebuilt.\n");

		NobsString rlib_src = nobs_string_format("-I%s/src", raylib_dir);
		NobsString glfw_inc = nobs_string_format("-I%s/src/external/glfw/include", raylib_dir);

		// Generate unity build file for Raylib.

		FILE * file = fopen(rlib_c, "wb+");
		fprintf(file, "%s\n", args);
		fprintf(file, "#include \"config.h\"\n"
					  "#undef SUPPORT_TRACELOG\n" // NOTE: Maybe make this optional?
					  "#include \"rtext.c\"\n"
					  "#include \"rtextures.c\"\n"
					  "#include \"rshapes.c\"\n"
					  "#include \"utils.c\"\n"
					  "#include \"rcore.c\"\n");
		fclose(file);

		// Build Raylib.

		NobsArray command = { 0 };
		nobs_array_append(&command, NOBS_COMPILER, NOBS_OUT_OBJ(rlib_o), rlib_c, "-DPLATFORM_DESKTOP", rlib_src, glfw_inc);
		nobs_array_merge(&command, compile_args);
		if (nobs_proc_run_sync(command) != 0) {
			nobs_panic("Failed building Raylib.\n");
		}

		// Build GLFW.

		command.count = 0;
		nobs_array_append(&command, NOBS_COMPILER, NOBS_OUT_OBJ(glfw_o));
#if NOBS_MACOS
		nobs_array_append(&command, "-x", "objective-c");
#endif
		nobs_array_append(&command, nobs_string_format("%s/src/rglfw.c", raylib_dir), rlib_src, glfw_inc);
		nobs_array_merge(&command, compile_args);
		if (nobs_proc_run_sync(command) != 0) {
			nobs_panic("Failed building Glfw.\n");
		}
	}

	// Create the compiler / linker arguments so that the caller can use Raylib / GLFW.

	NobsArray arguments = { 0 };
	nobs_array_append(&arguments, nobs_string_format("-I%s/src", raylib_dir),
								  nobs_string_format("-I%s/src/external/glfw/include", raylib_dir));
#if NOBS_WINDOWS
	nobs_array_append(&arguments, "/link",
								  "Gdi32.lib",
								  "Shell32.lib",
								  "User32.lib",
								  "Winmm.lib");
#elif NOBS_MACOS
	nobs_array_append(&arguments, "-framework", "Cocoa",
								  "-framework", "CoreFoundation",
								  "-framework", "IOKit");
#endif
	nobs_array_append(&arguments, nobs_string_concat(glfw_o, NOBS_OBJ_EXT),
								  nobs_string_concat(rlib_o, NOBS_OBJ_EXT));
	return arguments;
}

#endif
