# Fork and thread bugs

Example code for http://www.evanjones.ca/fork-is-dangerous.html

* `osx_python_crash_v1.py`: Crashes Python 2.7 and 3.4 on Mac OS X due to libdispatch not working with threads.
* `getaddrinfo_a_fork.c`: Shows that using libraries that don't obviously use threads can still cause problems. Uses `getaddrinfo_a` from glibc on Linux.
* `getaddrinfo_a_example.c`: Example program using `getaddrinfo_a`, used to make sure the basics were working before writing `getaddrinfo_a_fork.c`.
* `mac_crash.c`: A C version of the crash that is triggered by `osx_python_crash_v1.py`.