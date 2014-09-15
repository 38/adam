#ifndef __ADAM_H__
/** @file  adam.h
 *  @brief Global Intalization and Finalization Functions
 *  @mainpage 
 *
 *  ADAM is short for Another Dalvik Abstract Machine, an experimental static analysis based security checker for Android Apps.<br/>
 *
 *  <h1>How to compile</h1>
 *  Basicly 
 *  @code cmake . && make @endcode
 *
 *  To enable/disable building a tool package 
 *  @code cmake -Dbuild_<package_name>=yes|no . && make @endcode
 *
 *  To change the log level and the optimization level
 *  @code L=<log-level> O=<opt-level> cmake . && make @endcode
 *
 *  To show the compile flags 
 *  @code make show-flags @endcode
 *
 *  <h1> How to use </h1>
 *
 *	Currently only Adam Debugger is avaliable.<br/>
 *
 *	This package is enabled by default, if you want to enable it explicitly, run 
 *	@code cmake - Dbuild_adb=yes . @endcode before compilation. <br/>
 *
 *	You can use ADB to test the analyzer if you compile adam with ADB package. <br>
 *
 *	If the visualization feature is enabled, you can find two PostScript graphs (code.ps and  frameinfo.ps)
 *	which is actually the current machine status. <br/>
 *
 *	For more infomation about how to use ADB shell, use the command @code (help) @endcode in ADB shell to print help message 
 *
 *	<h1>Test</h1>
 *	Prepare the test data, use make data to download a sample input of ADAM from 
 *	<a href="http://www.cs.utah.edu/~haohou/adam/data.tar.gz">Here</a><br>
 *
 *	Then run test cases use @code make test @endcode<br>
 *
 *	<h1>Dalvik Disassemble Tool</h1>
 *  ADAM takes the output of <a href="https://github.com/38/dex2sex">dex2sex</a> which produces S-Expression represention of dalvik disassmebly code.
 *
 *  You can get a compiled binary from <a href="http://www.cs.utah.edu/~haohou/adam/dex2sex.tar.gz">here</a>.
 *
 *  To disassmble a APK package, simple use dex2sex &lt;apk-package&gt;.
 *
 */
#include <stdint.h>

#include <stringpool.h>
#include <log.h>
#include <vector.h>

#include <dalvik/dalvik.h>
#include <cesk/cesk.h>
#include <bci/bci.h>
#include <tag/tag.h>
/** 
 * @brief Intialize libadam
 * @return nothing
 **/
int adam_init(void);
/** 
 * @brief Finalize libadam
 * @return nothing
 **/
void adam_finalize(void);
#endif
