#ifndef __LOG_H__
#define __LOG_H__
/**
 * @file log.h
 * @brief function and macros for logging. (needs initalization and fianlization)
 * @details  You can use LOG_<LOG_LEVEL> to output a log in the code.
 * 			 
 * 			 In program you can use LOG_xxx to print a log 
 * 			 
 * 			 There are 6 log levels : fatal, error, warning, notice, info, trace, debug
 *
 * 			 You can use LOG_LEVEL to set above which level, the log should display. 
 *
 * 			 LOG_LEVEL=6 means record all logs, LOG_LEVEL=0 means only fatals.
 *
 * 			 Config file log.conf is used for redirect log to a file. For each log level, we 
 * 			 can define an output file, so that we can seperately record log in different  level in 
 * 			 different files.
 */
/* log levels */
enum{
	/** Use this level when something would stop the program */
	FATAL,
	/** Error level, the routine can not continue */
	ERROR,
	/** Warning level, the routine can continue, but something may be wrong */
	WARNING,
	/** Notice level, there's no error, but something you should notice */
	NOTICE,
	/** Info level, provide some information */
	INFO,
	/** Trace level, trace the program routine and behviours */
	TRACE,
	/** Debug level, detail information used for debugging */
	DEBUG
};

/** @brief initlaization 
 *  @return nothing
 */
int log_init();
/** @brief initlaization 
 *  @return nothing
 */
void log_finalize();

/** @brief	the implementation of write a log
 *  @param	level	the log level
 *  @param	file	the file name of the source code
 *  @param	function	function name
 *  @param	line	line number
 *  @param	fmt		formating string
 *  @return nothing
 */
void log_write(int level, const char* file, const char* function, int line, const char* fmt, ...) 
	__attribute__((format (printf, 5, 6)));

/** @brief helper macros for write a log, do not use it directly */
#define __LOG__(level,fmt,arg...) do{\
	    log_write(level,__FILE__,__FUNCTION__,__LINE__,fmt, ##arg);\
}while(0)

#ifndef LOG_LEVEL
	    #define LOG_LEVEL 6
#endif
#if LOG_LEVEL >= 0
/** @brief print a fatal log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_FATAL(fmt,arg...) __LOG__(FATAL,fmt,##arg)
#else
#        define LOG_FATAL(...)
#endif

#if LOG_LEVEL >= 1
/** @brief print a error log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_ERROR(fmt,arg...) __LOG__(ERROR,fmt,##arg)
#else
#        define LOG_ERROR(...)
#endif

#if LOG_LEVEL >= 2
/** @brief print a warning log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_WARNING(fmt,arg...) __LOG__(WARNING,fmt,##arg)
#else
#        define LOG_WARNING(...)
#endif

#if LOG_LEVEL >= 3
/** @brief print a notice log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_NOTICE(fmt,arg...) __LOG__(NOTICE,fmt,##arg)
#else
#        define LOG_NOTICE(...)
#endif

#if LOG_LEVEL >= 4
/** @brief print a info log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_INFO(fmt,arg...) __LOG__(INFO,fmt,##arg)
#else
#        define LOG_INFO(...)
#endif

#if LOG_LEVEL >= 5
/** @brief print a trace log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_TRACE(fmt,arg...) __LOG__(TRACE,fmt,##arg)
#else
#        define LOG_TRACE(...)
#endif

#if LOG_LEVEL >= 6
/** @brief print a debug log
 *  @param fmt	formating string
 *  @param arg arguments
 *  @return nothing
 */ 
#        define LOG_DEBUG(fmt,arg...) __LOG__(DEBUG,fmt,##arg)
#else
#        define LOG_DEBUG(...)
#endif
#endif
