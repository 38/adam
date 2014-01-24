#ifndef __DALVIK_EXCEPTION_H__
#define __DALVIK_EXCEPTION_H__ 

#include <sexp.h>

typedef struct {
    const char* exception;      /* Exception this handler catches, if it's NULL, that means the handler catches all exceptions */
    int         handler_label;  /* The label of the handler */
} dalvik_exception_handler_t;

dalvik_exception_handler_t* dalvik_exception_handler_from_sexp(sexpression_t* sexp);

typedef struct _dalvik_exception_handler_set_t{
    dalvik_excepttion_handler_t*      handler;
    dalvik_exception_handler_set_t*  next;
} dalvik_exception_handler_set_t;

void dalvik_exception_init();
void dalvik_exception_finalize();

//TODO: how to construct a set
#endif
