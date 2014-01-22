#ifndef __LABEL_H__
#define __LABEL_H__
#include <constants.h>
#include <stdint.h>
#include <dalvik/dalvik_instruction.h>
/* previous definitions */
struct _dalvik_instruction_t;
typedef struct _dalvik_instruction_t dalvik_instruction_t;
/* this is the jump table for the labels */
extern dalvik_instruction_t* dalvik_label_jump_table[DAVLIK_LABEL_POOL_SIZE];

void dalvik_label_init(void);   /* initialize the label pool */
void dalvik_label_free(void);   

/* this function is used look for the label table,
 * converting a label to a label id.
 * If the label is not existed, the function will create a 
 * new label entity and return it's value
 * label: the name of label we want to look for
 * return value: label id. If the value is less than 0,
 *               indicates that an error happend
 */
int dalvik_label_get_label_id(const char* label);

#endif /* __LABEL_H__ */
