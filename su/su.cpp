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

__forceinline bool IsSpaceOrNull(const WCHAR c)
{
	return (c == ' ' || c == '\t' || c == 0);
}

__forceinline bool IsCmdBuiltin(const WCHAR* cmdln)
{
	// Null-separated table of CMD built-ins. An additional implicit null at the end serves as a terminator.

	const char* tbl =
		"mklink\0"
		"mkdir\0md\0"
		"rmdir\0rd\0"
		"del\0erase\0"
		"rename\0ren\0"
		"move\0copy\0"
		"date\0time\0"
		"assoc\0ftype\0"
#ifdef _CONSOLE
		"type\0"
		"echo\0"
		"ver\0"
		"set\0"
		"dir\0"
#endif
		"start\0";

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

		if (*tbl == 0 && IsSpaceOrNull(*cmd))
		{
			return true;
		}

		// No, go to the next command in the built-ins table.

		while (*tbl) { tbl++; }
		tbl++;
	}

	return false;
}

__forceinline UINT Main()
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
		else if (!quote && IsSpace(*cmdln)) { break; }
		cmdln++;
	}
	while (IsSpace(*cmdln)) { cmdln++; }

	// Change current directory if asked.

	if (*cmdln == '@')
	{
		WCHAR* dst = buf;

		cmdln++;
		quote = false;
		while (*cmdln)
		{
			if (*cmdln == '"') { quote = !quote; }
			else if (!quote && IsSpace(*cmdln)) { break; }
			else if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
			else { *(dst++) = *(cmdln); }
			cmdln++;
		}
		*dst = NULL;
		while (IsSpace(*cmdln)) { cmdln++; }

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
		DWORD count = GetCurrentDirectoryW(32767 - 4, dst);
		if (!count) { return GetLastError(); }
		if (count > (32767 - 4 - 1)) { return ERROR_BUFFER_OVERFLOW; }
		dst += count;
		*(dst++) = '"'; *(dst++) = ' ';

		const WCHAR* src = cmdln;
		while (*src)
		{
			if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
			*(dst++) = *(src++);
		}
		*dst = NULL;

		WCHAR itself[MAX_PATH];
		if (!GetModuleFileNameW(NULL, itself, MAX_PATH)) { return GetLastError(); }

		SHELLEXECUTEINFO ei = {
			.cbSize = sizeof(SHELLEXECUTEINFO),
#ifdef _CONSOLE
			.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI,
#else
			.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI,
#endif
			.lpVerb = L"runas",
			.lpFile = itself,
			.lpParameters = buf,
			.nShow = SW_SHOWNORMAL,
		};

		if (!ShellExecuteExW(&ei))
		{
			return GetLastError();
		}

#ifdef _CONSOLE

		// Wait until the elevated SU exits.

		if (ei.hProcess)
		{
			defer [&] { CloseHandle(ei.hProcess); };

			WaitForSingleObject(ei.hProcess, INFINITE);

			if (DWORD exit_code = 0; GetExitCodeProcess(ei.hProcess, &exit_code))
			{
				return exit_code;
			}
			else
			{
				return GetLastError();
			}
		}

#endif

		return 0;
	}
	else
	{
		// Execute the command.

		STARTUPINFO si = { .cb = sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi = {};

#ifdef _CONSOLE

		// Detect flags.

		bool has_pause_flag = false;
		bool has_wait_flag = false;

		// This bit magic makes both '-' and '/' accepted.
		while ((cmdln[0] | 0b00000010) == '/' && IsSpaceOrNull(cmdln[2]))
		{
			// Detect /P flag (case insensitive).
			if ((cmdln[1] | 0b00100000) == 'p')
			{
				has_pause_flag = true;
				cmdln += 2;
			}
			// Detect /W flag (case insensitive).
			else if ((cmdln[1] | 0b00100000) == 'w')
			{
				has_wait_flag = true;
				cmdln += 2;
			}
			else
			{
				break;
			}

			while (IsSpace(cmdln[0])) { cmdln++; }
		}

#endif

		// Use "cmd" by default. Set title.

		if (!*cmdln) { cmdln = L"cmd"; }
		si.lpTitle = (WCHAR*) cmdln;

		// Prepare actual command for execution.

		WCHAR* dst = buf;

#ifdef _CONSOLE
		bool run_via_cmd = has_pause_flag || IsCmdBuiltin(cmdln);
#else
		bool run_via_cmd = IsCmdBuiltin(cmdln);
#endif

		if (run_via_cmd)
		{
			// Run CMD built-ins with CMD.

			for (const CHAR* src = "cmd /D /C "; *src; )
			{
				if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
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
					if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
					*(dst++) = '^';
				}

				if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
				*(dst++) = *(src++);
			}

#ifdef _CONSOLE

			if (has_pause_flag)
			{
				for (const CHAR* src = " & pause"; *src; )
				{
					if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
					*(dst++) = *(src++);
				}
			}

#endif

			*dst = NULL;
		}
		else
		{
			// Run the command directly.

			for (const WCHAR* src = cmdln; *src; )
			{
				if (dst >= end) { return ERROR_BUFFER_OVERFLOW; }
				*(dst++) = *(src++);
			}

			*dst = NULL;
		}


		// Note: CreateProcessW requires lpCommandLine to be writable.
		if (!CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { return GetLastError(); }

		defer [&]
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		};

#ifdef _CONSOLE

		// Wait until the process exits.

		if (has_wait_flag)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);

			if (DWORD exit_code = 0; GetExitCodeProcess(pi.hProcess, &exit_code))
			{
				return exit_code;
			}
			else
			{
				return GetLastError();
			}
		}

#endif

		return 0;
	}
}

void Start()
{
	DWORD error_code = Main();

#ifndef _CONSOLE

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

#endif

	ExitProcess(error_code);
}
