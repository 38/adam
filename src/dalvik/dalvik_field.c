#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_field.h>
#include <string.h>
#include <stdlib.h>

dalvik_field_t* dalvik_field_from_sexp(sexpression_t* sexp, const char* class_path, const char* file_name)
{
    dalvik_field_t* ret = NULL;
    if(SEXP_NIL == sexp) goto ERR;
    if(NULL == class_path) class_path = "(undefined)";
    if(NULL== file_name)   file_name = "(undefined)";
    const char* name;
    sexpression_t *attr_list , *type_sexp;
    if(!sexp_match(sexp, "(L=C?L?_?A", DALVIK_TOKEN_FIELD, &attr_list, &name, &type_sexp, &sexp))
        goto ERR;
    
    ret = (dalvik_field_t*)malloc(sizeof(dalvik_field_t));
    
    ret->name = name;
    ret->path = class_path;
    ret->file = file_name;

    if(NULL == (ret->type = dalvik_type_from_sexp(type_sexp))) goto ERR;

    if(-1 == (ret->attrs = dalvik_attrs_from_sexp(attr_list))) goto ERR;

    if(SEXP_NIL != sexp)
    {
        /* it has a defualt value */
        if(!sexp_match(sexp, "(_?", &ret->defualt_value)) goto ERR;
    }

    return ret;
ERR:
    dalvik_field_free(ret);
    return NULL;
}

void dalvik_field_free(dalvik_field_t* mem)
{
    if(NULL != mem) 
    {
        if(NULL != mem->type) dalvik_type_free(mem->type);
        free(mem);
    }
}
