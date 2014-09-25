ADAM
========
Another Dalvik Abstract Machine

This is an experimental static analysis based security checker for Android Apps. 

How to compile
---
Simplely `cmake . && make`

More Options
---
To enable/disable building a tool package `cmake -Dbuild_<package_name>=yes|no .` 
	
To change the log level and the optimization level, `L=<log-level> O=<opt-level> cmake .`

Use `make show-flags` to print the compile flags

Test
---
Prepare the test data, use `make data` to download a sample input of ADAM from [http://www.cs.utah.edu/~haohou/adam/data.tar.gz](http://www.cs.utah.edu/~haohou/adam/data.tar.gz).

Then run test cases `make test`

How to use
---
Currently only Adam Debugger is avaliable. 

This package is enabled by default, if you want to enable it explicitly, run `cmake - Dbuild_adb=yes .` before compilation.

You can use ADB to test the analyzer if you compile adam with ADB package.

Dalvik Disassemble Tool
---
ADAM takes the output of [dex2sex](https://github.com/38/dex2sex) which produces S-Expression represention of dalvik disassmebly code.

You can get a compiled binary from [here](http://www.cs.utah.edu/~haohou/adam/dex2sex.tar.gz).

To disassmble a APK package, simple use `dex2sex <apk-package>`. 

Documentation
---
You can either extract doxygen documentation from the source code by `make docs` 

or visit the online version documentation at [http://www.cs.utah.edu/~haohou/adam/html/](http://www.cs.utah.edu/~haohou/adam/html)


Logging
---

The default log config file is defined in include/constants.h.  By default it is log.cfg.

If you do not have log.cfg in adam/ directory, there will be noisy output.  The log.cfg file defines where to redirect log output of various levels (WARNING, ERROR, etc).

Debugger
---

Obtain android.jar and place in adam/test/de2sex/bin/lib/
Obtain dex2sex from my other repo.

How do I prepare a .dex file for analysis?

java2s path/to/code output/dir 
OR
apk2s path/to/apk output/dir

How do I start the debugger?

After building, run ./bin/adb, but no program is loaded yet.

How to load a program into the debugger?

(load "path/to/dir/with/sexpressionified-bytecode-files")

How do I inject into a start state?

Autocompletion is supported!!!

(frame/new  [fully/qualified/classname/method(params,...) [object fully/qualified/returntype]])

How do I view the frame?

(frame/info)

Or as a dot file!!!

(frame/dot)

How do I set parameter values?

(frame/set v1 + - Z)

(frame/allocate CLASSPATH)

(frame/alloc v1 "Some String") <- not yet supported, Cambells task

How do I add commands to the debugger?

Add the Command to adb/main.c with a handler.

TODO
---

data flow graph:
for each data tag: print file/line of occurences

tracking variable name changes: 
if it changes drastically, then it may be an attempt to hide data





