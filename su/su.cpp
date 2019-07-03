//------------------------------------------------------------------------------
// SU v1.2.1 [2019/07/03]
// Copyright 2013-2019 Evgeny Vrublevsky <veg@tut.by>
//------------------------------------------------------------------------------

#define WINVER 0x0600
#define _WIN32_WINNT WINVER
#include <windows.h>

#define APP_TITLE L"SU"
#define WinApiAssert(ops) if (!(ops)) DieBox();

void DieBox()
{
	DWORD code = GetLastError();
	LPTSTR msg = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, NULL);
	if (msg != NULL)
	{
		MessageBox(GetForegroundWindow(), msg, APP_TITLE, MB_ICONERROR);
		LocalFree(msg);
	}
	else
	{
		MessageBox(GetForegroundWindow(), L"Unknown error", APP_TITLE, MB_ICONERROR);
	}
	ExitProcess(code);
}

bool IsElevated()
{
	HANDLE token = NULL;
	TOKEN_ELEVATION info;
	DWORD outsize = 0;
	WinApiAssert(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token));
	WinApiAssert(GetTokenInformation(token, TokenElevation, &info, sizeof(TOKEN_ELEVATION), &outsize));
	CloseHandle(token);
	return info.TokenIsElevated != 0;
}

int Main()
{
	WCHAR* cmdln = GetCommandLine();
	WCHAR* src;
	WCHAR* dst;

	// Skip current program name and spaces after
	bool quote = false;
	while (*cmdln)
	{
		if (*cmdln == '"') quote = !quote;
		if (!quote && *cmdln == ' ') break;
		cmdln++;
	}
	while (*cmdln == ' ') cmdln++;

	if (*cmdln == '@')
	{
		// Read current directory from command line
		WCHAR currdir[MAX_PATH];
		dst = currdir;
		cmdln++;
		quote = false;
		while (*cmdln)
		{
			if (*cmdln == '"') quote = !quote;
			if (!quote && *cmdln == ' ') break;
			if (*cmdln != '"') *(dst++) = *(cmdln);
			cmdln++;
		}
		*dst = NULL;
		while (*cmdln == ' ') cmdln++;

		WinApiAssert(SetCurrentDirectory(currdir));
	}

	if (!IsElevated())
	{
		// Prepare new command line
		WCHAR args[4096] = {'@', '"'};
		dst = args + 2;
		dst += GetCurrentDirectory(MAX_PATH, dst);
		*(dst++) = '"'; *(dst++) = ' ';
		src = cmdln;
		while (*src) *(dst++) = *(src++);
		*dst = NULL;

		// Run itself with elevated permissions
		SHELLEXECUTEINFO ei;
		ZeroMemory(&ei, sizeof(ei));
		ei.cbSize = sizeof(SHELLEXECUTEINFO);
		ei.nShow = SW_SHOWNORMAL;

		WCHAR itself[MAX_PATH];
		GetModuleFileName(NULL, itself, MAX_PATH);
		ei.lpFile = itself;
		ei.lpVerb = L"runas";
		ei.lpParameters = args;

		if (!ShellExecuteEx(&ei))
		{
			DWORD code = GetLastError();
			if (code != ERROR_CANCELLED) DieBox();
			return code;
		}
		return 0;
	}
	else
	{
		// Start the destination process
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Set console title
		WCHAR defcmd[] = L"cmd";
		if (!*cmdln) cmdln = defcmd;
		si.lpTitle = cmdln;

		// Start the child process.
		WinApiAssert(CreateProcess(NULL, cmdln, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}
}

void Start()
{
	ExitProcess(Main());
}
