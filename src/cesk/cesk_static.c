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

/** 
 * @brief the segment tree node 
 **/
typedef struct _cesk_static_tree_node_t _cesk_static_tree_node_t;

struct _cesk_static_tree_node_t{
	uint32_t refcnt:31;                   /*!< the reference counter */
	uint8_t  isleaf:1;                    /*!< wether or not this node is a leaf node */
	cesk_set_t* value[0];                 /*!< the value */
	_cesk_static_tree_node_t* child[0];   /*!< the child pointer */
};

/**
 * @breif data structure for the static table
 **/
struct _cesk_static_table_t {
	hashval_t hashcode;                  /*!< current hashcode */
	_cesk_static_tree_node_t* root;      /*!< the root of the tree */
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

/** 
 * @brief how many field do i have? 
 **/
extern const uint32_t dalvik_static_field_count;


/* local inline functions */
/**
 * @brief return the node size for this segment [left, right)
 * @param left the left boundary
 * @param right the right boundary
 * @return the size of the node in bytes
 **/
static inline size_t _cesk_static_tree_node_size(int isleaf)
{
	if(!isleaf) return sizeof(_cesk_static_tree_node_t) + 2 * sizeof(_cesk_static_tree_node_t*);
	else return sizeof(_cesk_static_tree_node_t) + sizeof(cesk_set_t*);
}
/**
 * @brief increase the refcnt of the node
 * @param node the target node
 * @return nothing
 **/
static inline void _cesk_static_tree_node_incref(_cesk_static_tree_node_t* node)
{
	if(NULL != node) node->refcnt ++;
}
/**
 * @brief decrease the refcnt of the node
 * @param node the target node
 * @return nothing
 **/
static inline void _cesk_static_tree_node_decref(_cesk_static_tree_node_t* node)
{
	if(NULL == node) return;
	if(0 == --node->refcnt)
	{
		LOG_DEBUG("node %p is dead, swipe it out", node);
		/* if this is a leaf node, we have to delete the value set */
		if(node->isleaf) 
			cesk_set_free(node->value[0]);
		/* for a non-leaf node, we should derefence it's child */
		else
		{
			_cesk_static_tree_node_decref(node->child[0]);
			_cesk_static_tree_node_decref(node->child[1]);
		}
		free(node);
	}
}
/**
 * @brief create a new segment tree node for range [left, right)
 * @param left the left boundary
 * @param right the right boundary
 * @return the newly create node, NULL indicates error
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_new(uint32_t left, uint32_t right)
{
	int isleaf = (right - left <= 1);
	size_t size = _cesk_static_tree_node_size(isleaf);
	_cesk_static_tree_node_t* ret = (_cesk_static_tree_node_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for a new segment tree node");
		return NULL;
	}
	ret->refcnt = 0;
	ret->isleaf = isleaf;
	return ret;
}
/**
 * @brief make a new copy of a tree node
 * @param node the tree node we want to copy
 * @return the newly created node
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_duplicate(_cesk_static_tree_node_t* node)
{
	int isleaf = node->isleaf;
	size_t size = _cesk_static_tree_node_size(isleaf);
	_cesk_static_tree_node_t* ret = (_cesk_static_tree_node_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the new copy of the node %p", node);
		return NULL;
	}
	ret->refcnt = 0;
	/* for the leaf node, we should duplicate the set */
	if(isleaf)
	{
		ret->value[0] = cesk_set_fork(node->value[0]);
	}
	/* just copy the pointer */
	else
	{
		_cesk_static_tree_node_incref(ret->child[0] = node->child[0]);
		_cesk_static_tree_node_incref(ret->child[1] = node->child[1]);
	}
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
	/* for each iteration, we consider the address range [l, r) */
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
 * @brief make a signle node writable
 * @param reference a pointer to the reference to this node 
 * @param node current node
 * @return a pointer to writable node, NULL indicates an error
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_node_prepare_to_write(_cesk_static_tree_node_t** reference, _cesk_static_tree_node_t*  node)
{
	/* if there's only one reference to this node, just do nothing */
	if(node->refcnt == 1) return node;
	/* make a new copy of current node */
	_cesk_static_tree_node_t* dup = _cesk_static_tree_node_duplicate(node);
	if(NULL == dup)
	{
		LOG_ERROR("can not make a new copy of node %p", node);
		return NULL;
	}
	/* replace the current node with the new copy */
	*reference = dup;
	_cesk_static_tree_node_decref(node);
	_cesk_static_tree_node_incref(dup);
	return dup;
}
/**
 * @brief prepare to write a node in segment tree
 * @param root  the tree root 
 * @param index the field index we are to write
 * @param p_target a pointer to a buffer that can be used to pass the target node to caller
 *       (pass NULL if caller deesn't want know this)
 * @return the new root node after opration, NULL indicate a failure
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_prepare_to_write(
		_cesk_static_tree_node_t* root,
		uint32_t index,
		_cesk_static_tree_node_t** p_target)
{
	/* new root of the tree */ 
	_cesk_static_tree_node_t* ret = root;
	/* the parent of current node */
	_cesk_static_tree_node_t* parent = NULL;
	/* the last moving direction */
	int last_direction = -1;

