#include <log.h>
#include <cesk/cesk_value.h>
cesk_value_t*  _cesk_value_numberic_values_storage[3] = {};
/* In this way, the function can support [-1] [0] [1] */
cesk_value_t** _cesk_value_numberic_values = _cesk_value_numberic_values_storage + 1;

cesk_value_t*  _cesk_value_string = NULL;

cesk_value_t* _cesk_value_boolean[2] = {};

static inline cesk_value_t* _cesk_value_alloc(uint32_t type)
{
    size_t size = sizeof(cesk_value_t);
    switch(type)
    {
        case CESK_TYPE_NUMERIC:
            size += sizeof(cesk_value_numeric_t);
            break;
        case CESK_TYPE_STRING:
            size += sizeof(cesk_value_string_t);
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
        default:
            LOG_ERROR("unknown type %d", type);
            return NULL;
    }
    cesk_value_t* ret = (cesk_value_t*)malloc(size);
    if(NULL == ret) return NULL;
    ret->type = type;
    return ret;
}
void cesk_value_init()
{
    if(NULL == (_cesk_value_numberic_values[CESK_VALUE_NUMERIC_NEGATIVE] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))

    {
        LOG_FATAL("Can not create abstract value for negative value");
    }
    else 
        *(cesk_value_numeric_t*)_cesk_value_numberic_values[CESK_VALUE_NUMERIC_NEGATIVE]->data = CESK_VALUE_NUMERIC_NEGATIVE;

    if(NULL == (_cesk_value_numberic_values[CESK_VALUE_NUMERIC_ZERO] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value for zero value");
    }
    else
        *(cesk_value_numeric_t*)_cesk_value_numberic_values[CESK_VALUE_NUMERIC_ZERO]->data = CESK_VALUE_NUMERIC_ZERO;

    if(NULL == (_cesk_value_numberic_values[CESK_VALUE_NUMERIC_POSITIVE] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value for positive value");
    }
    else
        *(cesk_value_numeric_t*)(_cesk_value_numberic_values[CESK_VALUE_NUMERIC_POSITIVE]->data) = CESK_VALUE_NUMERIC_POSITIVE;

    if(NULL == (_cesk_value_string = _cesk_value_alloc(CESK_TYPE_STRING)))
    {
        LOG_FATAL("Can not create abstract value for string");
    }
    else
        /* Nothing to Initialize */;

    if(NULL == (_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE] = _cesk_value_alloc(CESK_TYPE_STRING)))
    {
        LOG_FATAL("Can not create abstract value for true");
    }
    else
        *(cesk_value_boolean_t*)(_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE]->data) = CESK_VALUE_BOOLEAN_TRUE;

    if(NULL == (_cesk_value_boolean[CESK_VALUE_BOOLEAN_FALSE] = _cesk_value_alloc(CESK_TYPE_STRING)))
    {
        LOG_FATAL("Can not create abstract value for false");
    }
    else
        *(cesk_value_boolean_t*)(_cesk_value_boolean[CESK_VALUE_BOOLEAN_TRUE]->data) = CESK_VALUE_BOOLEAN_FALSE;

}
void cesk_value_finalize()
{
    int i;
    for(i = 0; i < 3; i ++)
        free(_cesk_value_numberic_values_storage[i]);
    free(_cesk_value_string);
    free(_cesk_value_boolean[0]);
    free(_cesk_value_boolean[1]);
}
hashval_t cesk_value_hashcode(cesk_value_t* value)
{
    if(NULL == value) return (hashval_t)0x3c4fab47;
    switch(value->type)
    {
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
        case CESK_TYPE_STRING:
            /* atomic value */
            return (uint32_t)2654435761 * ((uint64_t)value&0xfffffffful);
        case CESK_TYPE_OBJECT:
            /* object */
            return cesk_object_hashcode(*(cesk_object_t**)value->data);
        case CESK_TYPE_ARRAY:
            return cesk_value_set_hashcode((*(cesk_value_array_t**)value->data)->values);
        default:
            return 0;
    }
}
hashval_t cesk_value_set_hashcode(cesk_value_set_t* value)
{

}
