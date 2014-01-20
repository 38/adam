#ifndef __DALVIK_METHOD_H__
#define __DALVIK_METHOD_H__
#include <stdint.h>

#include <log.h>
#include <sexp.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_type.h>

#define DALVIK_METHOD_FLAG_ABSTARCT    0x1
#define DALVIK_METHOD_FLAG_ANNOTATION  0x2
#define DALVIK_METHOD_FLAG_FINAL       0x4
#define DALVIK_METHOD_FLAG_PRIVATE     0x8
#define DALVIK_METHOD_FLAG_PROCTED     0x10
#define DALVIK_METHOD_FLAG_PUBLIC      0x20
#define DALVIK_METHOD_FLAG_STATIC      0x40
#define DALVIK_METHOD_FLAG_SYNCRONIZED 0x80
#define DALVIK_METHOD_FLAG_TRASIENT    0x100

typedef struct {
    const char*          path;     /* Method Path */
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
