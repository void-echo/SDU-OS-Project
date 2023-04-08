# SDU-OS-Project

This project is made possible by Void Echo and Three Liang @ Shandong University, School of Software Engineering.

The project is a simple OS, which is based on the book "Nachos: Not Another Completely Heuristic Operating System" by Andrew S. Tanenbaum.

All the code is under the CC0 1.0 Universal (CC0 1.0) Public Domain Dedication.

## How to run

> before you run the code, please make sure you have accomplished the following > steps:
> 
> 1. using one linux system, and install the gcc and g++ compiler
> 2. have full access to the root directory of the system
> 3. installed the gcc-mips cross-compiler.


First, run git clone command.

```bash
git clone https://github.com/void-echo/SDU-OS-Project
cd ./SDU-OS-Project
```

then you can make one or more components of the OS.


## How to develop our enhanced Nachos

In nachos, there are many components, and you can enhance the OS by enhancing one or more components.

Note that you should not change the ./code/machine, ./code/thread, ./code/monitor directories directly, because they are the core of the OS.

> if you do, the OS can also work, but this enhance style is not recommended.

For each lab, there is a README.md file, which contains the description of the lab. Please read the README.md file before getting started.

When you decide to change the code in ./code/machine, ./code/thread or ./code/monitor, you should firstly copy the file to be changed to lab directory, and then change the code there.

Under the directory of each lab, there are Makefile and Makefile.local files. Usually, you do not need to change the Makefile file, but you may need to change the Makefile.local file. to make sure your changes can be compiled.

The Makefile.local of each lab is different, and they should be like this:

```makefile
# Makefile.local for lab 2
ifndef MAKEFILE_THREADS_LOCAL
define MAKEFILE_THREADS_LOCAL
yes
endef


SFILES = switch$(HOST_LINUX).s


# If you add new files, you need to add them to CCFILES,
# you can define CFILES if you choose to make .c files instead.

CCFILES = main.cc\
	list.cc\
	scheduler.cc\
	synch.cc\
	synchlist.cc\
	system.cc\
	thread.cc\
	utility.cc\
	threadtest.cc\
	synchtest.cc\
	interrupt.cc\
	sysdep.cc\
	stats.cc\
	timer.cc

INCPATH += -I- -I../lab2 -I../threads -I../machine # find in ./ directory, if not found, find in ../threads, etc. Usually, this line is the only line you need to change.

DEFINES += -DTHREADS # This line defines the macro THREADS, which is equal to `#define THREADS`. `DEFINES` is a special variable in Makefile.

endif # MAKEFILE_THREADS_LOCAL
```

Note that Nachos cannot run correctly on Windows (at least for now), and if you modify the code on Windows, just make sure the code can be compiled on Linux.

If you develop on Linux, before you commit your code, **please run `make clean` first to clean the object files and executable files.** It is not a good idea to commit binaries to Github.

> You may also notice that there are some files named `placeholder`, which are all empty files. These files are used to make sure the directory structure is correct. Git cannot commit empty directories, so we use these placeholder to force git to remember the structure.

## Better Practice: Write On Windows, Run On Linux

If you are using Windows, you can use Visual Studio Code to write the code, and use VMWare Shared Folders to share the code between Windows and Linux.

This is a good practice, because you can use the powerful IDE to write the code, and use Linux to run the code. **No slow down**, no bugs, no delay, no problem.

but there are some limitations: you cannot use symbolic links, but don't worry, I had already solved this problem by editing the `Makefile.common` file and adding some shell scripts.

> You can now develop on Windows, and run on Linux without any need to change the `Makefile` or run any shell scripts. But, if you want enable Shared Folders, **you may have to configure your own `VMWare Settings`** to enable this feature. Chances are that you may fail to enable this feature, but don't worry, you may find [a solution](https://askubuntu.com/questions/1239726/windows-host-shared-folder-in-ubuntu-20-04) from this link.


⚠ By the way, If you find some parts too difficult to develop, feel free to contact me, I will try my best to provide help, or we can exchange our responsible project parts.

**Develop and enjoy!**
