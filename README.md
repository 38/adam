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
