#ifndef __DALVIK_BLOCK_H__
#define __DALVIK_BLOCK_H__

#include <constants.h>

#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_memberdict.h>
#include <dalvik/dalvik_field.h>

#define DALVIK_BLOCK_INDEX_INVALID 0xfffffffful
typedef struct _dalvik_block_t dalvik_block_t;
/* because all branches are based on value comparasion to 
 * determine wether or not the control flow will goes into
 * the branch */
typedef struct {
    dalvik_block_t*     block;   /* the code block of this branch */
    dalvik_operand_t*   left;    /* the left operand */
    dalvik_operand_t*   right;   /* the right operand */

    /* flags */
    uint8_t             flags[0];
    uint8_t             eq:1;    /* enter this branch if left == right */
    uint8_t             lt:1;    /* enter this branch if left < right */
    uint8_t             gt:1;    /* enter this branch if left > right */
} dalvik_block_branch_t;
struct _dalvik_block_t{ 
    uint32_t              index;    /* the index of the block with the method */
    dalvik_instruction_t* first;    /* the first instruction of this block */
    dalvik_instruction_t*   last;   /* the last instruction of this block */
    size_t  fanout;                 /* how many possible executing path after this block is done */
    dalvik_block_branch_t* branch[0]; /* all possible executing path */
};

/* initialize and finalize block cache (function path -> block graph) */
void dalvik_block_init();
void dalvik_block_finalize();

/* construct a block graph from a function */
dalvik_block_t* dalvik_block_from_method(const char* classpath, const char* methodname); 

#endif
