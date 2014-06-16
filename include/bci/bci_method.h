#ifndef __BCI_METHOD_H__
#define __BCI_METHOD_H__

#include <constants.h>
#include <const_assertion.h>

#include <log.h>

/**
 * @brief the buildin method definition
 **/
typedef struct {
	uint32_t nparam;   /* how many parameters */
	int (*invoke)(void* env, void* result);   /* invoke this method */
} bci_method_t;

#endif
