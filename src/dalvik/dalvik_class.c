#include <dalvik/dalvik_class.h>
#include <log.h>
#include <sexp.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_memberdict.h>
#include <string.h>


dalvik_class_t* dalvik_class_from_sexp(sexpression_t* sexp)
{
    dalvik_class_t* class = NULL;
    int length;
    int is_interface;
    const char* class_path;
    int attrs;
    int field_count = 0;
    if(SEXP_NIL == sexp) goto ERR;
    const char* firstlit;
    sexpression_t* attr_list;
    if(!sexp_match(sexp, "(L?C?A", &firstlit, &attr_list, &sexp))
        goto ERR;
    if(DALVIK_TOKEN_CLASS == firstlit)
        is_interface = 0;
    else if(DALVIK_TOKEN_INTERFACE == firstlit)
        is_interface = 1;
    else goto ERR;
    
    if((attrs = dalvik_attrs_from_sexp(attr_list)) < 0)
        goto ERR;
    
    if(NULL == (class_path = sexp_get_object_path(sexp, &sexp)))
        goto ERR;
    
    if((length = sexp_length(sexp)) < 0)
        goto ERR;

    /* We allocate length + 1 because we need a NULL at the end of member list */
    class = (dalvik_class_t*)malloc(sizeof(dalvik_class_t) + sizeof(const char*) * (length + 1));  

    if(class == NULL) goto ERR;

    class->path = class_path;
    class->attrs = attrs;
    class->is_interface = is_interface;
    memset(class->members, 0, sizeof(const char*) * length);

    const char* source = "(undefined)";
    /* Ok, let's go */
    for(;sexp != SEXP_NIL;)
    {
        sexpression_t* this_def, *tail;
        if(!sexp_match(sexp, "(C?A", &this_def, &sexp))
            goto ERR;
        if(sexp_match(this_def, "(L=S?", DALVIK_TOKEN_SOURCE, &source))
            /* Do Nothing*/;
        else if(sexp_match(this_def, "(L=A", DALVIK_TOKEN_SUPER, &tail))
        {
            if(NULL == (class->super = sexp_get_object_path(tail,NULL)))
                goto ERR;
        }
        else if(sexp_match(this_def, "(L=A", DALVIK_TOKEN_IMPLEMENTS, &tail))
        {
            if(NULL == (class->implements = sexp_get_object_path(tail, NULL)))
                goto ERR;
        }
        else
        {
            const char* firstlit;
            if(!sexp_match(this_def, "(L?A", &firstlit, &tail))
                goto ERR;
            if(firstlit == DALVIK_TOKEN_METHOD)
            {
                static char buf[10240];
                LOG_DEBUG("we are resuloving method %s", sexp_to_string(this_def, buf));
                dalvik_method_t* method;
                if(NULL == (method = dalvik_method_from_sexp(this_def, class->path, source)))
                    goto ERR;
                /* Register it */
                if(dalvik_memberdict_register_method(class->path, method) < 0)
                    goto ERR;
                class->members[field_count++] = method->name;
            }
            else if(firstlit == DALVIK_TOKEN_FIELD)
            {
                dalvik_field_t* field;
                if(NULL == (field = dalvik_field_from_sexp(this_def, class->path, source)))
                    goto ERR;
                if(dalvik_memberdict_register_field(class->path, field) < 0)
                    goto ERR;
                class->members[field_count++] = field->name;
            }
            else 
            {
                LOG_WARNING("we are ingoring a field");
            }
        }
    }
    if(dalvik_memberdict_register_class(class->path, class) < 0) goto ERR;
    return class;
ERR:
    if(NULL != class) free(class);
    return NULL;
}
