#include <log.h>
#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>

static cesk_value_t*  _cesk_value_list = NULL;

static cesk_value_t*  _cesk_value_numeric_values_storage[3] = {};
static cesk_value_t** _cesk_value_numeric_values = NULL; 

static cesk_value_t*  _cesk_value_string = NULL;

static cesk_value_t* _cesk_value_boolean[2] = {};

static inline cesk_value_t* _cesk_value_alloc(uint32_t type)
{
    size_t size = sizeof(cesk_value_t);
    switch(type)
    {
        case CESK_TYPE_NUMERIC:
            size += sizeof(cesk_value_numeric_t);
            break;
        case CESK_TYPE_BOOLEAN:
            size += sizeof(cesk_value_boolean_t);
            break;
        case CESK_TYPE_OBJECT:
            size += sizeof(cesk_object_t*);
            break;
        case CESK_TYPE_ARRAY:
            size += sizeof(cesk_value_array_t);
            break;
        case CESK_TYPE_SET:
            size += sizeof(cesk_set_t*);
            break;
        default:
            LOG_ERROR("unknown type %d", type);
            return NULL;
    }
    cesk_value_t* ret = (cesk_value_t*)malloc(size);
    if(NULL == ret) return NULL;
    ret->type = type;
    ret->refcnt = 0;
    ret->next = _cesk_value_list;
    ret->prev = NULL;
    if(_cesk_value_list) 
        _cesk_value_list->prev = ret;
    _cesk_value_list = ret;
    return ret;
}
void cesk_value_init()
{
    _cesk_value_list = NULL;
    _cesk_value_numeric_values = _cesk_value_numeric_values_storage + 1;
    if(NULL == (_cesk_value_numeric_values[CESK_VALUE_NUMERIC_NEGATIVE] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))

    {
        LOG_FATAL("Can not create abstract value for negative value");
    }
    else 
        *(cesk_value_numeric_t*)_cesk_value_numeric_values[CESK_VALUE_NUMERIC_NEGATIVE]->data = CESK_VALUE_NUMERIC_NEGATIVE;
    if(NULL == (_cesk_value_numeric_values[CESK_VALUE_NUMERIC_ZERO] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value for zero value");
    }
    else
        *(cesk_value_numeric_t*)_cesk_value_numeric_values[CESK_VALUE_NUMERIC_ZERO]->data = CESK_VALUE_NUMERIC_ZERO;

    if(NULL == (_cesk_value_numeric_values[CESK_VALUE_NUMERIC_POSITIVE] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value for positive value");
    }
    else
        *(cesk_value_numeric_t*)(_cesk_value_numeric_values[CESK_VALUE_NUMERIC_POSITIVE]->data) = CESK_VALUE_NUMERIC_POSITIVE;
    
    if(NULL == (_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE] = _cesk_value_alloc(CESK_TYPE_BOOLEAN)))
    {
        LOG_FATAL("Can not create abstract value for true");
    }
    else
        *(cesk_value_boolean_t*)(_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE]->data) = CESK_VALUE_BOOLEAN_TRUE;

    
    if(NULL == (_cesk_value_boolean[CESK_VALUE_BOOLEAN_FALSE] = _cesk_value_alloc(CESK_TYPE_BOOLEAN)))
    {
        LOG_FATAL("Can not create abstract value for false");
    }
    else
        *(cesk_value_boolean_t*)(_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE]->data) = CESK_VALUE_BOOLEAN_FALSE;

}
static void _cesk_value_free(cesk_value_t* val)
{
    if(val->prev) val->prev->next = val->next;
    if(val->next) val->next->prev = val->prev;
    if(_cesk_value_list == val) _cesk_value_list = val->next;
    
    free(val);
}
void cesk_value_finalize()
{
    cesk_value_t *ptr;
    for(ptr = _cesk_value_list; NULL != ptr; )
    {
        cesk_value_t* old = ptr;
        ptr = ptr->next;
        switch(old->type)
        {
            case CESK_TYPE_NUMERIC:
            case CESK_TYPE_BOOLEAN:
                break;
            case CESK_TYPE_OBJECT:
                if(*(cesk_object_t**)old->data != NULL)
                    cesk_object_free(*(cesk_object_t**)old->data);
                break;
            case CESK_TYPE_ARRAY:
                /* TODO: free the array */
                LOG_INFO("fixme: array value not disposed");
                break;
            case CESK_TYPE_SET:
                LOG_INFO("fixme: set value not disposed");
                break;
            default:
                LOG_WARNING("unknown type %d, do not know how to free", old->type);
        }
        _cesk_value_free(old);
    }
}

