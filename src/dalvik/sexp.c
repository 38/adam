#include <dalvik/sexp.h>
#include <stringpool.h>
#include <stdarg.h>

const char* sexpr_keywords[SEXPR_MAX_NUM_KEYWORDS];

const char* _sexpr_keywords_defs[SEXPR_MAX_NUM_KEYWORDS] = {
    "momve",
    "return",
    "const",
    "monitor",
    "check",
    "instance",
    "array",
    "new",
    "filled",
    "16",
    "from16",
    "wide",
    "object",
    "result",
    "exception",
    "void",
    "4",
    "high16",
    "32",
    "class",
    "jumbo",
    "enter",
    "exit",
    "cast",
    "of",
    "length",
    "range",
    "throw",
    "goto",
    "packed",
    "switched",
    "sparse",
    "cmpl",
    "cmpg",
    "cmp",
    "float",
    "double",
    "long",
    "if",
    "eq",
    "ne",
    "le",
    "ge",
    "gt",
    "lt",
    "eqz",
    "nez",
    "lez",
    "gez",
    "gtz",
    "ltz",
    "boolean",
    "byte",
    "char",
    "short",
    "aget",
    "aput",
    "sget",
    "sput",
    "iget",
    "iput",
    "invoke",
    "virtual",
    "super",
    "direct",
    "static",
    "interface",
    "int",
    "to",
    "neg",
    "not",
    "add",
    "sub",
    "mul",
    "div",
    "rem",
    "and",
    "or",
    "xor",
    "shl",
    "shr",
    "ushl",
    "ushr",
    "2addr",
    "lit8",
    "lit16",
    NULL
}; 

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
    return (sexpression_t*) malloc(sizeof(sexpression_t));
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
int sexp_init(void)
{
    int i;
    for(i = 0; _sexpr_keywords_defs[i]; i ++)
        sexpr_keywords[i] = stringpool_query(_sexpr_keywords_defs[i]);
    return 0;
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
/* parse the list from str  .... ), the first '(' is already eatten */
static inline const char* _sexp_parse_list(const char* str, sexpression_t** buf)
{
    /* strip the leading white spaces */
    _sexp_parse_ws(&str);
    /* If it's an empty list, then return NIL */
    if(*str == ')') 
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
    sexpression_t* this; 
    _sexp_parse_ws(&str);
    if(*str == '(') return _sexp_parse_list(str + 1, buf);
    if(*str == '"') return _sexp_parse_string(str + 1, buf);
    return _sexpr_parse_literal(str, buf);
}
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...)
{
   va_list va;
   int ret = 1;
   if(NULL == pattern) return 0;
   va_start(va,pattern);
   if(pattern[0] != '(')
   {
       /* TODO: verify it */
   }
   va_end(va);
   return ret;
}
