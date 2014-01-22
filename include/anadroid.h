#ifndef __ANDADROID_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <constants.h>
#include <log.h>
#include <stringpool.h>
#include <sexp.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_label.h>
#include <dalvik/dalvik_type.h>
#include <dalvik/dalvik_memberdict.h>
#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>


/* following two function is used for system inintialization and
 * finalization 
 */
void anadroid_init(void);
void anadroid_finalize(void);
#endif