	uint32_t l = 0, r = dalvik_static_field_count;
	
	for(;r - l > 1 && NULL != root;)
	{
		/* make current node writable */
		if(NULL == (root = _cesk_static_tree_node_prepare_to_write(parent?&ret:&parent->child[last_direction], root)))
		{
			LOG_ERROR("can not make current node (segment = <%u, %u>) writable", l, r);
			return NULL;
		}
		int m = (l + r) / 2;
		parent = root;
		if(index < m) r = m, root = root->child[last_direction = 0];
		else l = m, root = root->child[last_direction = 1];
	}
	/* if we reach the leaf node, we should do the same thing */
	if(root && NULL == (root = _cesk_static_tree_node_prepare_to_write(parent?&ret:&parent->child[last_direction], root)))
	{
		LOG_ERROR("can not make the leaf node for index %d writable", index);
		return NULL;
	}
	if(NULL != p_target) *p_target = root;
	return ret;
}
/**
 * @brief insert a new node to the segment tree
 * @param tree the pointer to the pointer to the root node
 * @param index the index node
 * @param initval the initial value, CESK_STORE_ADDR_NULL means do not actually initialize the vlaue set
 * @param p_target the newly inserted node
 * @note we assume that we do not have field index before the insertion
 * @return the root of the tree after insertion, NULL indicates an error
 **/
static inline _cesk_static_tree_node_t* _cesk_static_tree_insert(
		_cesk_static_tree_node_t* tree, 
		uint32_t index, 
		uint32_t initval,
		_cesk_static_tree_node_t** p_target)
{
	/* before we start, we should make sure the tree is wrtiable at least at this index */
	tree = _cesk_static_tree_prepare_to_write(tree, index, NULL);
	if(NULL == tree)
	{
		LOG_ERROR("failed to make the tree writable");
		return NULL;
	}
	/* ok, let's do it! */
	uint32_t l = 0, r = dalvik_static_field_count;
	_cesk_static_tree_node_t* parent = NULL;
	_cesk_static_tree_node_t* node = tree;
	int last_direction = -1;
	/* first we go down through the tree, until we got a NULL pointer */
	for(;r - l > 1 && NULL != node;)
	{
		int m = (l + r) / 2;
		parent = node;
		if(index < m) r = m, node = node->child[last_direction = 0];
		else l = m, node = node->child[last_direction = 1];
	}
	/* then we create new node until we touched the leaf node */
	do{
		node = _cesk_static_tree_node_new(l, r);
		if(NULL == node)
		{
			LOG_ERROR("can not create a new node for segment [%u, %u)", l ,r);
			return NULL;
		}
		/* if this node is the leaf node, we should initlaize the default value */
		if(node->isleaf)
		{
			if(initval != CESK_STORE_ADDR_NULL)
			{
				node->value[0] = cesk_set_empty_set();
				if(NULL == node->value[0])
				{
					LOG_ERROR("can not initialize the static field %u", index);
					free(node);
					return NULL;
				}
				if(cesk_set_push(node->value[0], initval) < 0)
				{
					LOG_ERROR("can not set the initial value for the static field %u", index);
					free(node->value[0]);
					free(node);
					return NULL;
				}
				LOG_DEBUG("initialize new field %u with value "PRSAddr, index, initval);
			} 
			else node->value[0] = NULL;
			if(NULL != p_target) *p_target = node;
		}
		else node->child[0] = node->child[1] = NULL;
		/* maintain the tree structure */
		if(parent) parent->child[last_direction] = node;
		else tree = node;
		/* keep moving down */
		parent = node;
		int m = (l + r) / 2;
		if(index < m) r = m, last_direction = 0;
		else l = m, last_direction = 1;
	}while(r - l > 1);
	return tree;
}
/**
 * @brief initialize a static field
 * @param table the static field table
 * @param index the field index 
 * @param init  wether or not perform an actual initilaization (since for some cases, the initial vlaue is about to overrided)
 * @return the newly added node in the segment tree
 **/
