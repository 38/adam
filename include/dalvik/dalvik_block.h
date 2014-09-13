#ifndef __DALVIK_BLOCK_H__
#define __DALVIK_BLOCK_H__
/** @file dalvik_block.h
 *  @brief in this file we define the code block. 
 *         And provie a gourp of functions to build 
 *         code blocks from method.
 *
 *  @details a block blok is a instruction sequence,
 *           which do not contasin branch ,goto and 
 *           invoke. In addition a instruction that 
 *           is not the first instruction of a code 
 *           block can not be the target of any jump.
 *           At the end of the block there might be 
 *           numbers branch instructions, which can 
 *           make the control flow goto different 
 *           blocks. 
 *
 */
#include <constants.h>
#include <const_assertion.h>

#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_memberdict.h>
#include <dalvik/dalvik_field.h>

/** @brief invalid block index */
#define DALVIK_BLOCK_INDEX_INVALID 0xfffffffful
typedef struct _dalvik_block_t dalvik_block_t;
/**
 * @brief unconditional branch type cods
 **/
enum {
	DALVIK_BLOCK_BRANCH_UNCOND_JUMP,     /*!< this is a unconditional jump */
	DALVIK_BLOCK_BRANCH_UNCOND_RETURN,   /*!< this is a return branch, if the branch is this type, the block field of the 
	                                      *   branch will be set to NULL, and the return value is in the left operand,
										  */
	DALVIK_BLOCK_BRANCH_UNCOND_EXCEPTION /*!< this is an exception branch */
};
/** @brief the mask for the unconditional type code */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_MSK 0x6
/** @brief get the type code in a unconditional branch */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_GET(branch) ((((branch).flags[0] & DALVIK_BLOCK_BRANCH_UNCOND_TYPE_MSK) >> 1))
/** @brief check this branch an unconditional jump */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_JUMP(b) (DALVIK_BLOCK_BRANCH_UNCOND_TYPE_GET(b) == DALVIK_BLOCK_BRANCH_UNCOND_JUMP && (b).conditional == 0)
/** @brief check this branch an unconditional return */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(b) (DALVIK_BLOCK_BRANCH_UNCOND_TYPE_GET(b) == DALVIK_BLOCK_BRANCH_UNCOND_RETURN && (b).conditional == 0)
/** @brief check this branch an unconditional exception */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_EXCEPTION(b) (DALVIK_BLOCK_BRANCH_UNCOND_TYPE_GET(b) == DALVIK_BLOCK_BRANCH_UNCOND_EXCEPTION && (b).conditional == 0)
/** @brief set type code of unconditional branch */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET(branch,type) do{\
	if((branch).conditional == 1) \
		LOG_WARNING("try to set unconditional type code in a conditional branch, looks like a mistake");\
	(branch).flags[0] &= ~DALVIK_BLOCK_BRANCH_UNCOND_TYPE_MSK;\
	(branch).flags[0] |= (type<<1);\
}while(0)
/** @brief set type code to unconditional jump */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET_JUMP(b) DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET(b, DALVIK_BLOCK_BRANCH_UNCOND_JUMP)
/** @brief set type code to unconditional return */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET_RETURN(b) DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET(b, DALVIK_BLOCK_BRANCH_UNCOND_RETURN)
/** @brief set type code to unconditional exception */
#define DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET_EXCEPTION(b) DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET(b, DALVIK_BLOCK_BRANCH_UNCOND_EXCEPTION)

/* because all branches are based on value comparasion to 
 * determine wether or not the control flow will goes into
 * the branch */
