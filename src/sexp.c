#include <stdarg.h>

#include <sexp.h>
#include <stringpool.h>


static inline sexpression_t* _sexp_alloc(int type)
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
    sexpression_t* ret = (sexpression_t*) malloc(sizeof(sexpression_t));
    if(NULL != ret) ret->type = type;
    return ret;
}
void sexp_free(sexpression_t* buf)
{
    sexp_cons_t* cons_data;
    if(SEXP_NIL == buf)  return; /* A empty S-Expression */
    switch(buf->type)
    {
        case SEXP_TYPE_CONS:     /* A Cons S-Expression, free the memory recursively */
            cons_data = (sexp_cons_t*) buf->data;
            sexp_free(cons_data->first);
            sexp_free(cons_data->second);
        default:
            free(buf);
    }
}
/* strip the white space, return value if the function eated a space */
static inline int _sexp_parse_ws(const char** p) 
{
    int ret = 0;
    for(; **p == ' ' || 
          **p == '\t' ||
          **p == '\r' ||
          **p == '\n'||
          **p == '/' ||
          **p == '-' ||
          **p == ',';
          (*p) ++) ret = 1;
    return ret;
}
void _sexp_parse_comment(const char** str)
{
    if(**str == ';')
    {
        (*str) ++;
        while(**str && **str != '\n')
            (*str) ++;
        _sexp_parse_ws(str);
        _sexp_parse_comment(str);
    }
}
/* parse the list from str  .... ), the first '(' is already eatten */
static inline const char* _sexp_parse_list(const char* str, sexpression_t** buf)
{
    /* strip the leading white spaces */
    _sexp_parse_ws(&str);
    _sexp_parse_comment(&str);
    /* If it's an empty list, then return NIL */
    if(*str == ')' || *str == ']') 
    {
        *buf = SEXP_NIL;
        return str + 1;
    }
    else
    {
        /* The list has at least one element */
        *buf = _sexp_alloc(SEXP_TYPE_CONS);
        sexp_cons_t* data = (sexp_cons_t*)((*buf)->data);
        if(NULL == *buf) goto ERR;
        str = sexp_parse(str, &data->first);
        if(NULL == str) goto ERR;
        str = _sexp_parse_list(str, &data->second);
        if(NULL == str) goto ERR;
        return str;
    }
ERR:
    sexp_free(*buf);
    return NULL;
}
/* parse a string */
static inline const char* _sexp_parse_string(const char* str, sexpression_t** buf)
{
    int escape = 0;
    stringpool_accumulator_t accumulator;
    stringpool_accumulator_init(&accumulator, str); 
    for(;*str; str ++)
    {
        if(escape == 0)
        {
            if(*str == '"') 
            {
                *buf = _sexp_alloc(SEXP_TYPE_STR);
                if(NULL == *buf) return NULL;
                sexp_str_t* data = (sexp_str_t*)((*buf)->data);
                (*data) = stringpool_accumulator_query(&accumulator);
                return str + 1;
            }
            else if(*str != '\\')
                stringpool_accumulator_next(&accumulator,*str);
            else
                escape = 1;
        }
        else
        {
            escape = 0;
            stringpool_accumulator_next(&accumulator, *str);
        }
    }
    return NULL;
}
/* parse a literal */
#define RANGE(l,r,v) (((l) <= (v)) && ((v) <= (r)))
static inline const char* _sexpr_parse_literal(const char* str, sexpression_t** buf)
{
    stringpool_accumulator_t accumulator;
    stringpool_accumulator_init(&accumulator, str);
    for(;RANGE('0','9',*str) || RANGE('a','z',*str) || RANGE('A','Z',*str) ; str++)
        stringpool_accumulator_next(&accumulator, *str);
    *buf = _sexp_alloc(SEXP_TYPE_LIT);
    if(*buf == NULL) return NULL;
    sexp_lit_t* data;
    data = (sexp_lit_t*)((*buf)->data);
    *data = stringpool_accumulator_query(&accumulator);
    return str;
}
const char* sexp_parse(const char* str, sexpression_t** buf)
{
    if(NULL == str) return NULL;
    sexpression_t* this; 
    _sexp_parse_ws(&str);
    _sexp_parse_comment(&str);
    if(*str == 0)
    {
        (*buf) = SEXP_NIL;
        return str;
    }
    else if(*str == '(' || *str == '[') return _sexp_parse_list(str + 1, buf);
    else if(*str == '"') return _sexp_parse_string(str + 1, buf);
    else return _sexpr_parse_literal(str, buf);
}
static inline int _sexp_match_one(const sexpression_t* sexpr, char tc, char sc, const void** this_arg)
{
   int ret = 1;
   int type;
   /* Determine the type expected */
   switch(tc)
   {
       case 'C':
           type = SEXP_TYPE_CONS;
           break;
       case 'L':
           type = SEXP_TYPE_LIT;
           break;
       case 'S':
           type = SEXP_TYPE_STR;
           break;
       default:
           ret = 0;
           goto DONE;
   }
   if(type != sexpr->type)
   {
       /* The type does not match ? */
       ret = 0;
       goto DONE;
   }
   if(sc == 0)
   {
       /* A type name without a suffix, bad pattern */
       ret = 0;
       goto DONE;
   }
   if(SEXP_TYPE_CONS == type && sc == '=')
   {
       /* Cons can not used as input */
       ret = 0;
       goto DONE;
   }
   if(sc == '=')
   {
       /* We are going to verify it*/
       const char* this_chr = (const char*) this_arg;
       if(this_chr == *(const char**)sexpr->data) 
           ret = 1;
       else 
           ret = 0;
       goto DONE;
   }
   else if(sc == '?')
   {
       /* Copy the point to user */
       ret = 1;
       if(type != SEXP_TYPE_CONS)
           (*this_arg) = *(const void**)sexpr->data;
       else
           (*this_arg) = sexpr;
   }
DONE:
   return ret;
}
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...)
{
   int ret = 1;
   va_list va;
   const void** this_arg;
   if(NULL == pattern) return 0;
   va_start(va,pattern);
   if(pattern[0] != '(')
   {
       this_arg = va_arg(va, const void **);
       if(*pattern != 0) 
           ret = _sexp_match_one(sexpr, pattern[0], pattern[1], this_arg);
       else
           ret = 0;
   }
   else
   {
       for(pattern++; *pattern && ret ; pattern += 2)
       {
           this_arg = va_arg(va, const void**);
           if('A' == pattern[0])
           {
               pattern ++;
               (*this_arg) = sexpr;
               sexpr = SEXP_NIL;
               break;
           }
           if(sexpr == SEXP_NIL) break;
           if(sexpr->type != SEXP_TYPE_CONS) 
           {
               ret = 0;
               break;
           }
           sexp_cons_t* cons = (sexp_cons_t*)sexpr->data;
           ret = _sexp_match_one(cons->first, pattern[0], pattern[1], this_arg);
           sexpr = cons->second;
       }
       if(*pattern == 0 && sexpr == SEXP_NIL && ret) ret = 1;
       else ret = 0;
   }
DONE:
   va_end(va);
   return ret;
}
sexpression_t* sexp_strip(sexpression_t* sexpr, ...)
{
    va_list va;
    if(NULL == sexpr) return sexpr;
    if(sexpr->type != SEXP_TYPE_CONS) return sexpr;
    va_start(va, sexpr);
    sexp_cons_t*   cons = (sexp_cons_t*) sexpr->data;
    void* first_addr = *(void**)cons->first->data;
    for(;;)
    {
        const char* this;
        this = va_arg(va, const char*);
        if(this == NULL) break;   /* we are reaching the end of the list */
        if(first_addr == this)
            return cons->second;   /* strip the expected element */
    }
    va_end(va);
    return sexpr;   /* nothing to strip */
}
const char* sexp_get_object_path(sexpression_t* sexpr)
{
    int len = 0;
    static char buf[4096];   /* Issue: thread safe */
    if(sexpr == NULL) return NULL;
    for(; sexpr != SEXP_NIL;)
    {
        /* Because the property of parser, we are excepting a cons here */
        if(sexpr->type != SEXP_TYPE_CONS) 
            return NULL;   
        sexp_cons_t *cons = (sexp_cons_t*)sexpr->data;
         /* first should not be a non-literial */
        if(SEXP_NIL == cons->first || cons->first->type != SEXP_TYPE_LIT) 
            return NULL;   
        const char *p;
        for(p = *(const char**)cons->first->data;
            *p;
             p++) buf[len++] = *p;
        buf[len ++] = '/';
        sexpr = cons->second;
    }
    buf[--len] = 0;
    return stringpool_query(buf);
}
const char* sexp_get_object_path_remaining(sexpression_t* sexpr, sexpression_t** remaining)
{
    int len = 0;
    static char buf[4096];   /* Issue: thread safe */
    if(sexpr == NULL) return NULL;
    for(; sexpr != SEXP_NIL;)
    {
        /* Because the property of parser, we are excepting a cons here */
        if(sexpr->type != SEXP_TYPE_CONS) 
            return NULL;   
        sexp_cons_t *cons = (sexp_cons_t*)sexpr->data;
        /* We are reaching the last element of the S-Expression */
        if(cons->second == SEXP_NIL)
        {
            (*remaining) = cons->first;
            break;
        }
         /* first should not be a non-literial */
        if(SEXP_NIL == cons->first || cons->first->type != SEXP_TYPE_LIT)
            return NULL;
        const char *p;
        for(p = *(const char**)cons->first->data;
            *p;
             p++) buf[len++] = *p;
        buf[len ++] = '/';
        sexpr = cons->second;
    }
    buf[--len] = 0;
    return stringpool_query(buf);
}