static inline _cesk_static_tree_node_t*  _cesk_static_table_init_field(cesk_static_table_t* table, uint32_t index, int init)
{
	_cesk_static_tree_node_t* ret = NULL;
	uint32_t init_val = _cesk_static_default_value[index];
	if(CESK_STORE_ADDR_NULL == init_val) 
	{
		LOG_ERROR("invalid initializer, you haven't do field query before doing this?");
		return NULL;
	}
	_cesk_static_tree_node_t* new_tree = _cesk_static_tree_insert(table->root, index, init?init_val:CESK_STORE_ADDR_NULL, &ret);
	if(NULL == new_tree) 
	{
		LOG_ERROR("can not insert %u new node in the segment tree", index);
		return NULL;
	}
	table->root = new_tree;
	return ret;
}
/**
 * @breif update hashcode
 * @param table
 * @param index the field index
 * @param vlaue the value set 
 * @return nothing
 **/
static inline void _cesk_static_table_update_hashcode(cesk_static_table_t* table, uint32_t index, const cesk_set_t* value)
{
    hashval_t node_hash = (index * index * MH_MULTIPLY) ^ cesk_set_hashcode(value);
    table->hashcode ^= node_hash;
}
/**
 * @brief find the smallest index larger than the given number has ever been initlaized in the segment tree
 * @param root the root node of the segment tree to search
 * @param start the start point to do the address search
 * @param p_target the buffer used to pass the target node to caller, NULL indicates caller do not need this node
 * @return the result index, if not found return CESK_STORE_ADDR_NULL 
 **/
