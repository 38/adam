#include <dalvik/dalvik_type.h>
#include <stdlib.h>
#include <log.h>
#include <string.h>
#include <dalvik/dalvik_tokens.h>
#include <debug.h>

dalvik_type_t* dalvik_type_atom[DALVIK_TYPECODE_NUM_ATOM];
static inline dalvik_type_t* _dalvik_type_alloc(int typecode)
{
    dalvik_type_t* ret = (dalvik_type_t*)malloc(sizeof(dalvik_type_t));
    if(NULL == ret) return NULL;
    memset(ret, 0, sizeof(dalvik_type_t));
    ret->typecode = typecode;
    return ret;
}
static inline dalvik_type_t* _dalvik_type_new(int typecode)
{
    if(DALVIK_TYPE_IS_ATOM(typecode)) return dalvik_type_atom[typecode];
    else return _dalvik_type_alloc(typecode);
}
void dalvik_type_init(void)
{
    int i;
    for(i = 0; i < DALVIK_TYPECODE_NUM_ATOM; i ++)
    {
        dalvik_type_atom[i] = _dalvik_type_alloc(i);
        if(NULL != dalvik_type_atom[i])
            LOG_DEBUG("Assigned memory@0x%x to atmoic type %d", dalvik_type_atom[i], i);
        else 
            LOG_FATAL("Unable to create a new atmoc type");
    }
}

void dalvik_type_finalize(void)
{
    int i;
    for(i = 0; i < DALVIK_TYPECODE_NUM_ATOM; i ++)
        free(dalvik_type_atom[i]);
}

dalvik_type_t* dalvik_type_from_sexp(sexpression_t* sexp)
{
   const char* curlit;
   LOG_DEBUG("parsing type %s", sexp_to_string(sexp, NULL));
   if(sexp_match(sexp, "L?", &curlit))  /* A single literal ? atom */
   {
       if(curlit == DALVIK_TOKEN_VOID)
           return DALVIK_TYPE_VOID;
       else if(curlit == DALVIK_TOKEN_INT)
           return DALVIK_TYPE_INT;
       else if(curlit == DALVIK_TOKEN_WIDE)
           return DALVIK_TYPE_WIDE;
       else if(curlit == DALVIK_TOKEN_LONG)
           return DALVIK_TYPE_LONG;
       else if(curlit == DALVIK_TOKEN_SHORT)
           return DALVIK_TYPE_SHORT;
       else if(curlit == DALVIK_TOKEN_BYTE)
           return DALVIK_TYPE_BYTE;
       else if(curlit == DALVIK_TOKEN_CHAR)
           return DALVIK_TYPE_CHAR;
       else if(curlit == DALVIK_TOKEN_BOOLEAN)
           return DALVIK_TYPE_BOOLEAN;
       else if(curlit == DALVIK_TOKEN_FLOAT)
           return DALVIK_TYPE_FLOAT;
       else if(curlit == DALVIK_TOKEN_DOUBLE)
           return DALVIK_TYPE_DOUBLE;
       else 
           return NULL;
   }
   if(sexp_match(sexp, "(L?A", &curlit, &sexp))
   {
       if(curlit == DALVIK_TOKEN_OBJECT)  /* [object a/b/c] */
       {
           dalvik_type_t* ret;
           ret = _dalvik_type_alloc(DALVIK_TYPECODE_OBJECT);
           if(NULL == ret) return NULL;
           if(NULL == (ret->data.object.path = sexp_get_object_path(sexp,NULL)))
           {
               dalvik_type_free(ret);
               return NULL;
           }
           return ret;
       }
       else if(curlit == DALVIK_TOKEN_ARRAY)  /* [array ...] */
       {
           dalvik_type_t* ret;
           ret = _dalvik_type_alloc(DALVIK_TYPECODE_ARRAY);
           if(NULL == ret) return NULL;
           
           /* We have too unpack the sexp first */

           if(sexp_match(sexp, "(_?", &sexp) == 0) return NULL;
           
           if(NULL == (ret->data.array.elem_type = dalvik_type_from_sexp(sexp)))
           {
               dalvik_type_free(ret);
               return NULL;
           }
           return ret;
       }
   }
   return NULL;
}

void dalvik_type_free(dalvik_type_t* type)
{
    if(NULL == type) return;
    if(DALVIK_TYPE_IS_ATOM(type->typecode)) return;
    switch(type->typecode)
    {
        case DALVIK_TYPECODE_ARRAY:
            dalvik_type_free(type->data.array.elem_type);
        default: 
            free(type);
    }
}

int dalvik_type_equal(const dalvik_type_t* left, const dalvik_type_t* right)
{
    if(NULL == left || NULL == right) 
        return left == right;   /* if left and right are nulls, we just return true */
    if(DALVIK_TYPE_IS_ATOM(left->typecode))
    {
        /* atomic type is singleton */
        return left == right;
    }
    /* if two type are not same */
    if(left->typecode != right->typecode) return 0;
    switch(left->typecode)
    {
        case DALVIK_TYPECODE_ARRAY:
            return dalvik_type_equal(left->data.array.elem_type, 
                                     right->data.array.elem_type);
        case DALVIK_TYPECODE_OBJECT:
            return left->data.object.path == right->data.object.path;
        default:
            LOG_ERROR("unknown type");
            return -1;
    }
}
hashval_t dalvik_type_hashcode(const dalvik_type_t* type)
{
    if(NULL == type) return 0xe7c54276ul;   /* if there's no type, just return a magic number */
    if(DALVIK_TYPE_IS_ATOM(type->typecode))
    {
        /* for a signleton, the hashcode is based on the memory address */
        return (((uintptr_t)type)&0xfffffffful) * MH_MULTIPLY;
    }
    switch(type->typecode)
    {
        case DALVIK_TYPECODE_ARRAY:
            return (dalvik_type_hashcode(type->data.array.elem_type) * MH_MULTIPLY) ^ 0x375cf68cul;
        case DALVIK_TYPECODE_OBJECT:
            return ((uintptr_t)type->data.object.path ^ 0x5fc7f961ul) * MH_MULTIPLY;
        default:
            LOG_ERROR("unknown type");
            return 0;
    }
}
hashval_t dalvik_type_list_hashcode(dalvik_type_t * const * typelist)
{
    int i;
    hashval_t h = 0;
    if(NULL == typelist) return 0x7c5b32f7ul;   /* just a magic number */
    for(i = 0; typelist[i] != NULL; i ++)
    {
        h ^= dalvik_type_hashcode(typelist[i]);
        h *= MH_MULTIPLY;
    }
    return h;
}
int dalvik_type_list_equal(dalvik_type_t * const * left, dalvik_type_t * const * right)
{
    if(NULL == left || NULL == right)
        return left == right;
    int i;
    for(i = 0; left[i] != NULL && right[i] != NULL; i ++)
        if(dalvik_type_equal(left[i], right[i]) == 0)
            return 0;
    /* Because the only possiblity of left[i] == right[i] here is both variable is NULL */
    return left[i] == right[i];
}
