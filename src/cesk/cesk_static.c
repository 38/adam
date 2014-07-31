/**
 * @file cesk_static.c
 * @brief the implementation of static field table
 **/
#include <dalvik/dalvik.h>

#include <cesk/cesk_static.h>
#include <cesk/cesk_value.h>
/* typeps */

/**
 * @brief the node in the skiplist 
 **/
typedef struct _cesk_static_skiplist_node_t _cesk_static_skiplist_node_t;

struct _cesk_static_skiplist_node_t{
	uint32_t index:(2-1);
};
/* global static variables */
/**
 * @brief the default value for each static fields
 * @note  because the static field which contains an object value is actually
 *        initialized by the function <clinit> so that this initial value must 
 *        be an constant address.
 *        Notice that the constant address should never be 0. So that if we found
 *        an 0x0 in the array, that means we should parse the initial value for this
 *        field
 * @todo  initialize with the interger value address space 
 **/
static uint32_t* _cesk_static_default_value;


/* local inline functions */


/* Interface implementation */

int cesk_static_init()
{
	extern const uint32_t dalvik_static_field_count;
	_cesk_static_default_value = (uint32_t*)malloc(sizeof(uint32_t) * dalvik_static_field_count);
	if(NULL == _cesk_static_default_value)
	{
		LOG_ERROR("can not allocate memory for default value list");
		return -1;
	}
	memset(_cesk_static_default_value, -1, sizeof(uint32_t) * dalvik_static_field_count);
	return 0;
}
void cesk_static_finalize()
{
	if(NULL != _cesk_static_default_value) free(_cesk_static_default_value);
}
int cesk_static_query_field(const char* class, const char* field, int initval)
{
	const dalvik_field_t* field_desc = dalvik_memberdict_get_field(class, field);
	if(NULL == field_desc) 
	{
		LOG_ERROR("can not find the field named %s.%s", class, field);
		return -1;
	}
	if(0 == (field_desc->attrs & DALVIK_ATTRS_STATIC))
	{
		LOG_ERROR("field %s.%s is not a static field", class, field);
		return -1;
	}
	/* then we know the index of this field */
	uint32_t index = field_desc->offset;
	LOG_DEBUG("found static field map %s.%s --> 0x%x", class, field, index);
	if(index >= CESK_STORE_ADDR_STATIC_SIZE)
	{
		LOG_ERROR("the field index(0x%x) is beyound the static address boundary(0x%x), "
				  "which must be a illegal static field", 
				  index, (uint32_t)CESK_STORE_ADDR_STATIC_SIZE);
		return -1;
	}
	
	/* if the initial value is already ready, or the caller do not need a initial value, just return the index */
	if(initval == 0 || 0 != _cesk_static_default_value[index]) return index;

	/* otherwise we need to parse the initial value for this field */
	const sexpression_t* def_val = field_desc->default_value;

	/* if there's no default value, the default value for this is ZERO */
	if(SEXP_NIL == def_val)
	{
		_cesk_static_default_value[index] = CESK_STORE_ADDR_ZERO;
	}
	/* otherwise we have an literal here */
	else
	{
		const char* init_str;
		if(!sexp_match(def_val, "S?", &init_str))
		{
			LOG_ERROR("invalid default value %s", sexp_to_string(def_val, NULL, 0));
			return -1;
		}
		int intval;
		double fltval;
		uint32_t addr = CESK_STORE_ADDR_NULL;
		switch(field_desc->type->typecode)
		{
			case DALVIK_TYPECODE_INT:
			case DALVIK_TYPECODE_LONG:
			case DALVIK_TYPECODE_BYTE:
			case DALVIK_TYPECODE_SHORT:
				intval = atoi(init_str);
				goto CONST_VALUE;
			case DALVIK_TYPECODE_BOOLEAN:
				intval = (DALVIK_TOKEN_TRUE == init_str);
				goto CONST_VALUE;
			case DALVIK_TYPECODE_FLOAT:
			case DALVIK_TYPECODE_DOUBLE:
				fltval = atof(init_str);
				/* current we treat all value type just as 3 value NEG/ZERO/POS */
				intval = (int)fltval;
				goto CONST_VALUE;
			case DALVIK_TYPECODE_ARRAY:
				/* suppose to be null */
				intval = 0;
				goto CONST_VALUE;
			case DALVIK_TYPECODE_OBJECT:
				/* the only thing we needs to process is string, because string value are initialized directly rather than in <clinit>
				 * but actually we can create an virtual method called <strinit> just like the clinit to do this, so that we just 
				 * keep this value (null)*/
				/* TODO */
				LOG_TRACE("fixme: method <strinit> is to be implemented");
				addr = CESK_STORE_ADDR_ZERO;
				break;
CONST_VALUE:
				if(intval > 0) addr = CESK_STORE_ADDR_POS;
				else if(intval < 0) addr = CESK_STORE_ADDR_NEG;
				else addr = CESK_STORE_ADDR_ZERO;
				break;
			default:
				LOG_ERROR("unsupported initializer type %d", field_desc->type->typecode); 
		}
		/* finally set the value */
		_cesk_static_default_value[index] = addr;
	}
	return index;
}
