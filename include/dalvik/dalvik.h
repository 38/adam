#ifndef __DALVIK_H__
#define __DALVIK_H__ 
/** @file dalvik.h
 *  @brief dalvik bytecode utils
 */
/* System Headers */
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

/* Global Utils */
#include <constants.h>
#include <log.h>
#include <stringpool.h>
#include <debug.h>
#include <vector.h>
#include <sexp.h>

/* Dalvik Headers */
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_label.h>
#include <dalvik/dalvik_type.h>
#include <dalvik/dalvik_memberdict.h>
#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_exception.h>
#include <dalvik/dalvik_attrs.h>
#include <dalvik/dalvik_loader.h>
#include <dalvik/dalvik_block.h>
/** @brief initialization */
void dalvik_init(void);
/** @brief finalization */
void dalvik_finalize(void);

#endif
