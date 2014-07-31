/**
 * @file cesk.h
 * @brief top level header file of CESK module
 **/
#ifndef __CESK_H__
#define __CESK_H__

#include <log.h>
#include <stringpool.h>

#include <cesk/cesk_object.h>
#include <cesk/cesk_value.h>
#include <cesk/cesk_store.h>
#include <cesk/cesk_set.h>
#include <cesk/cesk_frame.h>
#include <cesk/cesk_diff.h>
#include <cesk/cesk_arithmetic.h>
#include <cesk/cesk_alloctab.h>
#include <cesk/cesk_reloc.h>
#include <cesk/cesk_block.h>
#include <cesk/cesk_method.h>
#include <cesk/cesk_static.h>

/** @brief initialize 
 *  @return < 0 for error
 */
int cesk_init(void);
/** @brief finalize 
 *  @return nothing
 */
void cesk_finalize(void);
#endif
