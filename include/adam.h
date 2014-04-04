#ifndef __ADAM_H__
/** @file  adam.h
 *  @brief Global Intalization and Finalization Functions 
 */
#include <stdint.h>

#include <stringpool.h>
#include <log.h>
#include <vector.h>

#include <dalvik/dalvik.h>
#include <cesk/cesk.h>
/** @brief Intialize libadam
 *  @return nothing
 */
int adam_init(void);
/** @brief Finalize libadam
 *  @return nothing
 */
void adam_finalize(void);
#endif
