#ifndef __DALVIK_METHOD_H__
#define __DALVIK_METHOD_H__
#include <stdint.h>

#include <log.h>
#include <sexp.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_type.h>
#include <dalvik/dalvik_attrs.h>

typedef struct {
    const char*          path;     /* Method Path */
    const char*          file;     /* File Name  */
    uint32_t             flags;    /* Flags */
    dalvik_type_t*       return_type;
    uint32_t             num_args;
    dalvik_instruction_t* entry;
    dalvik_type_t*       args_type[0];
    /* DO NOT ADD DEFINITION AFTER args_type */
} dalvik_method_t;

dalvik_method_t* dalvik_method_from_sexp(sexpression_t* sexp, const char* class_path ,const char* file);

void dalvik_method_free(dalvik_method_t* method);

#endif
