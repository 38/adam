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

/** @brief the segment tree node */
typedef struct _cesk_static_tree_node_t _cesk_static_tree_node_t;

struct _cesk_static_tree_node_t{
	uint32_t refcnt;                      /*!< the reference counter */
	cesk_set_t* value[0];                 /*!< the value */
	_cesk_static_tree_node_t* child[0];   /*!< the child pointer */
};

struct _cesk_static_table_t {
	hashval_t hashcode;
	_cesk_static_tree_node_t* root;
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
 * @brief return the node size for this segment [left, right)
 * @param left the left boundary
 * @param right the right boundary
 * @return the size of the node in bytes
 **/
static inline size_t _cesk_static_tree_node_size(uint32_t left, uint32_t right)
{
	if(right - left > 1) return sizeof(_cesk_static_tree_node_t) + 2 * sizeof(_cesk_static_tree_node_t*);
	else return sizeof(_cesk_static_tree_node_t) + sizeof(cesk_set_t*);
}
/**
 * @brief create a new segment tree node for range [left, right)
 * @param left the left boundary
 * @param right the right boundary
 * @return the newly create node, NULL indicates error
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_new(uint32_t left, uint32_t right)
{
	_cesk_static_tree_node_t* ret = (_cesk_static_tree_node_t*)malloc(_cesk_static_tree_node_size(left, right));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for a new segment tree node");
		return NULL;
	}
	ret->left = left;
	ret->right = right;
	ret->refcnt = 0;
	return ret;
}
/**
 * @brief make a new copy of a tree node
 * @param node the tree node we want to copy
 * @return the newly created node
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_duplicate(_cesk_static_tree_node_t* node)
{
	size_t size = (_cesk_static_tree_node_size(node->left, node->right));
	_cesk_static_tree_node_t* ret = (_cesk_static_tree_node_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the new copy of the node [%d,%d)", node->left, node->right);
		return NULL;
	}
	node->refcnt --;
	ret->refcnt = 1;
	return ret;
}
/** 
 * @brief find a index in the segment tree
 * @param root the root of the tree
 * @param index the index
 * @return the tree node for the given index, NULL indicates not found
 **/
static inline const _cesk_static_tree_node_t* _cesk_static_tree_node_find(const _cesk_static_tree_node_t* root, uint32_t index)
{
	int l = 0, r = dalvik_static_field_count;
	for(;r - l > 1 && root;)
	{
		int m = (l + r) / 2;
		if(index < m) r = m, root = root->child[0];
		else l = m, root = root->child[1];
	}
	return root;
}
/**
 * @brief insert a new node to the segment tree
 * @param tree the pointer to the pointer to the root node
 * @param index the index node
 * @param initval the initial value
 * @note we assume that we do not have field index before the insertion
 * @return the node inserted to the tree, NULL indicate an error
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_insert(_cesk_static_tree_node_t** tree, uint32_t index, uint32_t initval)
{
	_cesk_static_tree_node_t* root = *tree;
	int l = 0, r = dalvik_static_field_count;
	for(;r - l > 1;)
	{
		if(root->refcnt > 1) 
		{
			_cesk_static_tree_node_t* dup = _cesk_static_tree_node_duplicate(root);
			if(NULL == dup) 
			{
				LOG_ERROR("can not make a duplication for this node");
				return NULL;
			}
			//TODO the seg tree
		}
		int m = (l + r) / 2;

	}
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
