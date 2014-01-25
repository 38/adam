#ifndef __DEBUG_H__
#include <log.h>
#ifdef PRINT_FREE
#   define free(mem) do{ LOG_TRACE("free memory@0x%x", mem); free(mem);}while(0)
#endif
#endif
