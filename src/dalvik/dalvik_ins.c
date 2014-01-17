#include <dalvik/dalvik_ins.h>
#include <dalvik/sexp.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

dalvik_instruction_t* dalvik_instruction_pool = NULL;

size_t _dalvik_instruction_pool_capacity = DALVIK_POOL_INIT_SIZE;

size_t _dalvik_instruction_pool_size = 0;

int _dalvik_instruction_pool_resize()
{
    dalvik_instruction_t* old_pool;
    assert(dalvik_instruction_pool != NULL);
    
    old_pool = dalvik_instruction_pool;
    
    dalvik_instruction_pool = realloc(old_pool, _dalvik_instruction_pool_capacity * 2);

    if(NULL == dalvik_instruction_pool) 
    {
        dalvik_instruction_pool = old_pool;
        return -1;
    }
    _dalvik_instruction_pool_capacity *= 2;
    return 0;
}

int dalvik_instruction_init( void )
{
    _dalvik_instruction_pool_size = 0;
    if(NULL == dalvik_instruction_pool) 
        dalvik_instruction_pool = (dalvik_instruction_t*)malloc(sizeof(dalvik_instruction_t) * _dalvik_instruction_pool_capacity);
    if(NULL == dalvik_instruction_pool)
        return -1;
    return 0;
}

int dalvik_instruction_finalize( void )
{
    if(dalvik_instruction_pool != NULL) free(dalvik_instruction_pool);
    return 0;
}

