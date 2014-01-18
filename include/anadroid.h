#ifndef __ANDADROID_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <log.h>
#include <stringpool.h>
#include <sexp.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_label.h>

#ifndef STRING_POOL_SIZE
#   define STRING_POOL_SIZE 100003
#endif

/* following two function is used for system inintialization and
 * finalization 
 */
void anadroid_init(void);
void anadroid_finalize(void);
#endif
