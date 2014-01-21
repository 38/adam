#ifndef __DALVIK_CLASS_H__
#define __DALVIK_CLASS_H__
#include <sexp.h>
#include <dalvik/dalvik_attrs.h>

//TODO:

#define DALVIK_CLASS_ATTRS_INTERFACE 0x10000

typedef struct{
    const char* class_path;
    int         super_class;        /* The class id of super class */
    int         class_attrs;
    /* We do not memorize all member of the class, the class can be retrived 
     * by (ClassId, methodName). The class itslef can be retrived by
     * (ClassId, NULL)
     */
} dalvik_class_t;

dalvik_class_t* dalvik_class_from_sexp(sexpression_t* sexp);

int dalvik_class_free(dalvik_class_t* mem);
#endif
