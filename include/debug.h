/** @file debug.h
 *  @brief Functions, macros , variable that only used for debugging 
 */
#ifndef __DEBUG_H__
#include <log.h>
#ifdef PRINT_FREE
/** @brief print a log for memory free */
#   define free(mem) do{ LOG_TRACE("free memory@0x%x", mem); free(mem);}while(0)
#endif
#endif
