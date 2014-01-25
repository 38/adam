#ifndef __DALVIK_EXCEPTION_H__
#define __DALVIK_EXCEPTION_H__ 

#include <sexp.h>

typedef struct {
    const char* exception;      /* Exception this handler catches, if it's NULL, that means the handler catches all exceptions */
    int         handler_label;  /* The label of the handler */
} dalvik_exception_handler_t;

typedef struct _dalvik_exception_handler_set_t{
    dalvik_exception_handler_t*      handler;
    struct _dalvik_exception_handler_set_t*  next;
} dalvik_exception_handler_set_t;

/* Parse a expection handler from a S-Expression, from & to are the range this handler applys */
dalvik_exception_handler_t* dalvik_exception_handler_from_sexp(sexpression_t* sexp, int* from, int *to);
/* Create a new exception handler set */
dalvik_exception_handler_set_t* dalvik_exception_new_handler_set(size_t count, dalvik_exception_handler_t** set);


/* The memory for exception handler is managed by dalvik_exception.c,
 * So there's no interface for free
 */
void dalvik_exception_init();
void dalvik_exception_finalize();

//TODO: how to construct a set
#endif
