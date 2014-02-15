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
    uintptr_t     block_id[0];   /* this is the address of member block, when the block remains unlinked, the pointer is resued as block id */
    /* DO NOT ADD ANYTHING HERE */
    dalvik_block_t*     block;     /* the code block of this branch */
    int32_t             ileft[0];  /* if left_inst is set, value is stored in ileft[0] */
    dalvik_operand_t*   left;      /* the left operand */
    dalvik_operand_t*   right;     /* the right operand */

    /* flags */
    uint8_t             flags[0];
    uint8_t             conditional;/* if this branch a conditional branch */
    uint8_t             eq:1;       /* enter this branch if left == right */
    uint8_t             lt:1;       /* enter this branch if left < right */
    uint8_t             gt:1;       /* enter this branch if left > right */
    uint8_t             linked:1;   /* this bit indicates if the link parse is finished, useless for other function */ 
    uint8_t             disabled:1; /* if this bit is set, the branch is disabled */
    uint8_t            left_inst:1; /* use ileft field ? */ 
} dalvik_block_branch_t;
struct _dalvik_block_t{ 
    uint32_t   index;    /* the index of the block with the method, we do assume that a function is not large*/
    uint16_t   begin;    /* the first instruction of this block */
    uint16_t   end;      /* the last instruction of this block  + 1. The range of the block is [begin,end) */
    size_t     nbranches;                 /* how many possible executing path after this block is done */
    uint16_t   nregs;     /* number of registers the block can use */
    dalvik_block_branch_t branches[0]; /* all possible executing path */
};

/* initialize and finalize block cache (function path -> block graph) */
void dalvik_block_init();
void dalvik_block_finalize();

/* construct a block graph from a function */
dalvik_block_t* dalvik_block_from_method(const char* classpath, const char* methodname, dalvik_type_t * const * args);

#endif