void cesk_value_incref(cesk_value_t* value)
{
    value->refcnt ++;
}
void cesk_value_decref(cesk_value_t* value)
{
    if(--value->refcnt == 0)
        _cesk_value_free(value);
}
cesk_value_t* cesk_value_from_classpath(const char* classpath)
{
    cesk_value_t* ret = _cesk_value_alloc(CESK_TYPE_OBJECT);
    if(NULL == ret) return ret;
    *(void**)ret->data = NULL;
    cesk_object_t* class = cesk_object_new(classpath);
    if(NULL == class) 
    {
        LOG_ERROR("can not create class %s", classpath);
        return NULL;
    }
    return ret;
}

cesk_value_t* cesk_value_from_operand(dalvik_operand_t* operand)
{
    cesk_value_t* ret = NULL;
    if(NULL == operand) return NULL;
    if(!operand->header.info.is_const)
    {
        LOG_ERROR("can not create a value from a non-constant operand");
        return NULL;
    }
    int typecode = -1;
    int intval = 0;
    switch(operand->header.info.type)
    {
        case DVM_OPERAND_TYPE_LONG:
            intval = operand->payload.int64;
            goto num;
        case DVM_OPERAND_TYPE_SHORT:
            intval = operand->payload.int16;
            goto num;
        case DVM_OPERAND_TYPE_INT:
            intval = operand->payload.int32;
            goto num;
        case DVM_OPERAND_TYPE_BYTE:
            intval = operand->payload.int8;
            goto num;
        case DVM_OPERAND_TYPE_DOUBLE:
            if(operand->payload.real64 < -1e-20)
                intval = -1;
            else if(operand->payload.real64 > 1e-20)
                intval = 1;
            else 
                intval = 0;
            goto num;
        case DVM_OPERAND_TYPE_FLOAT:
            if(operand->payload.real32 < -1e-20)
                intval = -1;
            else if(operand->payload.real32 > 1e-20)
                intval = 1;
            else 
                intval = 0;
            goto num;
        case DVM_OPERAND_TYPE_CHAR:
            intval = operand->payload.int8;
num:
            if(intval > 0)
                return _cesk_value_numeric_values[1];
            else if(intval < 0)
                return _cesk_value_numeric_values[-1];
            else 
                return _cesk_value_numeric_values[0];
        case DALVIK_TYPECODE_BOOLEAN:
            if(operand->payload.uint8)
                return _cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE];
            else
                return _cesk_value_boolean[CESK_VALUE_BOOLEAN_FALSE];
            break;
        default:
            LOG_ERROR("can not create value for the type");
            return NULL;
    }
    return NULL;
}
cesk_value_t* cesk_value_fork(cesk_value_t* value)
{
    if(value->type != CESK_TYPE_OBJECT && value->type != CESK_TYPE_ARRAY)
    {
        LOG_ERROR("Can not fork a basic type");
    }
    cesk_value_t* newval = _cesk_value_alloc(value->type);
    if(NULL == newval) return NULL;
    if(value->type == CESK_TYPE_OBJECT)
    {
        cesk_object_t* object = *(cesk_object_t**)value->data;
        cesk_object_t* newobj = cesk_object_fork(object);
        if(NULL == newobj)
        {
            free(newval);
            return NULL;
        }
        *(cesk_object_t**)newval->data = newobj;
        return newval;
    }
    else if(value->type == CESK_TYPE_ARRAY)
    {
        //TODO: finish fork for an array
        LOG_ERROR("fixme: fork an array");
        return NULL;
    }
    else if(value->type == CESK_TYPE_SET)
    {
        //TODO: forking a set
        LOG_ERROR("fixme: fork a set");
        return NULL;
    }
}
#if 0
// Knuth's multiptive hash function 
hashval_t cesk_value_hashcode(cesk_value_t* value)
{
    if(NULL == value) return (hashval_t)0x3c4fab47;
    switch(value->type)
    {
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
        case CESK_TYPE_STRING:
            /* atomic value */
            return 2654435761ul * ((uint64_t)value&0xfffffffful);
        case CESK_TYPE_OBJECT:
            /* object */
            return cesk_object_hashcode(*(cesk_object_t**)value->data);
        case CESK_TYPE_ARRAY:
            /* array */
            return cesk_value_set_hashcode((*(cesk_value_array_t**)value->data)->values);
        default:
            return 0;
    }
}
#endif
