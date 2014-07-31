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
struct _cesk_static_table_t{
	uint32_t refcnt;                        /*!< the reference counter */
	uint32_t hashcode;                      /*!< current the hashcode */
	_cesk_static_table_node_t* node_list;   /*!< the head of node list */
	_cesk_static_table_node_t* hash[0];     /*!< the hash table slots */
};
/* validate the type */
CONST_ASSERTION_SIZE(cesk_static_table_t, hash, 0);  /* the size for hash should be 0 */
CONST_ASSERTION_LAST(cesk_static_table_t, hash);     /* the field hash should be the last member*/

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
 * @brief allocate a new static table node
 * @param index the index of this static field
 * @return the newly created node, NULL indicates error
 **/
static inline _cesk_static_table_node_t* _cesk_static_table_node_new(uint32_t index)
{
	_cesk_static_table_node_t* ret = (_cesk_static_table_node_t*)malloc(sizeof(_cesk_static_table_node_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate new node for static field table");
		return NULL;
	}
	memset(ret, 0, sizeof(_cesk_static_table_node_t));
	ret->index = index;
	return ret;
}
/**
 * @brief find the static field table node for this index
 * @param table the static field table
 * @param index the index to find
 * @return the pointer to this node, NULL if not found
 **/
static inline _cesk_static_table_node_t* _cesk_static_table_find(cesk_static_table_t* table, uint32_t index)
{
	uint32_t h = index % _cesk_static_table_size;
	_cesk_static_table_node_t* node;
	for(node = table->hash[h]; NULL != node && node->index != index; node = node->hash_next);
	return node;
}
/**
 * @brief compute hashcode for a given node
 * @param node 
 * @return the hashcode computed from this node
 **/
static inline hashval_t _cesk_static_table_node_hashcode(const _cesk_static_table_node_t* node)
{
	return (MH_MULTIPLY * ~node->index) ^ cesk_set_hashcode(node->value_set);
}
/**
 * @brief insert a node for given index in the static field table
 * @param table the static field table
 * @param index the index for this field
 * @param init_val the init value
 * @note  we assume that the index is not exist in this table
 * @return the pointer to newly created node, NULL if an error 
 **/
static inline _cesk_static_table_node_t* _cesk_static_table_insert(cesk_static_table_t* table, uint32_t index, uint32_t init_val)
{
	uint32_t h = index % _cesk_static_table_size;
	_cesk_static_table_node_t* node = _cesk_static_table_node_new(index);
	cesk_set_t* value_set = NULL;
	if(NULL == node)
	{
		LOG_ERROR("can not allocate a new node for index 0x%x", index);
		goto ERR;
	}
	value_set = cesk_set_empty_set();
	if(NULL == value_set)
	{
		LOG_ERROR("can not create a new set for the field value");
		goto ERR;
	}

	/* manipulate the hash table */
	node->hash_next = table->hash[h];
	table->hash[h] = node;

	/* manipulate the node list */
	node->list_next = table->node_list;
	table->node_list = node;

	return node;
ERR:
	if(NULL != value_set) cesk_set_free(value_set);
	if(NULL != node) free(node);
	return NULL;
}
/**
 * @brief commit modification of the node and update the hashcode of this table
 * @param table the static field table
 * @param node  the target node
 * @return result of operation, <0 indicates errors
 **/
static inline int _cesk_static_table_commit(cesk_static_table_t* table, const _cesk_static_table_node_t* node)
{
	table->hashcode ^= _cesk_static_table_node_hashcode(node);
	return 0;
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

cesk_static_table_t* cesk_static_table_new()
{
	size_t size = sizeof(cesk_static_table_t) + sizeof(_cesk_static_table_node_t*) * _cesk_static_table_size;
	cesk_static_table_t* ret = (cesk_static_table_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for a new static field table");
		return NULL;
	}
	/* set the refcnt to 1, because we assume that the caller will use this table */
	ret->refcnt = 1;
	ret->node_list = NULL;
	/* initial hashcode, actually a random value */
	ret->hashcode = 0x73fd5efau;
	/* initialize the slots */
	memset(ret->hash, 0, size - sizeof(cesk_static_table_t));
	return ret;
}
int cesk_static_table_incref(cesk_static_table_t* table)
{
	if(NULL == table)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	table->refcnt ++;
	return 0;
}
int cesk_static_table_decref(cesk_static_table_t* table)
{
	if(NULL == table)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(0 == -- table->refcnt)
	{
		LOG_DEBUG("no one is using this static field table %p, we can delete this object", table);
		/* first we delete the node in this table */
		_cesk_static_table_node_t* ptr;
		for(ptr = table->node_list; NULL != ptr;)
		{
			_cesk_static_table_node_t* current = ptr;
			ptr = ptr->list_next;
			cesk_set_free(current->value_set);
			free(current);
		}
		/* then we can delete the table */
		free(table);
	}
	return 0;
}
const cesk_set_t* cesk_static_table_get_ro(cesk_static_table_t* table, uint32_t addr)
{
	extern const uint32_t dalvik_static_field_count;
	/* we check if or not this address is a valid static field address */
	if(!CESK_STORE_ADDR_IS_STATIC(addr))
	{
		LOG_ERROR("invalid static address "PRSAddr, addr);
		return NULL;
	}
	if(NULL == table)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	uint32_t idx = CESK_STORE_ADDR_STATIC_IDX(addr);

	/* we should validate the value of index first */
	if(idx >= dalvik_static_field_count)
	{
		LOG_ERROR("static field address out of bound: "PRSAddr, addr);
		return NULL;
	}

	/* okay, try to find this field in the table */
	_cesk_static_table_node_t* node = _cesk_static_table_find(table, idx);
	/* if can not found this field, we should initialize a new one */
	if(NULL == node)
	{
		LOG_DEBUG("static field %u not found", idx);
		uint32_t init_val;
		/* no initializer for this address? must be a mistake */
		if(CESK_STORE_ADDR_NULL == (init_val = _cesk_static_default_value[idx]))
		{
			LOG_ERROR("I do not know how to initialize this field, aborting");
			return NULL;
		}
		/* okay, let's do initalization */
		node = _cesk_static_table_insert(table, idx, init_val);
		if(NULL == node) 
		{
			LOG_ERROR("can not insert a new node in the static field table");
			return NULL;
		}
		/* ok, we need commit the modification on this node first */
		_cesk_static_table_commit(table, node);
	}
	return node->value_set;
}
