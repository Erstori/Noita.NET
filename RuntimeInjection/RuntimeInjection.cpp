// RuntimeInjection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

#include <Windows.h>
#include <process.h>
#include <TlHelp32.h>
#include <string>
#include <memoryapi.h>
#include <iostream>
using namespace std;

#define DIR_SEPARATOR L'\\'

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

DWORD FindProcessId(const wstring& processName);
BOOL InjectDLL(DWORD dwProcessID, LPCWSTR dllFullPath);

int main()
{
	LPTSTR buffer = new TCHAR[_MAX_PATH * 3];
	GetModuleFileName((HINSTANCE)&__ImageBase, buffer, _MAX_PATH * 3);
	wstring root_path = buffer;

	auto pos = root_path.find_last_of(DIR_SEPARATOR);
	root_path = root_path.substr(0, pos + 1);

	// Find Noita Process Id
	int pId;
	if (
		(pId = FindProcessId(L"noita.exe")) == 0 &&
		(pId = FindProcessId(L"noita_dev.exe") == 0)
		)
	{
		return 1;
	}

	wstring noitaCLRPath = root_path + L"Noita.CLR.dll";

	InjectDLL(pId, noitaCLRPath.c_str());
	// Alloc

}

DWORD FindProcessId(const wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

BOOL GrantDebugPriv(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tokenPriv;
	LUID luid;

	// open the access token associated witht the current process
	// hToken is a pointer to a handle that identifies the newly opened access token when the function returns.
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	// retrieve the locally unique identifier of the specified privilege
	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		return FALSE;
	}

	// assign the requested privilege enable state to the token privilege structure
	tokenPriv.PrivilegeCount = 1;
	tokenPriv.Privileges[0].Luid = luid;
	tokenPriv.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : NULL;

	// enable the requested privilege in the specified access token
	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tokenPriv,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL InjectDLL(DWORD dwProcessID, LPCWSTR dllFullPath)
{
	// enable the SE_DEBUG_NAME privilege for the current process
	GrantDebugPriv(NULL, SE_DEBUG_NAME, TRUE);

	// if the DLL path does not exist, the DLL does not exists, exit
	if (_waccess_s(dllFullPath, 0) != 0)
	{
		return FALSE;
	}

	// open a handle to the target process given then PID, exit if null
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD, FALSE, dwProcessID);
	if (hProcess == NULL)
	{
		return FALSE;
	}

	// the size of our soon to be allocated memory; 260 bytes
	DWORD dwMemSize = (DWORD)MAX_PATH;

	// allocate 260 bytes of memory in the target process; returns the base address of that section
	LPVOID lpRemoteMemory = VirtualAllocEx(
		hProcess,
		NULL,
		dwMemSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	// exit if memory allocation failed
	if (lpRemoteMemory == NULL)
	{
		return FALSE;
	}

	// write the path of the DLL into the newly allocateed memory region
	if (!WriteProcessMemory(
		hProcess,
		lpRemoteMemory,
		(LPCVOID)dllFullPath,
		dwMemSize,
		NULL))
	{
		return FALSE;
	}

	// get a handle to kernel32.dll, needed to get the address of LoadLibrary; exit if failed
	HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
	if (hKernel32 == NULL)
	{
		return FALSE;
	}

	// get the address of LoadLibrary given the handle to kernel32.dll
	// will be used to load the DLL into the target process as a thread
	LPVOID lpLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
	if (lpLoadLibrary == NULL)
	{
		return FALSE;
	}

	// create a thread that will load the DLL into the target process using LoadLibrary
	HANDLE hRemoteThread = CreateRemoteThread(
		hProcess,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)lpLoadLibrary,
		lpRemoteMemory,
		NULL,
		NULL);

	// if the handle to the new thread is null, exit
	if (hRemoteThread == NULL)
	{
		return FALSE;
	}

	// wait for the thread to finish executing
	WaitForSingleObject(hRemoteThread, INFINITE);

#pragma region MyRegion
	// get a handle to Noita.CLR.dll; exit if failed

	HMODULE hNoitaCLR = LoadLibrary(dllFullPath);
	if (hNoitaCLR == NULL)
	{
		return FALSE;
	}

	// get the address of Init given the handle to Noita.CLR.dll
	LPVOID lpInit = GetProcAddress(hNoitaCLR, "Init");
	if (lpInit == NULL)
	{
		return FALSE;
	}

	HANDLE hRemoteThreadNoitaCLRInit = CreateRemoteThread(
		hProcess,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)lpInit,
		NULL,
		NULL,
		NULL);

	// if the handle to the new thread is null, exit
	if (hRemoteThreadNoitaCLRInit == NULL)
	{
		return FALSE;
	}

	// wait for the thread to finish executing
	WaitForSingleObject(hRemoteThreadNoitaCLRInit, INFINITE);
#pragma endregion


	// release the allocated memory where the DLL pat resides
	// close the handle to the target process
	VirtualFreeEx(hProcess, (LPVOID)lpRemoteMemory, 0, MEM_RELEASE);
	CloseHandle(hProcess);

	return TRUE;
}