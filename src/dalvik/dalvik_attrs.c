#include <sexp.h>
#include <log.h>
#include <dalvik/dalvik_attrs.h>
#include <dalvik/dalvik_tokens.h>
#include <debug.h>

int dalvik_attrs_from_sexp(const sexpression_t* sexp)
{
    LOG_DEBUG("sexp = %s", sexp_to_string(sexp, NULL));
    int flags = 0;
    /* all attribute section has form (attrs ....) */
    if(!sexp_match(sexp, "(L=A", DALVIK_TOKEN_ATTRS, &sexp)) return -1;
    for(; SEXP_NIL != sexp;)
    {
        const char* this_attr;
        if(!sexp_match(sexp, "(L?A", &this_attr, &sexp)) return -1;
        if(DALVIK_TOKEN_ABSTRACT == this_attr)
            flags |= DALVIK_ATTRS_ABSTARCT;
        else if(DALVIK_TOKEN_ANNOTATION == this_attr)
            flags |= DALVIK_ATTRS_ANNOTATION;
        else if(DALVIK_TOKEN_FINAL == this_attr)
            flags |= DALVIK_ATTRS_FINAL;
        else if(DALVIK_TOKEN_PRIVATE == this_attr)
            flags |= DALVIK_ATTRS_PRIVATE;
        else if(DALVIK_TOKEN_PROTECTED == this_attr)
            flags |= DALVIK_ATTRS_PROCTED;
        else if(DALVIK_TOKEN_PUBLIC == this_attr)
            flags |= DALVIK_ATTRS_PUBLIC;
        else if(DALVIK_TOKEN_SYNCHRNIZED == this_attr)
            flags |= DALVIK_ATTRS_SYNCRONIZED;
        else if(DALVIK_TOKEN_STATIC == this_attr)
            flags |= DALVIK_ATTRS_STATIC;
        else if(DALVIK_TOKEN_TRANSIENT == this_attr)
            flags |= DALVIK_ATTRS_TRASIENT;
        else 
        {
            LOG_WARNING("unknown method attribute %s", this_attr);
        }
    }
    LOG_DEBUG("attribute = 0x%x", flags);
    return flags;
}
