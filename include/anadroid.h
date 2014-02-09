#ifndef __ANDADROID_H__
/*
 * anadroid.c: 
 *  1. Global Initalization & Finalization 
 *  2. Include all header files
 */
#include <stdint.h>

#include <stringpool.h>
#include <log.h>
#include <vector.h>

#include <dalvik/dalvik.h>
#include <cesk/cesk.h>
/* following two function is used for system inintialization and
 * finalization 
 */
void anadroid_init(void);
void anadroid_finalize(void);
#endif
