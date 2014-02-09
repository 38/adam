#ifndef __DEBUG_H__
/* 
 * this file contains some debug code.
 */
#include <log.h>
#ifdef PRINT_FREE
/* print a log for memory free */
#   define free(mem) do{ LOG_TRACE("free memory@0x%x", mem); free(mem);}while(0)
#endif
#endif
