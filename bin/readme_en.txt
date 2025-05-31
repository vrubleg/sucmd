Super User v1.2.3 [2025/05/31]
http://veg.by/en/projects/sucmd/

The program allows to run elevated commands without entering your password (but with UAC dialog), same as choosing "Run as administrator" in context menu of a program. In cotrary to the standard mechanism, it also preserves current directory.

Execute install.cmd to copy su64.exe and su32.exe (for x86) or su64a.exe (for ARM64) into appropriate system directories as su.exe to make it globally available.

A simple "su" without arguments runs an elevated "cmd". Any arguments are executed as an elevated command. For example, if you are in the c:\windows\system32\drivers\etc\ directory, and want to edit the hosts file, you can just execute "su notepad hosts" and agree with UAC using Alt+Y.

(C) 2013-2025 Evgeny Vrublevsky <me@veg.by>
