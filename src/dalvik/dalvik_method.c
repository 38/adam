#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_tokens.h>
static inline int _dalvik_method_get_attrs(sexpression_t* sexp)
{
    int flags = 0;
    if(!sexp_match(sexp, "(L=A", DALVIK_TOKEN_ATTRS, &sexp)) return -1;
    for(; SEXP_NIL != sexp;)
    {
        const char* this_attr;
        if(!sexp_match(sexp, "(L?A", &this_attr, &sexp)) return -1;
        if(DALVIK_TOKEN_ABSTRACT == this_attr)
            flags |= DALVIK_METHOD_FLAG_ABSTARCT;
        else if(DALVIK_TOKEN_ANNOTATION == this_attr)
            flags |= DALVIK_METHOD_FLAG_ANNOTATION;
        else if(DALVIK_TOKEN_FINAL == this_attr)
            flags |= DALVIK_METHOD_FLAG_FINAL;
        else if(DALVIK_TOKEN_PRIVATE == this_attr)
            flags |= DALVIK_METHOD_FLAG_PRIVATE;
        else if(DALVIK_TOKEN_PROTECTED == this_attr)
            flags |= DALVIK_METHOD_FLAG_PROCTED;
        else if(DALVIK_TOKEN_PUBLIC == this_attr)
            flags |= DALVIK_METHOD_FLAG_PUBLIC;
        else if(DALVIK_TOKEN_SYNCHRNIZED == this_attr)
            flags |= DALVIK_METHOD_FLAG_SYNCRONIZED;
        else if(DALVIK_TOKEN_STATIC == this_attr)
            flags |= DALVIK_METHOD_FLAG_STATIC;
        else if(DALVIK_TOKEN_TRANSIENT == this_attr)
            flags |= DALVIK_METHOD_FLAG_TRASIENT;
        else 
        {
            LOG_WARNING("unknown method attribute %s", this_attr);
        }
    }
    return flags;
}
dalvik_method_t* dalvik_method_from_sexp(sexpression_t* sexp, const char* class_path,const char* file)
{
    if(SEXP_NIL == sexp) return NULL;
    
    if(NULL == class_path) class_path = "(undefined)";
    if(NULL == file) file = "(undefined)";

    const char* name;
    sexpression_t *attrs, *arglist, *ret, *body;
    /* matches (method (attribute-list) method-name (arg-list) return-type body) */
    if(!sexp_match(sexp, "(L=C?L?C?_?A", DALVIK_TOKEN_METHOD, &attrs, &name, &arglist, &ret, &body))
        return NULL;

    /* get attributes */
    int attrnum;
    if((attrnum = _dalvik_method_get_attrs(attrs)) < 0)
        return NULL;

    /* get number of arguments */
    int num_args;
    num_args = sexp_length(arglist);

    //TODO: process other parts of a method
}

void dalvik_method_free(dalvik_method_t* method)
{

}
