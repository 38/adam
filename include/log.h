#ifndef __LOG_H__
#define __LOG_H__
/*
 * log.h: Macros for logging. 
 * You can use LOG_<LOG_LEVEL> to output a log in the code.
 * There are 6 log levels, you can find them in the enumerate below
 * e.g.:
 *       p = malloc(1024);
 *       if(NULL == p) LOG_ERROR("something was wrong!");
 * 
 * By default, all log will be printed to stderr. But you can write a 
 * log.conf in the configure directory (defined in constants.h).
 * 
 * Sample configure file :
 *      # This is a comment
 *      # level    file name      mode
 *        DEBUG   /tmp/debug.log    a
 *        WARNING /dev/null             # WARNING will be ignored
 *
 *        default <stderr>              # other level will be printed to stderr
 *
 *
 * THIS FILE NEEDS INITIALIZATION BEFORE YOU CAN USE THE FUNCTION
 */

/* log levels */
enum{
    FATAL,
    ERROR,
    WARNING,
    NOTICE,
    INFO,
    TRACE,
    DEBUG
};

/* Initialization & Finalization */
void log_init();
void log_finalize();

/* the implementation of write a log */
void log_write(int level, const char* file, const char* function, int line, const char* fmt, ...) 
	__attribute__((format (printf, 5, 6)));

/* helper macros for write a log, do not use it directly */
#define __LOG__(level,fmt,arg...) do{\
        log_write(level,__FILE__,__FUNCTION__,__LINE__,fmt, ##arg);\
}while(0)

#ifndef LOG_LEVEL
        #define LOG_LEVEL 6
#endif
#if LOG_LEVEL >= 0
#        define LOG_FATAL(fmt,arg...) __LOG__(FATAL,fmt,##arg)
#else
#        define LOG_FATAL(...)
#endif

#if LOG_LEVEL >= 1
#        define LOG_ERROR(fmt,arg...) __LOG__(ERROR,fmt,##arg)
#else
#        define LOG_ERROR(...)
#endif

#if LOG_LEVEL >= 2
#        define LOG_WARNING(fmt,arg...) __LOG__(WARNING,fmt,##arg)
#else
#        define LOG_WARNING(...)
#endif

#if LOG_LEVEL >= 3
#        define LOG_NOTICE(fmt,arg...) __LOG__(NOTICE,fmt,##arg)
#else
#        define LOG_NOTICE(...)
#endif

#if LOG_LEVEL >= 4
#        define LOG_INFO(fmt,arg...) __LOG__(INFO,fmt,##arg)
#else
#        define LOG_INFO(...)
#endif

#if LOG_LEVEL >= 5
#        define LOG_TRACE(fmt,arg...) __LOG__(TRACE,fmt,##arg)
#else
#        define LOG_TRACE(...)
#endif

#if LOG_LEVEL >= 6
#        define LOG_DEBUG(fmt,arg...) __LOG__(DEBUG,fmt,##arg)
#else
#        define LOG_DEBUG(...)
#endif
#endif
