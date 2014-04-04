#ifndef __DALVIK_EXCEPTION_H__
#define __DALVIK_EXCEPTION_H__ 
/** @file dalvik_exception.h
 *  @brief Parse and store exception handler
 */
#include <sexp.h>
/** @brief exception handler */
typedef struct {
	const char* exception;      /*!<Exception this handler catches, if it's NULL, that means the handler catches all exceptions */
	int         handler_label;  /*!<The label of the handler */
} dalvik_exception_handler_t;

/**@brief a set of exception handler */
typedef struct _dalvik_exception_handler_set_t{
	dalvik_exception_handler_t*      handler;   /*!<this handler*/
	struct _dalvik_exception_handler_set_t*  next; /*!<the linked list pointer*/
} dalvik_exception_handler_set_t;

/** @brief Parse a expection handler from a S-Expression, from & to are the range this handler applys 
 *  @param sexp the s-expression
 *  @param from,to the valid range of this handler
 *  @return the handler object
 */
dalvik_exception_handler_t* dalvik_exception_handler_from_sexp(const sexpression_t* sexp, int* from, int *to);
/** @brief Create a new exception handler set 
 *  @param count number of handler set
 *  @param set list of handler
 *  @return handler set
 */
dalvik_exception_handler_set_t* dalvik_exception_new_handler_set(size_t count, dalvik_exception_handler_t** set);


/* The memory for exception handler is managed by dalvik_exception.c,
 * So there's no interface for free
 */
/** brief initialization */
int dalvik_exception_init();
/** brief finalization */
void dalvik_exception_finalize();

#endif
