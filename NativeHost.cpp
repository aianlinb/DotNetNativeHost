/*
	Copyright (C) 2022 aianlinb
	
			2022/8/5
*/

#include <assert.h>

// From nuget: Microsoft.NETCore.DotNetAppHost (Only available in .Net project) or
// C:\Program Files\dotnet\packs\Microsoft.NETCore.App.Host.win-x64\6.0.6\runtimes\win-x64\native
#include "nethost.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"

#ifdef _WIN32 // Windows

#include <direct.h>
#define STR(s) L ## s
//#define DIR_SEPARATOR L'\\'
#define cd _wchdir

#include <ole2.h> // If dont use OleInitialize(Line:116), this can be replaced by <windows.h>

void* load_library(const char_t* path) {
	HMODULE h = LoadLibraryW(path);
	assert(h != nullptr);
	return (void*)h;
}
void* get_export_func(void* h, const char* name) {
	void* f = GetProcAddress((HMODULE)h, name);
	assert(f != nullptr);
	return f;
}
#else // Unix

#include <limits.h>
#define MAX_PATH PATH_MAX
#include <unistd.h>
#define cd chdir

#define STR(s) s
//#define DIR_SEPARATOR '/'

#include <dlfcn.h>
void* load_library(const char_t* path) {
	void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
	assert(h != nullptr);
	return h;
}
void* get_export_func(void* h, const char* name) {
	void* f = dlsym(h, name);
	assert(f != nullptr);
	return f;
}
#endif

#if defined(_WIN32)
int __cdecl wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
	char_t buffer[MAX_PATH];
	size_t buffer_size = sizeof(buffer) / sizeof(char_t);
	int32_t hr = get_hostfxr_path(buffer, &buffer_size, nullptr); // Usually is C:\Program Files\dotnet\host\fxr\6.0.7\hostfxr.dll in Windows
	assert(hr == 0);
	if (hr != 0)
		return hr;

	void* lib = load_library(buffer);
	auto hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)get_export_func(lib, "hostfxr_initialize_for_dotnet_command_line");
	auto hostfxr_run_app = (hostfxr_run_app_fn)get_export_func(lib, "hostfxr_run_app");
	/* Change current working directory
	hr = cd(argv[1]);
	assert(hr == 0);
	*/
	hostfxr_handle hh;
	hr = hostfxr_initialize_for_dotnet_command_line(argc - 1, ((const char_t**)argv + 1), nullptr, &hh);
	assert(hr == 0);
	if (hr != 0)
		return hr;

	return hostfxr_run_app(hh); // Run application

	
	/*
	==============================================================
	The following code has a bug that causes the WPF application to fail to start.
	WPF calls Assembly.GetEntryAssembly() for getting resources which return null in this case.
	A quick fix is to set Application.ResourceAssembly to Assembly.GetExecutingAssembly()
	  in App() constructor before MainWindows is created.
	But a more easy way is to use the code above directly instead.
	==============================================================

	
	auto hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)get_export_func(lib, "hostfxr_initialize_for_runtime_config");
	auto hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)get_export_func(lib, "hostfxr_get_runtime_delegate");
	auto hostfxr_close = (hostfxr_close_fn)get_export_func(lib, "hostfxr_close");
	
	hr = hostfxr_initialize_for_runtime_config(STR("D:\\test\\bin\\Debug\\net6.0-windows\\test.runtimeconfig.json"), nullptr, &hh);
	assert(hr == 0);

	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;
	hr = hostfxr_get_runtime_delegate(hh, hdt_load_assembly_and_get_function_pointer, (void**)& load_assembly_and_get_function_pointer);
	assert(hr == 0);
	assert(load_assembly_and_get_function_pointer != nullptr);
	hr = hostfxr_close(hh);
	assert(hr == 0);

	void (*Main)();
	//System.Func`1[[System.Int32, System.Private.CoreLib]], System.Private.CoreLib
	hr = load_assembly_and_get_function_pointer(STR("D:\\test\\bin\\Debug\\net6.0-windows\\test.dll"), STR("test.Program, test"), STR("Main"), STR("System.Action"), nullptr, (void**)&Main); // Change Program to App in Windows Application
	assert(hr == 0);
	assert(Main != nullptr);

#if defined(_WIN32)
	assert(OleInitialize(nullptr) == S_OK); // For COM (like WPF or Clipboard)
#endif
	Main(); // Run application
	*/
}