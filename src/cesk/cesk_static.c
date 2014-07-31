/**
 * @file cesk_static.c
 * @brief the implementation of static field table
 **/
#include <dalvik/dalvik.h>

#include <cesk/cesk_static.h>
#include <cesk/cesk_value.h>
/**
 * @brief a table of prime number that can be selected as size of the static table
 * @note the size of the static table should be larger than the total number of static
 *       fields in the source code
 **/
static const uint32_t _cesk_static_table_primes[] = {
	2, 5, 11, 17, 37, 67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411, 
	32771, 65537, 131101, 262147, 524309, 1048583, 2097169, 4194319    
};
/**
 * @brief the node type in the static field table
 **/
typedef struct _cesk_static_table_node_t _cesk_static_table_node_t;
struct _cesk_static_table_node_t {
	uint32_t    index;                 /*!< the index of this field*/
	cesk_set_t* value_set;             /*!< the value of of this field */
	_cesk_static_table_node_t* hash_next; /*!< the next node in the hash table */
	_cesk_static_table_node_t* list_next; /*!< the next node in the address list */
};

/**
 * @brief the implementation of the static field table
 **/
struct {
	uint32_t refcnt;                        /*!< the reference counter */
	_cesk_static_table_node_t* node_list;   /*!< the head of node list */
	_cesk_static_table_node_t* hash[0];     /*!< the hash table slots */
} _cesk_static_table_t;

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

/** 
 * @brief this variable determine the number of slots in the hash table
 * @note  0 means this size remains undetermined
 **/
static uint32_t _cesk_static_table_size = 0;

/* local inline functions */


/**
 * @brief look for the static field in the member dict and translate the default value to cesk value
 * @param class the name of the target class
 * @param field the field name of the target
 * @param initval wether or not the caller needs the initval
 * @return the field index, <0 indicates an error 
 **/
static inline int _cesk_static_query_field(const char* class, const char* field, int initval)
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
				LOG_TRACE("fixme: method <strinit> is to be implemented");
				addr = CESK_STORE_ADDR_ZERO;
				break;
CONST_VALUE:
				if(intval > 0) addr = CESK_STORE_ADDR_POS;
				else if(intval < 0) addr = CESK_STORE_ADDR_NEG;
				else addr = CESK_STORE_ADDR_ZERO;
		}
		/* finally set the value */
		_cesk_static_default_value[index] = addr;
	}
	return index;
}

/* Interface implementation */

int cesk_static_init()
{
	extern const uint32_t dalvik_static_field_count;
	int i;
	for(i = 0; 
	    i < sizeof(_cesk_static_table_primes)/sizeof(*_cesk_static_table_primes) &&
		_cesk_static_table_primes[i] < dalvik_static_field_count;
		i ++);
	_cesk_static_table_size = _cesk_static_table_primes[i];
	LOG_DEBUG("The size of the static field table is %d", _cesk_static_table_size);
	_cesk_static_default_value = (uint32_t*)malloc(sizeof(uint32_t) * dalvik_static_field_count);
	if(NULL == _cesk_static_default_value)
	{
		LOG_ERROR("can not allocate memory for default value list");
		return -1;
	}
	return 0;
}
void cesk_static_finalize()
{
	if(NULL != _cesk_static_default_value) free(_cesk_static_default_value);
}
cesk_static_table_t* cesk_static_table_new()
{
	// TODO
	return NULL;
}
