/**
 * @file cesk_static.c
 * @brief the implementation of static field table
 **/
#include <dalvik/dalvik.h>

#include <cesk/cesk_static.h>
#include <cesk/cesk_value.h>
/**
 * @brief the implementation of the static field table
 **/
struct {
	uint32_t refcnt;

} _cesk_static_table_t;
#if 0
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
static uint32_t _cesk_static_default_value[CESK_STORE_ADDR_STATIC_SIZE];

/* local inline functions */


/**
 * @brief look for the static field in the member dict and translate the default value to cesk value
 * @param class the name of the target class
 * @param field the field name of the target
 * @param initval wether or not the caller needs the initval
 * @return the field index, <0 indicates an error 
 **/
static inline int _cesk_static_query_static_field(const char* class, const char* field, int initval)
{
	const dalvik_field_t* field = dalvik_memberdict_get_field(class, field);
	if(NULL == field) 
	{
		LOG_ERROR("can not find the field named %s.%s", class, field);
		return -1;
	}
	if(0 == (field->attrs & DALVIK_ATTRS_STATIC))
	{
		LOG_ERROR("field %s.%s is not a static field", class, field);
		return -1;
	}
	/* then we know the index of this field */
	uint32_t index = field->offset;
	LOG_DEBUG("found static field map %s.%s --> 0x%", class, field, index);
	if(index >= CESK_STORE_ADDR_STATIC_SIZE)
	{
		LOG_ERROR("the field index(0x%x) is beyound the static address boundary(0x%x), which must be a illegal static field", index, CESK_STORE_ADDR_STATIC_SIZE);
		return -1;
	}
	
	/* if the initial value is already ready, or the caller do not need a initial value, just return the index */
	if(initval == 0 || 0 != _cesk_static_default_value[index]) return index;

	/* otherwise we need to parse the initial value for this field */
	const sexpression_t* def_val = field->default_value;

	/* if there's no default value, the default value for this is ZERO */
	if(SEXP_NIL == def_val)
	{
		_cesk_static_default_value[index] = CESK_STORE_ADDR_ZERO;
	}
	else
	{
		const char* literal;
		if(!sexp_match(def_val, "S?", &literal))
		{
			LOG_ERROR("invalid default value %s", sexp_to_string(def_val, NULL, 0));
			return -1;
		}
		switch(field->type->typecode)
		{
			case DALVIK_TYPECODE_INT:

		}
	}
	return index;
}
#endif
