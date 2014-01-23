#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_label.h>
dalvik_method_t* dalvik_method_from_sexp(sexpression_t* sexp, const char* class_path,const char* file)
{
    dalvik_method_t* method = NULL;

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
    if((attrnum = dalvik_attrs_from_sexp(attrs)) < 0)
        return NULL;

    /* get number of arguments */
    int num_args;
    num_args = sexp_length(arglist);

    /* Now we know the size we have to allocate for this method */
    method = (dalvik_method_t*)malloc(sizeof(dalvik_method_t) + sizeof(dalvik_type_t*) * num_args);
    if(NULL == method) return NULL;

    method->num_args = num_args;
    method->path = class_path;
    method->file = file;
    method->name = name;

    /* Setup the type of argument list */
    int i;
    for(i = 0;arglist != SEXP_NIL; i ++)
    {
        sexpression_t *this_arg;
        if(!sexp_match(arglist, "(_?A", &this_arg, &arglist))
        {
            LOG_ERROR("invalid argument list");
            goto ERR;
        }
        if(NULL == (method->args_type[i] = dalvik_type_from_sexp(this_arg)))
        {
            LOG_ERROR("invalid argument type @ #%d", i);
            goto ERR;
        }

    }

    /* Setup the return type */
    if(NULL == (method->return_type = dalvik_type_from_sexp(ret)))
    {
        LOG_ERROR("invalid return type");
        goto ERR;
    }

    /* Now fetch the body */
    
    //TODO: process other parts of a method
    int current_line_number;    /* Current Line Number */
    dalvik_instruction_t *last = NULL;
    int last_label = -1;
    for(;body != SEXP_NIL;)
    {
        sexpression_t *this_smt;
        if(!sexp_match(body, "(C?A", &this_smt, &body))
        {
            LOG_ERROR("invalid method body");
            goto ERR;
        }
        /* First check if the statement is a psuedo-instruction */
        const char* arg;
        char buf[40906];
        static int counter = 0;
        LOG_DEBUG("#%d current instruction : %s",(++counter) ,sexp_to_string(this_smt, buf) );
        if(sexp_match(this_smt, "(L=L=L?", DALVIK_TOKEN_LIMIT, DALVIK_TOKEN_REGISTERS, &arg))
        {
            /* (limit registers ...) */
            /* simple ignore */
        }
        else if(sexp_match(this_smt, "(L=L?", DALVIK_TOKEN_LINE, &arg))
        {
            /* (line arg) */
            current_line_number = atoi(arg);
        }
        else if(sexp_match(this_smt, "(L=L?", DALVIK_TOKEN_LABEL, &arg))
        {
            /* (label arg) */
            int lid = dalvik_label_get_label_id(arg);
            if(lid == -1) 
            {
                LOG_ERROR("can not create label for %s", arg);
                goto ERR;
            }
            last_label = lid;
        }
        else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_ANNOTATION, &arg))
            /* Simplely ignore */;
        else if(sexp_match(this_smt, "(L=L=A", DALVIK_TOKEN_DATA, DALVIK_TOKEN_ARRAY, &arg))
            /* TODO: what is (data-array ....)statement currently ignored */;
        else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_CATCH, &arg))
        {
            //TODO: implmenentation of catch
        }
        else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_CATCHALL, &arg))
        {
            //TODO: implmenentation of catch
        }
        else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_FILL, &arg))
        {
            //TODO: fill-array-data psuedo-instruction
        }
        else
        {
            dalvik_instruction_t* inst = dalvik_instruction_new();
            if(NULL == inst) goto ERR;
            if(dalvik_instruction_from_sexp(this_smt, inst, current_line_number, file) < 0)
            {
                LOG_ERROR("can not recognize instuction %s", sexp_to_string(this_smt, NULL));
                goto ERR;
            }
            if(NULL == last) method->entry = inst;
            else
            {
                last->next = inst;
                last = inst;
            }
            if(last_label >= 0)
            {
                dalvik_label_jump_table[last_label] = inst;
                last_label = -1;
            }
        }
    }
    return method;
ERR:
    dalvik_method_free(method);
    return NULL;
}

void dalvik_method_free(dalvik_method_t* method)
{
    if(NULL == method) return;
    if(NULL != method->return_type) dalvik_type_free(method->return_type);
    int i;
    for(i = 0; i < method->num_args; i ++)
        if(NULL != method->args_type[i])
            dalvik_type_free(method->args_type[i]);
    free(method);
}
