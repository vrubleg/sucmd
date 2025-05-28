//----------------------------------------------------------------------------------------------------------------------
// Super User v1.2.2 [2024/12/06]
// (C) 2013-2024 Evgeny Vrublevsky <me@veg.by>
//----------------------------------------------------------------------------------------------------------------------

import "windows.hpp";
import "defer.hpp";

__forceinline int Main()
{
	// Prepare a WCHAR buffer.

	HANDLE heap = GetProcessHeap();
	WCHAR* buf = (WCHAR*) HeapAlloc(heap, NULL, sizeof(WCHAR) * 32767);
	if (!buf) { return ERROR_NOT_ENOUGH_MEMORY; }
	defer [&] { HeapFree(heap, NULL, buf); };
	WCHAR* end = buf + 32767 - 1;

	// Parse command line.

	WCHAR* cmdln = GetCommandLineW();

	// Skip current program name.

	bool quote = false;
	while (*cmdln)
	{
		if (*cmdln == '"') { quote = !quote; }
		else if (!quote && (*cmdln == ' ' || *cmdln == '\t')) { break; }
		cmdln++;
	}
	while (*cmdln == ' ' || *cmdln == '\t') { cmdln++; }

	// Change current directory if asked.

	if (*cmdln == '@')
	{
		WCHAR* dst = buf;

		cmdln++;
		quote = false;
		while (*cmdln)
		{
			if (*cmdln == '"') { quote = !quote; }
			else if (!quote && (*cmdln == ' ' || *cmdln == '\t')) { break; }
			else if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
			else { *(dst++) = *(cmdln); }
			cmdln++;
		}
		*dst = NULL;
		while (*cmdln == ' ' || *cmdln == '\t') { cmdln++; }

		if (!SetCurrentDirectoryW(buf)) { return GetLastError(); }
	}

	// Check if SU is elevated.

	bool is_elevated = false;

	{
		HANDLE token = NULL;
		TOKEN_ELEVATION info = {};
		DWORD outsize = 0;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) { return GetLastError(); }
		defer [&] { CloseHandle(token); };
		if (!GetTokenInformation(token, TokenElevation, &info, sizeof(TOKEN_ELEVATION), &outsize)) { return GetLastError(); }
		is_elevated = (bool) info.TokenIsElevated;
	}

	if (!is_elevated)
	{
		// Pass current directory and command to an elevated instance of SU.

		WCHAR* dst = buf;

		*(dst++) = '@'; *(dst++) = '"';
		dst += GetCurrentDirectoryW(MAX_PATH, dst);
		*(dst++) = '"'; *(dst++) = ' ';
		WCHAR* src = cmdln;
		while (*src)
		{
			if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
			*(dst++) = *(src++);
		}
		*dst = NULL;

		SHELLEXECUTEINFO ei = { .cbSize = sizeof(SHELLEXECUTEINFO), .nShow = SW_SHOWNORMAL };

		WCHAR itself[MAX_PATH];
		GetModuleFileNameW(NULL, itself, MAX_PATH);
		ei.lpFile = itself;
		ei.lpVerb = L"runas";
		ei.lpParameters = buf;

		if (!ShellExecuteExW(&ei)) { return GetLastError(); }
	}
	else
	{
		// Execute the command.

		STARTUPINFO si = { .cb = sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi = {};

		// CreateProcess requires lpCommandLine to be writable, so the default command is constructed on stack.
		WCHAR defcmd[] = L"cmd";
		if (!*cmdln) { cmdln = defcmd; }
		si.lpTitle = cmdln;

		if (!CreateProcessW(NULL, cmdln, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { return GetLastError(); }
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return 0;
}

void Start()
{
	DWORD error_code = Main();

	if (error_code && error_code != ERROR_CANCELLED)
	{
		LPTSTR msg = NULL;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, NULL
		);
		MessageBoxW(GetForegroundWindow(), msg ? msg : L"Unknown error.", L"Super User", MB_ICONERROR);
		if (msg) { LocalFree(msg); }
	}

	ExitProcess(error_code);
}
