//------------------------------------------------------------------------------
// Su v1.0 [18.02.2013]
// Copyright 2013 Evgeny Vrublevsky <veg@tut.by>
//------------------------------------------------------------------------------
#include <windows.h>

// Spaces between cmd&ver added to the title, we should avoid it
WCHAR init_tpl[] = L"/K title cmd&cd /D ";
WCHAR info_tpl[] = L"ver|findstr .";

int main()
{
	WCHAR* cmdln = GetCommandLine();

	// Skip current program name and spaces after
	bool quote = false;
	while (*cmdln)
	{
		if (*cmdln == L'"') quote = !quote;
		if (!quote && *cmdln == L' ') break;
		cmdln++;
	}
	while (*cmdln == L' ') cmdln++;

	// Build args for cmd
	WCHAR  args[4096];
	WCHAR* src;
	WCHAR* dst = args;

	// Copy init part
	src = init_tpl;
	while (*src) *(dst++) = *(src++);

	// Add current directory
	int shift = GetCurrentDirectory(MAX_PATH, dst);
	dst += shift;
	if (!shift)
	{
		// Going to the root of the system disk
		*(dst++) = L'\\';
	}
	*(dst++) = L'&';

	if (!*cmdln)
	{
		// Display windows version
		src = info_tpl;
		while (*src) *(dst++) = *(src++);
	}
	else
	{
		// Copy user's command as is for execution
		src = cmdln;
		while (*src) *(dst++) = *(src++);
	}

	*dst = NULL;

	// Execute the program
	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = L"runas";
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.lpFile = L"cmd";
	ShExecInfo.lpParameters = args;
	ShExecInfo.nShow = SW_SHOWNORMAL;
	ShExecInfo.hInstApp = NULL;
	return ShellExecuteEx(&ShExecInfo);
}

void __cdecl start()
{
	ExitProcess(main());
}