/** @brief a branch in the end of the block */
typedef struct {
	uintptr_t     block_id[0];   /*!<this is the address of member block, when the block remains unlinked, the pointer is resued as block id */
	/* DO NOT ADD ANYTHING HERE */
	dalvik_block_t*     block;     /*!<the code block of this branch */
	int32_t             ileft[0];  /*!<A instant number as left operand. if left_inst is set, value is stored in ileft[0] */
	int32_t             handler[0]; /*!< the exception handler */
	const dalvik_operand_t*   left;      /*!<the left operand */
	const char*         exception[0];  /*!<A exception class path operand */
	const dalvik_operand_t*   right;     /*!<the right operand */


	/* flags */
	/* because when conditional == 0, the eq lt gt flag is useless, 
	 * so we reuse the bit for other purpose to specify the type of 
	 * a unconditional branch 
	 * use macro DALVIK_BLOCK_BRANCH_UNCOND_TYPE_GET to pull the type code
	 * use macro DALVIK_BLOCK_BRANCH_UNCOND_TYPE_SET to set the type code
	 */
	uint8_t             flags[0];   /*!<the flags array*/
	uint8_t             conditional:1;/*!<if this branch a conditional branch */
	uint8_t             eq:1;       /*!<enter this branch if left == right */
	uint8_t             lt:1;       /*!<enter this branch if left < right */
	uint8_t             gt:1;       /*!<enter this branch if left > right */
	uint8_t             linked:1;   /*!<this bit indicates if the link parse is finished, useless for other function */ 
	uint8_t             disabled:1; /*!<if this bit is set, the branch is disabled */
	uint8_t             left_inst:1; /*!<use ileft field ? */
} dalvik_block_branch_t;
CONST_ASSERTION_FOLLOWS(dalvik_block_branch_t, block_id, block);
CONST_ASSERTION_FOLLOWS(dalvik_block_branch_t, ileft, left);
CONST_ASSERTION_FOLLOWS(dalvik_block_branch_t, handler, left);
CONST_ASSERTION_FOLLOWS(dalvik_block_branch_t, exception, right);
CONST_ASSERTION_SIZE(dalvik_block_branch_t, block_id, 0);
CONST_ASSERTION_SIZE(dalvik_block_branch_t, ileft, 0);
CONST_ASSERTION_SIZE(dalvik_block_branch_t, flags, 0);
CONST_ASSERTION_SIZE(dalvik_block_branch_t, handler, 0);
CONST_ASSERTION_SIZE(dalvik_block_branch_t, exception, 0);

/** @brief the block structure */
struct _dalvik_block_t{ 
	uint32_t   index;    /*!<the index of the block with the method, we do assume that a function is not large*/
	uint32_t   begin;    /*!<the first instruction of this block */
	uint32_t   end;      /*!<the last instruction of this block  + 1. The range of the block is [begin,end) */
	size_t     nbranches; /*!<how many possible executing path after this block is done */
	uint16_t   nregs;     /*!<number of registers the block can use */
	struct {
		const char* method;   /*!<the method name */
		const char* class;    /*!<the class path */
		const dalvik_type_t * const * signature; /*!< the function signature */
		const dalvik_type_t* return_type; /*!< the function return type */
	} *info;                  /*!<info*/
	dalvik_block_branch_t branches[0]; /*!<all possible executing path */
};
CONST_ASSERTION_LAST(dalvik_block_t, branches);
CONST_ASSERTION_SIZE(dalvik_block_t, branches, 0);

/** @brief initialize block cache (function path -> block graph) */
int dalvik_block_init();
/** @brief finalize block cache (function path -> block graph) */
void dalvik_block_finalize();

/** 
 *  @brief construct a block graph from a function 
 *  @param classpath the class path contains the method from which we want to build the code block graph
 *  @param methodname the name of the function
 *  @param args the argument table. This is because of the function can be overloaded, so the only way to distingush a method is use argument type list
 *  @param rtype the return type of this function
 *  @return the entry point of the code block 
 **/
dalvik_block_t* dalvik_block_from_method(
		const char* classpath, 
		const char* methodname, 
		const dalvik_type_t * const * args, 
		const dalvik_type_t* rtype);

#endif