dalvik_instruction_t* dalvik_instruction_new( void )
{
    if(_dalvik_instruction_pool_size >= _dalvik_instruction_pool_capacity)
    {
        if(_dalvik_instruction_pool_resize() < 0) return NULL;
    }
    return dalvik_instruction_pool + (_dalvik_instruction_pool_size ++);
}
static inline void _dalvik_instruction_operand_setup(dalvik_operand_t* operand, uint8_t flags, uint64_t payload)
{
    operand->header.flags = flags;
    operand->payload.uint64 = payload;
}
#define __DI_CONSTRUCTOR(kw) static inline int _dalvik_instruction_##kw(sexpression_t* next, dalvik_instruction_t* buf)
#define __DI_SETUP_OPERAND(id, flag, value) do{_dalvik_instruction_operand_setup(buf->operands + (id), (flag), (uint64_t)(value));}while(0)
#define __DI_REGNUM(buf) (atoi((buf)+1))
#define __DI_INSNUM(buf) (atoi(buf))
#define __DI_INSNUMLL(buf) (atoll(buf))
__DI_CONSTRUCTOR(NOP)
{
    buf->opcode = DVM_NOP;
    buf->num_oprands = 0;
    buf->flags = 0;
    return 0;
}
__DI_CONSTRUCTOR(MOVE)
{
    const char *sour, *dest;
    buf->opcode = DVM_MOVE;
    buf->num_oprands = 2;
    const char* curlit;
    int rc;
    rc = sexp_match(next, "(L?A", &curlit, &next);
    if(rc == 0) return -1;
    else if(curlit == SEXPR_KW_FROM16 || curlit == SEXPR_KW_16)  /* move/from16 or move/16 */
    {
        rc = sexp_match(next, "(L?L?", &dest, &sour);
        if(rc == 0) return -1;
        __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
        __DI_SETUP_OPERAND(1, 0, __DI_REGNUM(sour));
    }
    else if(curlit == SEXPR_KW_WIDE)  /* move-wide */
    {
        next = sexp_strip(next, SEXPR_KW_FROM16, SEXPR_KW_16, NULL); /* strip 'from16' and '16', because we don't distinguish the range of register */
        if(sexp_match(next, "(L?L?", &dest, &sour)) 
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(sour));
        }
        else return -1; 
    }
    else if(curlit == SEXPR_KW_OBJECT)  /*move-object*/
    {
        next = sexp_strip(next, SEXPR_KW_FROM16, SEXPR_KW_16, NULL);  /* for the same reason, strip it frist */
        if(sexp_match(next, "(L?L?", &dest, &sour)) 
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(sour));
        }
    }
    else if(curlit == SEXPR_KW_RESULT)  /* move-result */
    {
        if(sexp_match(next, "(L=L?", SEXPR_KW_WIDE, &dest))  /* move-result/wide */
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE | DVM_OPERAND_FLAG_RESULT, 0);
        }
        else if(sexp_match(next, "(L=L?", SEXPR_KW_OBJECT, &dest)) /* move-result/object */
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT) | DVM_OPERAND_FLAG_RESULT, 0);
        }
        else if(sexp_match(next, "(L?", &dest))  /* move-result */
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_RESULT, 0);
        }
        else return -1;
    }
    else if(curlit == SEXPR_KW_EXCEPTION)  /* move-exception */
    {
        if(sexp_match(next, "(L?", &dest))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_EXCEPTION, 0);
        }
        else return -1;
    }
    else
    {
        if(sexp_match(next, "(L?", &sour))
        {
            dest = curlit;
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, 0, __DI_REGNUM(sour));
        }
    }
    return 0;
}
__DI_CONSTRUCTOR(RETURN)
{
    buf->opcode = DVM_RETURN;
    buf->num_oprands = 1;
    const char* curlit, *dest;
    int rc;
    rc = sexp_match(next, "(L?A", &curlit, &next);
    if(curlit == SEXPR_KW_VOID) /* return-void */
    {
        __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_VOID), 0);
    }
    else if(curlit == SEXPR_KW_WIDE) /* return wide */
    {
        if(sexp_match(next, "(L?", &dest))
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
        else return -1;
    }
    else if(curlit == SEXPR_KW_OBJECT) /* return-object */
    {
        if(sexp_match(next, "(L?", &dest))
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
        else return -1;
    }
    else  /* return */
    {
        __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(curlit));
    }
    return 0;
}
__DI_CONSTRUCTOR(CONST)
{
    buf->opcode = DVM_CONST;
    buf->num_oprands = 1;
    const char* curlit, *dest, *sour;
    int rc;
    rc = sexp_match(next, "(L?A", &curlit, &next);
    next = sexp_strip(next, SEXPR_KW_4, SEXPR_KW_16, NULL);     /* We don't care the size of instance number */
    if(curlit == SEXPR_KW_HIGH16) /* const/high16 */
    {
        if(sexp_match(next, "(L?L?", &dest, sour))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST, __DI_REGNUM(sour) << 16);
        }
        else return -1;
    }
    else if(curlit == SEXPR_KW_WIDE) /* const-wide */
    {
        next = sexp_strip(next, SEXPR_KW_16, SEXPR_KW_32, NULL);
        if(sexp_match(next, "(L=L?L?", SEXPR_KW_HIGH16, &dest, &sour))  /* const-wide/high16 */
        {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE | DVM_OPERAND_FLAG_CONST, ((uint64_t)__DI_INSNUM(sour)) << 48);
        }
        else
        {
            /* const-wide */
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST | DVM_OPERAND_FLAG_WIDE, __DI_INSNUMLL(sour));
        }
    }
    else if(curlit == SEXPR_KW_STRING) /* const-string */
    {
       next = sexp_strip(next, SEXPR_KW_JUMBO, NULL);   /* Jumbo is useless for us */
       if(sexp_match(next, "(L?S?", &dest, &sour))
       {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_STRING), __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_STRING) | DVM_OPERAND_FLAG_CONST, sour);
       }
       else return -1;
    }
    else if(curlit == SEXPR_KW_CLASS)  /* const-class */
    {
        if(sexp_match(next, "(L?A", &dest, &next))
        {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_STRING) | DVM_OPERAND_FLAG_CONST, sexp_get_object_path(next));
        }
        else return -1;
    }
    else /* const or const/4 or const/16 */
    {
        if(sexp_match(next, "(L?L?", &dest, &sour))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST, __DI_INSNUM(sour));
        }
        else return -1;
    }
    return 0;
}
#undef __DI_CONSTRUCTOR
int dalvik_instruction_from_sexp(sexpression_t* sexp, dalvik_instruction_t* buf, int line, const char* file)
{
    if(sexp == SEXP_NIL) return -1;
    if(NULL == buf) return -1;
    const char* firstword;
    sexpression_t* next;
    int rc;
    rc = sexp_match(sexp, "(L?A", &firstword, &next);
#define __DI_BEGIN if(0 == rc){return -1;}
#define __DI_CASE(kw) else if(firstword == SEXPR_KW_##kw){ rc = _dalvik_instruction_##kw(next, buf); }
#define __DI_END else rc = -1;
    __DI_BEGIN
        __DI_CASE(NOP)
        __DI_CASE(MOVE)
        __DI_CASE(RETURN)
        __DI_CASE(CONST)   /* TODO: test */
    __DI_END
#undef __DI_END
#undef __DI_CASE
#undef __DI_BEGIN
    return rc;
}
