/**
 * @file cesk_alloc_param.h
 * @brief data structure for the allocation parameter
 **/
#ifndef __CESK_ALLOC_PARAM_H__
#define __CESK_ALLOC_PARAM_H__
#include <const_assertion.h>
#include <constants.h>
#include <dalvik/dalvik.h>
/** @brief the data structure of store allocation parameter */
typedef struct {
	uint32_t inst;                      /*!< The instruction which makes this allocation */
	uint32_t offset;                    /*!< The field offset */
} cesk_alloc_param_t;
/** 
 * @brief addressing hash code
 * @param param the allocation parameter
 * @return the hashcode
 **/
static inline hashval_t cesk_alloc_param_hash(const cesk_alloc_param_t* param)
{
	return (param->inst * param->inst * MH_MULTIPLY + (param->offset * MH_MULTIPLY * MH_MULTIPLY)); 

}
/**
 * @brief return wether or not two allocation parameter are the same
 * @brief first the first parameter
 * @brief section the second parameter
 **/
static inline int cesk_alloc_param_equal(const cesk_alloc_param_t* first, const cesk_alloc_param_t* second)
{
	return (first->inst == second->inst) && (first->offset == second->offset);
}
/**
 * @brief allocation parameter value
 * @param inst_value the value of instruction index
 * @param offset_value the value of field offset
 **/
#define CESK_ALLOC_PARAM(inst_value, offset_value) {\
	.inst = (inst_value),\
	.offset = (offset_value)\
}
/**
 * @brief field not available
 **/
#define CESK_ALLOC_NA 0
#endif /*__CESK_ALLOC_PARAM_H__*/
