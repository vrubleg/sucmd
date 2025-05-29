//----------------------------------------------------------------------------------------------------------------------
// Super User v1.2.2 [2024/12/06]
// (C) 2013-2024 Evgeny Vrublevsky <me@veg.by>
//----------------------------------------------------------------------------------------------------------------------

import "windows.hpp";
import "defer.hpp";

__forceinline bool IsSpace(const WCHAR c)
{
	return (c == ' ' || c == '\t');
}

__forceinline bool IsSpaceOrEnd(const WCHAR c)
{
	return (c == ' ' || c == '\t' || c == 0);
}

__forceinline bool IsCmdBuiltin(const WCHAR* cmdln)
{
	// Null-separated table of CMD built-ins. An additional implicit null at the end serves as a terminator.
	// Excluded commands: title, color, chcp (it's actually a .com file) - those could be SU flags in the future.

	const char* tbl = "start\0md\0mkdir\0rd\0rmdir\0ren\0rename\0move\0del\0erase\0copy\0date\0time\0assoc\0ftype\0"
		"type\0echo\0ver\0pause\0set\0dir\0";

	while (*tbl)
	{
		const WCHAR* cmd = cmdln;

		// Case-insensitive comparison.

		while (*cmd && WCHAR(*cmd | 0b00100000) == WCHAR(*tbl))
		{
			cmd++;
			tbl++;
		}

		// Is it a full match?

		if (*tbl == 0 && (*cmd == 0 || *cmd == ' ' || *cmd == '\t'))
		{
			return true;
		}

		// No, go to the next command in the built-ins table.

		while (*tbl) { tbl++; }
		tbl++;
	}

	return false;
}

__forceinline int Main()
{
	// Prepare a WCHAR buffer.

	HANDLE heap = GetProcessHeap();
	WCHAR* buf = (WCHAR*) HeapAlloc(heap, NULL, sizeof(WCHAR) * 32767);
	if (!buf) { return ERROR_NOT_ENOUGH_MEMORY; }
	defer [&] { HeapFree(heap, NULL, buf); };
	WCHAR* end = buf + 32767 - 1;

	// Parse command line.

	const WCHAR* cmdln = GetCommandLineW();

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

		const WCHAR* src = cmdln;
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

		return 0;
	}
	else
	{
		// Execute the command.

		STARTUPINFO si = { .cb = sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi = {};

		// Detect /P flag (case insensitive).

		bool has_pause_flag = false;

		// This bit magic makes both -p and /P syntaxes supported.
		if ((cmdln[0] | 0b00000010) == '/' && (cmdln[1] | 0b00100000) == 'p' && IsSpaceOrEnd(cmdln[2]))
		{
			has_pause_flag = true;
			cmdln += 2;
			while (IsSpace(cmdln[0])) { cmdln++; }
		}

		// Use "cmd" by default. Set title.

		if (!*cmdln) { cmdln = L"cmd"; }
		si.lpTitle = (WCHAR*) cmdln;

		// Prepare actual command for execution.

		WCHAR* dst = buf;

		if (has_pause_flag || IsCmdBuiltin(cmdln))
		{
			// Run CMD built-ins with CMD.

			for (const CHAR* src = "cmd /D /C "; *src; )
			{
				if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
				*(dst++) = *(src++);
			}

			// Append escaped command.

			quote = false;
			for (const WCHAR* src = cmdln; *src; )
			{
				if (*src == '"')
				{
					quote = !quote;
				}
				else if (!quote && (*src == '&' || *src == '|' || *src == '<' || *src == '>' || *src == '^' || *src == '%'))
				{
					if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
					*(dst++) = '^';
				}

				if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
				*(dst++) = *(src++);
			}

			if (has_pause_flag)
			{
				for (const CHAR* src = " & pause"; *src; )
				{
					if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
					*(dst++) = *(src++);
				}
			}

			*dst = NULL;
		}
		else
		{
			// Run the command directly.

			for (const WCHAR* src = cmdln; *src; )
			{
				if (dst >= end) { return ERROR_FILENAME_EXCED_RANGE; }
				*(dst++) = *(src++);
			}

			*dst = NULL;
		}


		// Note: CreateProcessW requires lpCommandLine to be writable.
		if (!CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { return GetLastError(); }
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return 0;
	}
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
