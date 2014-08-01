/**
 * @file cesk_static.c
 * @brief the implementation of static field table
 **/
#include <stdlib.h>
#include <time.h>

#include <dalvik/dalvik.h>

#include <cesk/cesk_static.h>
#include <cesk/cesk_value.h>
/* typeps */

/** @brief how many pointers do a static field table have **/
#define NPOINTERS (1<<CESK_STATIC_SKIP_LIST_DEPTH)

/** @brief return wether or not use array as data structure for the table */
#ifndef ALWAYS_SKIPLIST
#	define USE_ARRAY  0
#else
#	define USE_ARRAY (dalvik_static_field_count <= NPOINTERS)
#endif


/**
 * @brief the node in the skiplist 
 **/
typedef struct _cesk_static_skiplist_node_t _cesk_static_skiplist_node_t;
/**
 * @brief actual data structure for _cesk_static_skiplist_node_t
 **/
struct _cesk_static_skiplist_node_t{
	uint32_t index;                                         /*!< the static field index */
	uint32_t refcnt:(32 - CESK_STATIC_SKIP_LIST_DEPTH);     /*!< the refcnt of this node */
	uint32_t order :CESK_STATIC_SKIP_LIST_DEPTH;            /*!< how many pointers does this node have */
	_cesk_static_skiplist_node_t* pointers[0];              /*!< the pointer array */
};
CONST_ASSERTION_SIZE(_cesk_static_skiplist_node_t, pointers, 0);
CONST_ASSERTION_LAST(_cesk_static_skiplist_node_t, pointers);

/**
 * @brief actual data structure for cesk_static_table_t
 **/
struct _cesk_static_table_t{
	hashval_t hashcode;                                    /*!< the hashcode of this table */
	union{
		_cesk_static_skiplist_node_t* skiplist[NPOINTERS]; /*!< the pointer array used by the skiplist */
		cesk_set_t*                   array   [NPOINTERS]; /*!< the array impmenetation */
	} data;                                                /*!< the data field of this table */
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

/** @brief how many field do i have? **/
extern const uint32_t dalvik_static_field_count;


/* local inline functions */
/**
 * @breif generate an randomized order
 * @return the order number
 **/
static inline uint32_t _cesk_static_skiplist_get_order()
{
	static int msk = 0;
	static int rnd = 0;
	int ret;
	for(ret = 1; ret <= NPOINTERS; ret ++)
	{
		/* if there's no random bit avaible, generate a new random number */
		if(0 == msk)
		{
			rnd = rand();
			msk = (RAND_MAX >> 1) + 1;
		}
		int rand_bit = rnd & msk;
		msk >>= 1;
		if(!rand_bit) break;
	}
	return ret;
}
/**
 * @breif create a new skip list node
 * @param index the field index 
 * @return the newly created node
 **/
static inline _cesk_static_skiplist_node_t* _cesk_static_skiplist_node_new(uint32_t index)
{
	uint32_t order = _cesk_static_skiplist_get_order();
	size_t   size = sizeof(_cesk_static_skiplist_node_t) + sizeof(_cesk_static_skiplist_node_t*) * order;
	_cesk_static_skiplist_node_t* ret = (_cesk_static_skiplist_node_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate new skiplit node for field %d", index);
		goto ERR;
	}
	ret->order = order;
	ret->refcnt = 0;
	ret->index = index;
	memset(ret->pointers, 0, size);
	return ret;
ERR:
	if(NULL != ret) free(ret);
	return NULL;
}
/**
 * @biref insert a new node to the skiplist
 * @param list the target skiplist
 * @param index the index to inerst
 * @note  this function does nothing aoubt the hashcode,
 *        so that caller is responsible for updating the
 *        hashcode
 *        And we assume that this node is not exist in the list before this insertion
 * @return the new node created for this index, NULL indicates error 
 **/
static inline _cesk_static_skiplist_node_t* _cesk_static_skiplist_insert(_cesk_static_skiplist_node_t** list, uint32_t index)
{
	_cesk_static_skiplist_node_t* node = _cesk_static_skiplist_node_new(index);
	if(NULL == node)
	{
		LOG_ERROR("can not create list node");
		return NULL;
	}
	/* we should insert the node between begin and end */
	_cesk_static_skiplist_node_t *begin[NPOINTERS + 1] = {}, *end = NULL;
	int i;
	/* find the begin and end node */
	for(i = NPOINTERS - 1; i >= 0; i ++)
	{
		_cesk_static_skiplist_node_t* ptr;
		begin[i] = begin[i + 1];
		for(ptr = begin[i]?begin[i]->pointers[i]:list[i]; end != ptr && ptr->index < index; ptr = ptr->pointers[i])
			begin[i] = ptr;
		end = ptr;
	}
	/* if we need insert this node at the beginning of the list */
	if(NULL == begin[0]) 
	{
		for(i = 0; i < node->order; i ++)
		{
			node->pointers[i] = list[i];
			list[i] = node;
		}
	}
	/* otherwise, insert after the node begin */
	else
	{
		for(i = 0; i < node->order; i ++)
		{
			node->pointers[i] = begin[i]->pointers[i];
			begin[i]->pointers[i] = node;
		}
	}
	return node;
}
/**
 * @brief find a node in the skiplist
 * @param list the skiplist
 * @param index the index to insert
 * @return the list node for this index, NULL indicates nothing found
 **/
static inline _cesk_static_skiplist_node_t* _cesk_static_skiplist_find(_cesk_static_skiplist_node_t** list,  uint32_t index)
{
	int i;
	_cesk_static_skiplist_node_t *begin = NULL, *end = NULL;
	for(i = NPOINTERS - 1; i >= 0; i ++)
	{
		_cesk_static_skiplist_node_t* ptr;
		for(ptr = begin?begin->pointers[i]:list[i]; end != ptr && ptr->index <= index; ptr = ptr->pointers[i])
			begin = ptr;
		end = ptr;
	}
	if(NULL == begin || begin->index != index) return NULL;
	return begin;
}

/* Interface implementation */

int cesk_static_init()
{
	srand((unsigned)time(NULL));
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