static inline uint32_t _cesk_static_tree_next(const _cesk_static_tree_node_t* root, uint32_t start, const _cesk_static_tree_node_t** p_target)
{
    uint32_t l[30] = {0}, r[30] = {dalvik_static_field_count}, level;
    const _cesk_static_tree_node_t* path[30] = {root};
    for(level = 0;r[level] - l[level] > 1 && NULL != path[level]; level ++)
    {
        uint32_t m = (l[level] + r[level]) / 2;
        if(start < m) r[level + 1] = m, path[level + 1] = path[level]->child[0];
        else l[level + 1] = m, path[level + 1] = path[level]->child[1];
    }
    int i;
    for(i = level - 1; i >= 0; i --)
    {
        if(path[i]->child[1] != NULL && path[i]->child[1] != path[i + 1])
        {
            uint32_t left = (l[i] + r[i]) / 2, right = r[i];
            for(root = path[i]->child[1]; !root->isleaf;)
            {
                uint32_t mid = (left + right) / 2;
                if(NULL != root->child[0]) right = mid, root = root->child[0];
                else left = mid, root = root->child[1];
            }
            if(p_target) *p_target = root;
            return left;
        }
    }
    return CESK_STORE_ADDR_NULL;
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
cesk_static_table_t* cesk_static_table_fork(const cesk_static_table_t* table)
{
	cesk_static_table_t* ret = (cesk_static_table_t*)malloc(sizeof(cesk_static_table_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the new static field table");
		return NULL;
	}
	if(NULL == table)
	{
		ret->hashcode = 0x3c457fabu;   /* just a magic number */
		ret->root = NULL;              /* everything reminds uninitlaized */
	}
	else
	{
		ret->hashcode = table->hashcode;
		ret->root = (_cesk_static_tree_node_t*)table->root;
		_cesk_static_tree_node_incref(ret->root);
	}
	return ret;
}
void cesk_static_table_free(cesk_static_table_t* table)
{
	if(NULL == table) return;
	_cesk_static_tree_node_decref(table->root);
	free(table);
}
const cesk_set_t* cesk_static_table_get_ro(const cesk_static_table_t* table, uint32_t addr)
{
	if(NULL == table || !CESK_STORE_ADDR_IS_STATIC(addr))
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	uint32_t idx = CESK_STORE_ADDR_STATIC_IDX(addr);
	if(idx >= dalvik_static_field_count)
	{
		LOG_ERROR("invalid static field address "PRSAddr", out of boundary", idx);
		return NULL;
	}
	const _cesk_static_tree_node_t* node = _cesk_static_tree_node_find(table->root, idx);
	if(NULL == node)
	{
		LOG_DEBUG("static field %u hasn't been initliazed, initialize it now", idx);
		node = _cesk_static_table_init_field((cesk_static_table_t*)table, idx, 1); 
        if(NULL == node)
        {
            LOG_ERROR("can not initialize static field %u", idx);
            return NULL;
        }
        _cesk_static_table_update_hashcode((cesk_static_table_t*)table, idx, node->value[0]);
	}
	return node->value[0];
}
cesk_set_t** cesk_static_table_get_rw(cesk_static_table_t* table, uint32_t addr, int init)
{
    if(NULL == table || !CESK_STORE_ADDR_IS_STATIC(addr))
    {
        LOG_ERROR("invalid argument");
        return NULL;
    }
    uint32_t idx = CESK_STORE_ADDR_STATIC_IDX(addr);
    if(idx >= dalvik_static_field_count)
    {
		LOG_ERROR("invalid static field address "PRSAddr", out of boundary", idx);
		return NULL;
    }
    _cesk_static_tree_node_t* target;
    _cesk_static_tree_node_t* new_root = _cesk_static_tree_prepare_to_write(table->root, idx, &target);
    if(NULL == new_root)
    {
        LOG_ERROR("can not make the tree ready for modification");
        return NULL;
    }
    /* if the node remains uninitialized */
    if(NULL == target)
    {
        LOG_DEBUG("static field %u hasn't been initliazed, initialize it now", idx);
        target = _cesk_static_table_init_field(table, idx, init);
        if(NULL == target)
        {
            LOG_ERROR("can not initialize static field %u", idx);
            return NULL;
        }
    }
    /* otherwise we need to update the hashcode */
    else _cesk_static_table_update_hashcode(table, idx, target->value[0]);
    return target->value;
}
int cesk_static_table_release_rw(cesk_static_table_t* table, uint32_t addr,  const cesk_set_t* value)
{
    if(NULL == table || !CESK_STORE_ADDR_IS_STATIC(addr) || NULL == value)
    {
        LOG_ERROR("invalid argument");
        return -1;
    }
    uint32_t idx = CESK_STORE_ADDR_STATIC_IDX(addr);
    _cesk_static_table_update_hashcode(table, idx, value);
    return 0;
}
cesk_static_table_iter_t* cesk_static_table_iter(const cesk_static_table_t* table, cesk_static_table_iter_t* buf)
{
    if(NULL == buf || NULL == table) return NULL;
    buf->table = table;
    buf->begin = 0;
    return buf;
}
const cesk_set_t* cesk_static_table_iter_next(cesk_static_table_iter_t* iter, uint32_t *paddr)
{
    const _cesk_static_tree_node_t* node;
    if(NULL ==  iter) return NULL;
    *paddr = _cesk_static_tree_next(iter->table->root, iter->begin, &node);
    if(CESK_STORE_ADDR_NULL == *paddr) return NULL;
    iter->begin = *paddr + 1;
    return node->value[0]; 
}
hashval_t cesk_static_table_hashcode(const cesk_static_table_t* table)
{
    return table->hashcode;
}
int cesk_static_table_equal(const cesk_static_table_t* left, const cesk_static_table_t* right)
{
    if(left == NULL || right == NULL) return left == right;
    cesk_static_table_iter_t li_buf, ri_buf;
    if(cesk_static_table_hashcode(left) != cesk_static_table_hashcode(right)) return 0;
    cesk_static_table_iter_t* li = cesk_static_table_iter(left, &li_buf);
    cesk_static_table_iter_t* ri = cesk_static_table_iter(right, &ri_buf);
    if(NULL == li || NULL == ri)
    {
        LOG_ERROR("can not get iterator for the static field table");
        return -1;
    }
    uint32_t laddr, raddr;
	const cesk_set_t *lset, *rset;
    for(lset = cesk_static_table_iter_next(li, &laddr), rset = cesk_static_table_iter_next(ri, &raddr);
        NULL != lset && NULL != rset && laddr == raddr && cesk_set_equal(lset, rset);
        lset = cesk_static_table_iter_next(li, &laddr), rset = cesk_static_table_iter_next(ri, &raddr));
    return NULL == lset && NULL == rset;
}
