#include <log.h>
#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>

static cesk_value_t*  _cesk_value_list = NULL;

static cesk_value_t*  _cesk_value_numeric_values_storage[3] = {};
static cesk_value_t** _cesk_value_numeric_values = NULL; 

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
    switch(val->type)
    {
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
            break;
        case CESK_TYPE_OBJECT:
            if(*(cesk_object_t**)val->data != NULL)
                cesk_object_free(*(cesk_object_t**)val->data);
            break;
        case CESK_TYPE_ARRAY:
            /* TODO: free the array */
            LOG_INFO("fixme: array value not disposed");
            break;
        case CESK_TYPE_SET:
			cesk_set_free(*(cesk_set_t**)val->data);
            break;
        default:
            LOG_WARNING("unknown type %d, do not know how to free", val->type);
    }
    free(val);
}
void cesk_value_finalize()
{
    cesk_value_t *ptr;
    for(ptr = _cesk_value_list; NULL != ptr; )
    {
        cesk_value_t* old = ptr;
        ptr = ptr->next;
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
    cesk_object_t* class = cesk_object_new(classpath);
    if(NULL == class) 
    {
        *(void**)ret->data = NULL;
        LOG_ERROR("can not create class %s", classpath);
        return NULL;
    }
    *(cesk_object_t**)ret->data = class;
    return ret;
}
cesk_value_t* cesk_value_empty_set()
{
	cesk_value_t* ret = _cesk_value_alloc(CESK_TYPE_SET);
	
	cesk_set_t* empty_set = cesk_set_empty_set();

	if(NULL == empty_set)
	{
		LOG_ERROR("failed to create an empty set for new value");
		*(void**)ret->data = NULL;
		return NULL;
	}
	*(cesk_set_t**)ret->data = empty_set;
	return ret;
}
cesk_value_t* cesk_value_from_operand(dalvik_operand_t* operand)
{
    if(NULL == operand) return NULL;
    if(!operand->header.info.is_const)
    {
        LOG_ERROR("can not create a value from a non-constant operand");
        return NULL;
    }
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
    LOG_ERROR("unknown type");
    return NULL;
}
// Knuth's multiptive hash function 
hashval_t cesk_value_hashcode(cesk_value_t* value)
{
    if(NULL == value) return (hashval_t)0x3c4fab47;
    switch(value->type)
    {
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
            /* atomic value */
            return MH_MULTIPLY * ((uintptr_t)value&0xfffffffful);
        case CESK_TYPE_OBJECT:
            /* object */
            return cesk_object_hashcode(*(cesk_object_t**)value->data);
        case CESK_TYPE_SET:
            return cesk_set_hashcode(*(cesk_set_t**)value->data);
        case CESK_TYPE_ARRAY:
            /* array */
            LOG_INFO("fixme: array type support");
            //return cesk_set_hashcode((*(cesk_value_array_t**)value->data)->values);
        default:
            return 0;
    }
}
int cesk_value_equal(cesk_value_t* first, cesk_value_t* second)
{
    if(NULL == first || NULL == second) return first == second;
    if(first->type != second->type) return 0;
    switch(first->type)
    {
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
            return first == second;
        case CESK_TYPE_OBJECT:
            return cesk_object_equal(*(cesk_object_t**)first->data, *(cesk_object_t**)second->data);
        case CESK_TYPE_SET:
            return cesk_set_equal(*(cesk_set_t**)first->data, *(cesk_set_t**)second->data);
        case CESK_TYPE_ARRAY:
            LOG_INFO("fixme : array type support");
        default:
            LOG_WARNING("can not compare value type %d", first->type);
            return 1;
    }
}
