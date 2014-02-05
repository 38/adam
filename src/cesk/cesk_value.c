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
        LOG_FATAL("Can not create abstract value");
        return;
    }
    *(cesk_value_numeric_t*)_cesk_value_numberic_values[CESK_VALUE_NUMERIC_NEGATIVE]->data = CESK_VALUE_NUMERIC_NEGATIVE;

    if(NULL == (_cesk_value_numberic_values[CESK_VALUE_NUMERIC_ZERO] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value");
        return;
    }
    *(cesk_value_numeric_t*)_cesk_value_numberic_values[CESK_VALUE_NUMERIC_ZERO]->data = CESK_VALUE_NUMERIC_ZERO;

    if(NULL == (_cesk_value_numberic_values[CESK_VALUE_NUMERIC_POSITIVE] = _cesk_value_alloc(CESK_TYPE_NUMERIC)))
    {
        LOG_FATAL("Can not create abstract value");
        return;
    }
    *(cesk_value_numeric_t*)_cesk_value_numberic_values[CESK_VALUE_NUMERIC_POSITIVE]->data = CESK_VALUE_NUMERIC_POSITIVE;

    //TODO: initialization of other atmoic types, LAST 
}
void cesk_value_finalize()
{

}
hashval_t cesk_value_hashcode(cesk_value_t* value)
{
    if(NULL == value) return (hashval_t)0x3c4fab47;
    switch(value->type)
    {
        
    }
}
hashval_t cesk_value_set_hashcode(cesk_value_set_t* value)
{
    //TODO
}
