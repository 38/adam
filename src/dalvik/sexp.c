#include <dalvik/sexp.h>
static inline sexpression_t* _sexpr_alloc(int type)
{
    size_t size = sizeof(sexpression_t);
    switch(type)
    {
        case SEXP_TYPE_LIT:
            size += sizeof(sexp_lit_t);
            break;
        case SEXP_TYPE_STR:
            size += sizeof(sexp_str_t);
            break;
        case SEXP_TYPE_CONS:
            size += sizeof(sexp_cons_t);
            break;
        default:
            return NULL;
    }
    return (sexpression_t*) malloc(sizeof(sexpression_t));
}
const char* sexp_free(sexpression_t* buf)
{

}
const char* sexp_parse(const char* str, sexpression_t* buf)
{
}
