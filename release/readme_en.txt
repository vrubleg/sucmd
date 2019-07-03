Super User CMD v1.2.1 [2019/07/03]
http://veg.by/en/projects/sucmd/

If you are an active user of the Command Prompt on Windows, you will like this tiny utility. It allows you to run elevated programs without entering your password (but with UAC dialog). You could achieve similar behavior if you choose "Run as administrator" in context menu of a program, but it is not possible to achieve it from command line by default. As an additional useful and important feature, this utility preserves current directory instead of using "c:\windows\system32\". So, if you run su from a Total Commander window, it will open cmd with elevated privileges, and in your current directory. Other similar utilities usually don't preserve current directory, so cmd runs in "c:\windows\system32\", and it is very inconvenient.

Run install.cmd to copy su64.exe and su32.exe to appropriate directories as su.exe ("c:\windows\system32\su.exe" and "c:\windows\syswow64\su.exe" accordingly). After this, the su command will always be available in command line or after pressing Win+R.

"su" without arguments runs elevated cmd. If you provide a command in arguments, "su" will execute it with elevated rights. For example, if you are in the c:\windows\system32\drivers\etc\ directory, and want to edit the hosts file, you can just execute "su notepad hosts" and agree with UAC using Alt+Y.

(С) 2019 Evgeny Vrublevsky <veg@tut.by>