#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dalvik/dalvik_ins.h>
#include <sexp.h>
#include <dalvik/dalvik_tokens.h>

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
    else if(curlit == DALVIK_TOKEN_FROM16 || curlit == DALVIK_TOKEN_16)  /* move/from16 or move/16 */
    {
        rc = sexp_match(next, "(L?L?", &dest, &sour);
        if(rc == 0) return -1;
        __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
        __DI_SETUP_OPERAND(1, 0, __DI_REGNUM(sour));
    }
    else if(curlit == DALVIK_TOKEN_WIDE)  /* move-wide */
    {
        next = sexp_strip(next, DALVIK_TOKEN_FROM16, DALVIK_TOKEN_16, NULL); /* strip 'from16' and '16', because we don't distinguish the range of register */
        if(sexp_match(next, "(L?L?", &dest, &sour)) 
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(sour));
        }
        else return -1; 
    }
    else if(curlit == DALVIK_TOKEN_OBJECT)  /*move-object*/
    {
        next = sexp_strip(next, DALVIK_TOKEN_FROM16, DALVIK_TOKEN_16, NULL);  /* for the same reason, strip it frist */
        if(sexp_match(next, "(L?L?", &dest, &sour)) 
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(sour));
        }
    }
    else if(curlit == DALVIK_TOKEN_RESULT)  /* move-result */
    {
        if(sexp_match(next, "(L=L?", DALVIK_TOKEN_WIDE, &dest))  /* move-result/wide */
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE | DVM_OPERAND_FLAG_RESULT, 0);
        }
        else if(sexp_match(next, "(L=L?", DALVIK_TOKEN_OBJECT, &dest)) /* move-result/object */
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
    else if(curlit == DALVIK_TOKEN_EXCEPTION)  /* move-exception */
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
    if(curlit == DALVIK_TOKEN_VOID) /* return-void */
    {
        __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_VOID), 0);
    }
    else if(curlit == DALVIK_TOKEN_WIDE) /* return wide */
    {
        if(sexp_match(next, "(L?", &dest))
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
        else return -1;
    }
    else if(curlit == DALVIK_TOKEN_OBJECT) /* return-object */
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
    buf->num_oprands = 2;
    const char* curlit, *dest, *sour;
    int rc;
    next = sexp_strip(next, DALVIK_TOKEN_4, DALVIK_TOKEN_16, NULL);     /* We don't care the size of instance number */
    rc = sexp_match(next, "(L?A", &curlit, &next);
    if(curlit == DALVIK_TOKEN_HIGH16) /* const/high16 */
    {
        if(sexp_match(next, "(L?L?", &dest, &sour))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST, __DI_INSNUM(sour) << 16);
        }
        else return -1;
    }
    else if(curlit == DALVIK_TOKEN_WIDE) /* const-wide */
    {
        next = sexp_strip(next, DALVIK_TOKEN_16, DALVIK_TOKEN_32, NULL);
        if(sexp_match(next, "(L=L?L?", DALVIK_TOKEN_HIGH16, &dest, &sour))  /* const-wide/high16 */
        {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_WIDE | DVM_OPERAND_FLAG_CONST, ((uint64_t)__DI_INSNUM(sour)) << 48);
        }
        else if(sexp_match(next, "(L?L?", &dest, &sour))
        {
            /* const-wide */
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_WIDE, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST | DVM_OPERAND_FLAG_WIDE, __DI_INSNUMLL(sour));
        }
        else return -1;
    }
    else if(curlit == DALVIK_TOKEN_STRING) /* const-string */
    {
       next = sexp_strip(next, DALVIK_TOKEN_JUMBO, NULL);   /* Jumbo is useless for us */
       if(sexp_match(next, "(L?S?", &dest, &sour))
       {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_STRING), __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_STRING) | DVM_OPERAND_FLAG_CONST, sour);
       }
       else return -1;
    }
    else if(curlit == DALVIK_TOKEN_CLASS)  /* const-class */
    {
        if(sexp_match(next, "(L?A", &dest, &next))
        {
           __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(dest));
           __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_CLASS) | DVM_OPERAND_FLAG_CONST, sexp_get_object_path(next));
        }
        else return -1;
    }
    else /* const or const/4 or const/16 */
    {
        dest = curlit;
        if(sexp_match(next, "(L?", &sour))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(dest));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_CONST, __DI_INSNUM(sour));
        }
        else return -1;
    }
    return 0;
}
__DI_CONSTRUCTOR(MONITOR)
{
    buf->opcode = DVM_MONITOR;
    buf->num_oprands = 1;
    const char* curlit, *arg;
    int rc;
    rc = sexp_match(next, "(L?A", &curlit, &next);
    if(curlit == DALVIK_TOKEN_ENTER)  /* monitor-enter */
    {
        buf->flags = DVM_FLAG_MONITOR_ENT;
        if(sexp_match(next, "(L?", &arg))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(arg));
        }
        else return -1;
    }
    else if(curlit == DALVIK_TOKEN_EXIT) /* monitor-exit */
    {
        buf->flags = DVM_FLAG_MONITOR_EXT;
        if(sexp_match(next, "(L?", &arg))
        {
            __DI_SETUP_OPERAND(0, 0, __DI_REGNUM(arg));
        }
        else return -1;
    }
    return 0;
}
__DI_CONSTRUCTOR(CHECK)
{
    buf->opcode = DVM_MONITOR;
    buf->num_oprands = 3;
    const char* curlit, *sour;
    int rc;
    rc = sexp_match(next, "(L?A", &curlit, &next);
    if(curlit == DALVIK_TOKEN_CAST)  /* check-cast */
    {
        if(sexp_match(next, "(L?A", &sour, &next))
        {
            __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(sour));
            __DI_SETUP_OPERAND(1, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_CLASS), sexp_get_object_path(next));
        }
        else return -1;
    }
    else return -1;
    return 0;
}
__DI_CONSTRUCTOR(THROW)
{
    buf->opcode = DVM_THROW;
    buf->num_oprands = 1;
    const char* sour;
    if(sexp_match(next, "(L?", &sour))
    {
        __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_OBJECT), __DI_REGNUM(sour));
    }
    else return -1;
    return 0;
}
__DI_CONSTRUCTOR(GOTO)
{
    buf->opcode = DVM_GOTO;
    buf->num_oprands = 1;
    const char* label;
    next = sexp_strip(next, DALVIK_TOKEN_16, DALVIK_TOKEN_32, NULL);   /* We don't care the size of offest */
    if(sexp_match(next, "(L?", &label))   /* goto label */
    {
        int lid = dalvik_label_get_label_id(label);
        __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_LABEL) | DVM_OPERAND_FLAG_CONST, label);
    }
    else return -1;
    return 0;
}
__DI_CONSTRUCTOR(PACKED)
{
    buf->opcode = DVM_SWITCH;
    const char *reg, *begin;
    if(sexp_match(next, "(L=L?L?A", DALVIK_TOKEN_SWITCH, &reg, &begin, &next))/* (packed-switch reg begin label1..label N) */
    {
        const char* label;
        vector_t*   jump_table;
        __DI_SETUP_OPERAND(0, DVM_OPERAND_FLAG_TYPE(DVM_OPERAND_TYPE_LABELVECTOR) |
                              DVM_OPERAND_FLAG_CONST,
                              jump_table = vector_new(sizeof(uint32_t)));
        if(NULL == jump_table) return -1;
        while(SEXP_NIL != next)
        {
            if(!sexp_match(next, "(L?A", &label, next)) 
            {
                vector_free(jump_table);
                return -1;
            }
            
            int lid = dalvik_label_get_label_id(label);

            if(lid < 0) 
            {
                vector_free(jump_table);
                return -1;
            }

            vector_pushback(jump_table, &lid);
        }
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
#define __DI_CASE(kw) else if(firstword == DALVIK_TOKEN_##kw){ rc = _dalvik_instruction_##kw(next, buf); }
#define __DI_END else rc = -1;
    __DI_BEGIN
        __DI_CASE(NOP)
        __DI_CASE(MOVE)
        __DI_CASE(RETURN)
        __DI_CASE(CONST)
        __DI_CASE(MONITOR)
        __DI_CASE(CHECK)
        __DI_CASE(THROW)
        __DI_CASE(GOTO)
        __DI_CASE(PACKED)
        //__DI_CASE(SPARSE)
        /* 
         * TODO:
         * 1. test vector
         * 2. test label pool
         * 3. test packed parser & test goto
         * 4. finish sparse
         * 5. unfished : from instance-of to fill-array-data 
         */

    __DI_END
#undef __DI_END
#undef __DI_CASE
#undef __DI_BEGIN
    return rc;
}

void dalvik_instruction_free(dalvik_instruction_t* buf)
{
    if(NULL == buf) return;
    int i;
    for(i = 0; i < buf->num_oprands; i ++)
    {
        if(buf->operands[i].header.info.is_const &&
           buf->operands[i].header.info.type == DVM_OPERAND_TYPE_LABELVECTOR)
            vector_free(buf->operands[i].payload.branches);
    }
}
