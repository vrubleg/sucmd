//------------------------------------------------------------------------------
// Su v1.1 [19.02.2013]
// Copyright 2013 Evgeny Vrublevsky <veg@tut.by>
//------------------------------------------------------------------------------
#include <windows.h>

#define APP_TITLE L"SU"

void ErrorBox(DWORD code)
{
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

	if (*cmdln != '@')
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
			if (code != ERROR_CANCELLED) ErrorBox(code);
			return code;
		}
		return 0;
	}
	else
	{
		// Read current directory
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
		if(!CreateProcess(NULL, cmdln, NULL, NULL, FALSE, 0, NULL, currdir, &si, &pi))
		{
			DWORD code = GetLastError();
			ErrorBox(code);
			return code;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}
}

void __cdecl Start()
{
	ExitProcess(Main());
}
