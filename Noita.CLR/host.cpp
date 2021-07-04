// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

// Standard headers
#include "pch.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>

// Provided by the AppHost NuGet package and installed as an SDK pack

// Header files copied from https://github.com/dotnet/core-setup
#include "..\dotnet-runtime\shared\Microsoft.NETCore.App\5.0.7\coreclr_delegates.h"
#include "..\dotnet-runtime\shared\Microsoft.NETCore.App\5.0.7\hostfxr.h"
using namespace std;

#define STR(s) L ## s
#define CH(c) L ## c
#define DIR_SEPARATOR L'\\'
#define ManagedDllName L"Noita.NET.dll"

using string_t = std::basic_string<char_t>;
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
wstring root_path;
wstring hostfxrPath;


namespace
{
	// Globals to hold hostfxr exports
	hostfxr_initialize_for_runtime_config_fn init_fptr;
	hostfxr_get_runtime_delegate_fn get_delegate_fptr;
	hostfxr_close_fn close_fptr;

	// Forward declarations
	bool load_hostfxr();
	load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* assembly);
}

/********************************************************************************************
 * Function used to load and activate .NET Core
 ********************************************************************************************/

namespace
{
	// Forward declarations
	void* load_library(LPCWSTR);
	void* get_export(void*, const char*);

	void* load_library(LPCWSTR path)
	{
		HMODULE h = ::LoadLibraryW(path);
		assert(h != nullptr);
		return (void*)h;
	}
	void* get_export(void* h, const char* name)
	{
		void* f = ::GetProcAddress((HMODULE)h, name);
		assert(f != nullptr);
		return f;
	}

	// <SnippetLoadHostFxr>
	// Using the nethost library, discover the location of hostfxr and get exports
	bool load_hostfxr()
	{
		// Pre-allocate a large buffer for the path to hostfxr

		// Load hostfxr and get desired exports
		void* lib = load_library(hostfxrPath.c_str());
		init_fptr = (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
		get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
		close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");

		return (init_fptr && get_delegate_fptr && close_fptr);
	}
	// </SnippetLoadHostFxr>

	// <SnippetInitialize>
	// Load and initialize .NET Core and get desired function pointer for scenario
	load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* config_path)
	{
		// Load .NET Core
		void* load_assembly_and_get_function_pointer = nullptr;
		hostfxr_handle cxt = nullptr;
		int rc = init_fptr(config_path, nullptr, &cxt);
		if (rc != 0 || cxt == nullptr)
		{
			std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
			close_fptr(cxt);
			return nullptr;
		}

		// Get the load assembly function pointer
		rc = get_delegate_fptr(
			cxt,
			hdt_load_assembly_and_get_function_pointer,
			&load_assembly_and_get_function_pointer);
		if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
			std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

		close_fptr(cxt);
		return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
	}
	// </SnippetInitialize>
}

#pragma region Init

extern "C" __declspec (dllexport) int Init()
{
	LPTSTR buffer = new WCHAR[_MAX_PATH * 3];
	GetModuleFileName((HINSTANCE)&__ImageBase, buffer, _MAX_PATH * 3);
	root_path = buffer;

	// Get the current executable's directory
	// This sample assumes the managed assembly to load and its runtime configuration file are next to the host
	auto pos = root_path.find_last_of(DIR_SEPARATOR);
	assert(pos != string_t::npos);
	root_path = root_path.substr(0, pos + 1);

	hostfxrPath = root_path + L"hostfxr.dll";

	//
	// STEP 1: Load HostFxr and get exported hosting functions
	//
	if (!load_hostfxr())
	{
		assert(false && "Failure: load_hostfxr()");
		return EXIT_FAILURE;
	}

	//
	// STEP 2: Initialize and start the .NET Core runtime
	//
	const string_t config_path = root_path + STR("Noita.NET.runtimeconfig.json");
	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
	load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path.c_str());
	assert(load_assembly_and_get_function_pointer != nullptr && "Failure: get_dotnet_load_assembly()");

	//
	// STEP 3: Load managed assembly and get function pointer to a managed method
	//
	const string_t dotnetlib_path = root_path + ManagedDllName;
	const char_t* dotnet_type = STR("Noita.NET.Main, Noita.NET");

	// Function pointer to managed delegate with non-default signature
	typedef void (CORECLR_DELEGATE_CALLTYPE* custom_entry_point_fn)();
	custom_entry_point_fn initialize = nullptr;
	int rc = load_assembly_and_get_function_pointer(
		dotnetlib_path.c_str(),
		dotnet_type,
		STR("Initialize") /*method_name*/,
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		(void**)&initialize);
	assert(rc == 0 && initialize != nullptr && "Failure: load_assembly_and_get_function_pointer()");

	initialize();
	return EXIT_SUCCESS;
}
#pragma endregion