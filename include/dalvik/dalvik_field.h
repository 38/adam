#ifndef __DALVIK_FIELD_H__
#define __DALVIK_FIELD_H__
#include <log.h>
#include <stringpool.h>
#include <sexp.h>
#include <dalvik/dalvik_attrs.h>
#include <dalvik/dalvik_type.h>

typedef struct {
    /* DO NOT ADD ANY DEFINITION HERE */
    const char*    name;            /* THIS MUST BE THE FIRST MEMBER OF THIS STRUCT */
    const char*    path;            /* class path */
    const char*    file;            /* the source file name */
    dalvik_type_t* type;            /* type of this field */
    int            attrs;           /* attributes of the field */
    sexpression_t* defualt_value;   /* default value */
    int            offset;          /* the offest of the field */
} dalvik_field_t;

dalvik_field_t* dalvik_field_from_sexp(sexpression_t* sexp, const char* class_path, const char* file_name);

void dalvik_field_free(dalvik_field_t* mem);

#endif
