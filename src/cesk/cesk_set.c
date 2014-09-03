/** 
 * @file cesk_set.h
 * @brief implementation of CESK address set
 **/
#include <stdio.h>
#include <string.h>
#include <log.h>
#include <const_assertion.h>
#include <cesk/cesk_set.h>
#include <tag/tag_set.h>
/** @brief invalid set id */
#define CESK_SET_INVALID (~0u)
/* We do not maintain a hash table for each set, because
 * most set contains a few element. 
 * Instead of maintaining a hash table for each set,
 * we maintain a large hash table use <set_idx, address>
 * as key. 
 */
/** @brief the structure holds a address set */
struct _cesk_set_t {
	uint32_t set_idx;    /*!<the set index */
};
/**@brief data that the some set holds */
typedef struct {
	cesk_set_node_t *next;  /*!<next pointer*/
	cesk_set_node_t *prev;
} cesk_set_data_entry_t;
/**@brief the entry that holds metadata of a set */
typedef struct {
	uint32_t size;          /*!<how many element in the set */
	uint32_t refcnt;        /*!<the reference count of this, indicates how many cesk_set_t for this set are returned */
	uint32_t reloc;         /*!<if this set contains relocated address */
	hashval_t hashcode;     /*!<the hash code of the set */
	tag_set_t* tags;          /*!<the value tags */
	cesk_set_node_t* first; /*!<first element of the set */
} cesk_set_info_entry_t;

/**@brief the node in the hash table*/
struct _cesk_set_node_t {
	uint32_t set_idx;       /*!<the set index */
	uint32_t addr;          /*!<the address this data entry refer to */
	/* this pointer is for the hash table */
	cesk_set_node_t *next;  /*!<next element in hash slot */
	cesk_set_node_t *prev;  /*!<previous element in hash slot */
	/* the following space is for the actuall data */
	char data_section[0]; /*!<the data section of this node */
	cesk_set_data_entry_t data_entry[0];   /*!<this is valid for a data entry node */
	cesk_set_info_entry_t info_entry[0];   /*!<this is valid for an info entry node */
};
CONST_ASSERTION_LAST(cesk_set_node_t, data_section);
CONST_ASSERTION_LAST(cesk_set_node_t, data_entry);
CONST_ASSERTION_LAST(cesk_set_node_t, info_entry);
CONST_ASSERTION_SIZE(cesk_set_node_t, data_section, 0);
CONST_ASSERTION_SIZE(cesk_set_node_t, data_entry, 0);
CONST_ASSERTION_SIZE(cesk_set_node_t, info_entry, 0);

static cesk_set_node_t* _cesk_set_hash[CESK_SET_HASH_SIZE];

#define DATA_ENTRY 0
#define INFO_ENTRY 1

#define INFO_ADDR CESK_STORE_ADDR_NULL  /* this is a dumb address to distingush between data_entry and info_entry */
/**
 * @brief this is a debug function which checks the hash structure, but for normal build, just does nothing 
 * @return nothing
 **/
static inline void _cesk_verify_hash_structure()
#if __DEBUG_VERIFY_HASH_STRCUTURE__
{
	int i, j = 0;
	cesk_set_node_t* ptr;
	for(i = 0; i < CESK_SET_HASH_SIZE; i ++)
	{
		for(ptr = _cesk_set_hash[i]; NULL != ptr; ptr = ptr->next)
		{
			if(ptr->prev != NULL)
			{
				if(ptr->prev->next != ptr) 
				{
					goto ERR;
				}
			}
			else
				if(_cesk_set_hash[i] != ptr)
				{
					j = 1;
					goto ERR;
				}
			if(ptr->next != NULL)
			{
				if(ptr->next->prev != ptr)
				{
					j = 2;
					goto ERR;
				}
			}
		}
	}
	return;
ERR:
	LOG_ERROR("set hash table corruption with corruption reason = %d", j);
}
#else
{}
#endif
/**
 * @brief allocate a new set node
 * @param type what is the type of this node data/meta_data
 * @return the newly created node 
 **/
static inline cesk_set_node_t* _cesk_set_node_alloc(int type)
{
	size_t size = sizeof(cesk_set_node_t);
	switch(type)
	{
		case DATA_ENTRY:
			size += sizeof(cesk_set_data_entry_t);
			break;
		case INFO_ENTRY:
			size += sizeof(cesk_set_info_entry_t);
			break;
		default:
			LOG_ERROR("unknown type %d", type);
			return NULL;
	}
	cesk_set_node_t* ret = (cesk_set_node_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for node");
		return NULL;
	}
	memset(ret, 0, size);
	return ret;
}
/**
 * @brief the hash function used in the global hash table for set
 * @prama hashidx the index of the set
 * @param addr the actual data
 * @return result
 **/
static inline uint32_t _cesk_set_idx_hashcode(uint32_t hashidx, uint32_t addr)
{
	return (hashidx * MH_MULTIPLY) ^ ((addr & 0xffff) * MH_MULTIPLY) ^ (addr >> 16);
}
/**
 * @brief the function will insert a node in the hash table regardless if it's duplicated
 * The return value of the function is the header address of data section
 * @param setidx the index of the set to insert
 * @param addr data
 * @return the pointer to the data section of the newly created node
 **/
static inline void* _cesk_set_hash_insert(uint32_t setidx, uint32_t addr)
{
	_cesk_verify_hash_structure();
	
	/* if addr == CESK_STORE_ADDR_NULL, the node is a info node */
	uint32_t h = _cesk_set_idx_hashcode(setidx, addr) % CESK_SET_HASH_SIZE;
	int type = DATA_ENTRY;
	if(addr == CESK_STORE_ADDR_NULL) type = INFO_ENTRY;
	cesk_set_node_t* ret = _cesk_set_node_alloc(type);
	if(NULL == ret) return NULL;
	ret->prev = NULL;
	ret->next = _cesk_set_hash[h];
	ret->set_idx = setidx;
	ret->addr = addr;
	if(_cesk_set_hash[h]) _cesk_set_hash[h]->prev = ret;
	_cesk_set_hash[h] = ret;
	

	_cesk_verify_hash_structure();

	return ret->data_section;
}
/** 
 * @brief look for a value in the hash table, return the address of data section. In other words check if an address belongs to a set
 * @param setidx the set index
 * @param addr the data
 * @return the data section of the target node, if not found, return NULL
 **/
static inline void* _cesk_set_hash_find(uint32_t setidx, uint32_t addr)
{
	uint32_t h = _cesk_set_idx_hashcode(setidx, addr) % CESK_SET_HASH_SIZE;
	cesk_set_node_t *p;
	for(p = _cesk_set_hash[h]; p != NULL; p = p->next)
	{
		if(p->set_idx == setidx &&
		   p->addr    == addr)
		 {
			 return p->data_section;
		 }
	}
	LOG_TRACE("can not find the set hash entry (%d, "PRSAddr")", setidx, addr);
	return NULL;
}
/** 
 * @brief allocate a fresh set index, and append the info entry to hash table 
 * @param p_enbtry pointer to a buffer used to return a reference to the newly created entry node
 * @return the fresh index for the new set, CESK_SET_INVALID if failed
 **/
static inline uint32_t _cesk_set_idx_alloc(cesk_set_info_entry_t** p_entry)
{
	static uint32_t next_idx = 0;
	cesk_set_info_entry_t* entry = (cesk_set_info_entry_t*)_cesk_set_hash_insert(next_idx, CESK_STORE_ADDR_NULL);
	if(NULL == entry)
	{
		LOG_ERROR("can not insert the set info to hash table");
		return CESK_SET_INVALID;
	}
	entry->size = 0;
	entry->refcnt = 0;
	entry->first = NULL;
	entry->tags = tag_set_empty();
	if(NULL == entry->tags)
	{
		LOG_WARNING("can not create an empty tag set for this");
		return 0;
	}
	entry->hashcode = CESK_SET_EMPTY_HASH ^ tag_set_hashcode(entry->tags);  /* any magic number */
	*(p_entry) = entry;
	return next_idx ++;
}
/**
 * @brief the singleton of empty set
 **/
static cesk_set_t* _cesk_empty_set;   /* this is the only empty set in the table */
/**
 * @brief the metadata of the empty set 
 **/
static cesk_set_info_entry_t* _cesk_empty_set_metadata;
int cesk_set_init()
{
	/* make the constant empty set */
	_cesk_empty_set = (cesk_set_t*)malloc(sizeof(cesk_set_t));
	if(NULL == _cesk_empty_set)
	{
		LOG_ERROR("can not allocate memory for the constant set {}");
		return -1;
	}
	_cesk_empty_set->set_idx = _cesk_set_idx_alloc(&_cesk_empty_set_metadata);
	if(NULL == _cesk_empty_set_metadata)
	{
		LOG_ERROR("invalid hash info entry");
		return -1;
	}
	_cesk_empty_set_metadata->refcnt ++;
	return 0;
}
void cesk_set_finalize()
{
	/* free all memory in the hash table */
	int i;
	for(i = 0; i < CESK_SET_HASH_SIZE; i ++)
	{
		cesk_set_node_t *p;
		for(p = _cesk_set_hash[i]; NULL != p; )
		{
			cesk_set_node_t *old = p;
			p = p->next;
			if(CESK_STORE_ADDR_NULL == old->addr) 
				tag_set_free(old->info_entry->tags);
			free(old);
		}
	}
	free(_cesk_empty_set);
}
/* fork a set */
cesk_set_t* cesk_set_fork(const cesk_set_t* sour)
{
	if(NULL == sour) return NULL;
	/* verify if the set exists */
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*) _cesk_set_hash_find(sour->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info)
	{
		LOG_ERROR("the set does not exist");
		return NULL;
	}
	cesk_set_t* ret = (cesk_set_t*)malloc(sizeof(cesk_set_t));
	if(NULL == ret) return NULL;
	/* set the set index */
	ret->set_idx = sour->set_idx;
	/* update the reference counter */
	info->refcnt ++;
	return ret;
}
/* make an empty set, it actually a fork, but we can simplify the operation because we are forking a singleton */
cesk_set_t* cesk_set_empty_set()
{
	cesk_set_t* ret = (cesk_set_t*)malloc(sizeof(cesk_set_t));
	if(NULL == ret) return NULL;

	ret->set_idx = _cesk_empty_set->set_idx;

	_cesk_empty_set_metadata->refcnt ++;

	return ret;
}

size_t cesk_set_size(const cesk_set_t* set)
{
	if(NULL == set) return 0;
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info) return 0;
	return info->size;
}
void cesk_set_free(cesk_set_t* set)
{
	_cesk_verify_hash_structure();
	
	if(NULL == set) return;
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info) return;
	/* the reference counter is reduced to zero, nobody is using this */
	if(0 == --info->refcnt)
	{
		/* get the info node of this */
		cesk_set_node_t* info_node = (cesk_set_node_t*)(((char*)info) - sizeof(cesk_set_node_t));
		cesk_set_node_t* data_node;
		/* traverse all members of this set */
		for(data_node = info->first; data_node != NULL;)
		{
			/* delete it from hash chain */
			if(NULL != data_node->prev) 
				data_node->prev->next = data_node->next;
			else 
			{
				/* first element of the slot */
				uint32_t h = _cesk_set_idx_hashcode(data_node->set_idx, data_node->addr) % CESK_SET_HASH_SIZE;
				_cesk_set_hash[h] = data_node->next;
			}
			if(NULL != data_node->next) 
				data_node->next->prev = data_node->prev;
			cesk_set_node_t* tmp = data_node;
			data_node = data_node->data_entry->next;
			free(tmp);
		}
		/* maintain the pointer used in the hash table */
		if(NULL != info_node->prev)
			info_node->prev->next = info_node->next;
		else
		{
			uint32_t h = _cesk_set_idx_hashcode(info_node->set_idx, CESK_STORE_ADDR_NULL) % CESK_SET_HASH_SIZE;
			_cesk_set_hash[h] = info_node->next;
		}
		if(NULL != info_node->next)
			info_node->next->prev = info_node->prev;
		tag_set_free(info_node->info_entry->tags);
		free(info_node);
	}
	free(set);
	
	_cesk_verify_hash_structure();
	
	return;
}
/**
 * @brief initialize a new set iterator from set info node
 * @param info the info node
 * @param buf the iter buffer
 * @return the result iterator
 **/
static inline cesk_set_iter_t* _cesk_set_iter_from_info_node(cesk_set_info_entry_t* info, cesk_set_iter_t* buf)
{
	buf->next = info->first;
	return buf;
}
cesk_set_iter_t* cesk_set_iter(const cesk_set_t* set, cesk_set_iter_t* buf)
{
	if(NULL == set || NULL == buf) 
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	return _cesk_set_iter_from_info_node(info, buf);
}

uint32_t cesk_set_iter_next(cesk_set_iter_t* iter)
{
	if(NULL == iter) return CESK_STORE_ADDR_NULL;
	if(NULL == iter->next) return CESK_STORE_ADDR_NULL;
	uint32_t ret = iter->next->addr;
	//iter->next = iter->next->next;
	iter->next = iter->next->data_entry->next;
	return ret;
}
/* for performance reason, we also maintain reference counter here
 * Because we assume the caller always returns a new set rather than 
 * the old one, if this function is called
 */
static inline uint32_t _cesk_set_duplicate(cesk_set_info_entry_t* info, cesk_set_info_entry_t** new)
{
	if(NULL == info)
	{
		LOG_ERROR("invalid arguments");
		return CESK_SET_INVALID;
	}
	
	cesk_set_info_entry_t* new_info = NULL;
	uint32_t new_idx = _cesk_set_idx_alloc(&new_info);
	if(NULL == new_info) 
	{
		LOG_ERROR("can not create a new set");
		return CESK_SET_INVALID;
	}
	new_info->size = info->size;
	new_info->hashcode = info->hashcode;
	new_info->reloc = info->reloc;
	new_info->tags = tag_set_fork(info->tags);

	new_info->refcnt = 1;
	info->refcnt --;

	new_info->first = NULL;

	/* duplicate the elements */
	cesk_set_node_t* ptr;
	for(ptr = info->first; NULL != ptr; ptr = ptr->data_entry->next)
	{
		uint32_t addr = ptr->addr;
		/* 
		 * we do not need to check if the element is in the set, because the set is 
		 * newly created and must be empty 
		 */
		cesk_set_node_t* node = (cesk_set_node_t*)(((char*)_cesk_set_hash_insert(new_idx, addr)) - sizeof(cesk_set_node_t));
		node->data_entry->next = new_info->first;
		if(NULL != new_info->first) new_info->first->data_entry->prev = node;
		new_info->first = node;
	}

	if(NULL != new) (*new) = new_info;

	return new_idx;
}
/**
 * @brief make the address set ready to be written
 * @param dest the target set
 * @return the info entry of the written-prepared set, NULL indicates errors
 **/
static inline cesk_set_info_entry_t* _cesk_set_prepare_to_write(cesk_set_t* dest)
{
	cesk_set_info_entry_t *info;
	info = (cesk_set_info_entry_t*)_cesk_set_hash_find(dest->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info)
	{
		LOG_ERROR("can not find set #%d", dest->set_idx);
		return NULL;
	}
	if(info->refcnt > 1)
	{
		LOG_DEBUG("set #%d is refered by %d set objects, duplicate before writing", 
				   dest->set_idx,
				   info->refcnt);
		cesk_set_info_entry_t *new;
		uint32_t idx = _cesk_set_duplicate(info, &new);
		if(CESK_SET_INVALID == idx)
		{
			LOG_ERROR("error during duplicate the set");
			return NULL;
		}
		dest->set_idx = idx;
		info = new;
	}
	return info;
}
int cesk_set_modify(cesk_set_t* dest, uint32_t from, uint32_t to)
{
	_cesk_verify_hash_structure();
	
	if(from == to) return 0;
	if(NULL == dest || CESK_STORE_ADDR_NULL == from || CESK_STORE_ADDR_NULL == to)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(CESK_SET_INVALID == dest->set_idx)
	{
		LOG_ERROR("invalid set object");
		return -1;
	}
	cesk_set_info_entry_t *info;
	info = (cesk_set_info_entry_t*)_cesk_set_hash_find(dest->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info)
	{
		LOG_ERROR("can not find info entry for set #%d", dest->set_idx);
		return -1;
	}
	/* check if the content of the set is used by more than one set. If yes, duplicate it before merge */
	if(info->refcnt > 1)
	{
		LOG_DEBUG("set #%d is used by %d set objects, duplicate before writing",
				  dest->set_idx,
				  info->refcnt);
		cesk_set_info_entry_t *new = NULL;
		uint32_t idx = _cesk_set_duplicate(info, &new);
		dest->set_idx = idx;
		info = new;
	}
	cesk_set_data_entry_t* data = (cesk_set_data_entry_t*)_cesk_set_hash_find(dest->set_idx, from);
	if(NULL == data)
	{
		LOG_WARNING("can not find element @0x%x in set #%d, nothing to modify", from, dest->set_idx);
		return 0;
	}
	cesk_set_node_t* this = (cesk_set_node_t*)(((char*)data) - sizeof(cesk_set_node_t));  
	/* remove the node from the slot list */
	if(this->prev == NULL)
	{
		/* if it's the first element in the slot */
		int h = _cesk_set_idx_hashcode(dest->set_idx, from) % CESK_SET_HASH_SIZE;
		_cesk_set_hash[h] = this->next;
	}
	else
		this->prev->next = this->next;
	if(this->next != NULL)
		this->next->prev = this->prev;
	if(_cesk_set_hash_find(dest->set_idx, to))
	{
		/* if the destination element is duplicated, just delete this node */
		if(this->data_entry->prev)
			this->data_entry->prev->data_entry->next = this->data_entry->next;
		else
			info->first = this->data_entry->next;
		if(this->data_entry->next)
			this->data_entry->next->data_entry->prev = this->data_entry->prev;
		free(this);
		info->hashcode ^= (from * MH_MULTIPLY);
	}
	else
	{
		/* if the destination element is not in the set, move it to a new position */
		this->addr = to;
		int h = _cesk_set_idx_hashcode(dest->set_idx, to) % CESK_SET_HASH_SIZE;
		this->next = _cesk_set_hash[h];
		if(NULL != _cesk_set_hash[h]) _cesk_set_hash[h]->prev = this;
		_cesk_set_hash[h] = this;
		this->prev = NULL;
		info->hashcode ^= (from * MH_MULTIPLY) ^ (to * MH_MULTIPLY);   /* update the hash code */
	}
	if(CESK_STORE_ADDR_IS_RELOC(from) ^ CESK_STORE_ADDR_IS_RELOC(to))
	{
		if(CESK_STORE_ADDR_IS_RELOC(from))
			info->reloc --;
		else 
			info->reloc ++;
	}

	_cesk_verify_hash_structure();


	return 0;
}
int cesk_set_push(cesk_set_t* dest, uint32_t addr)
{
	if(NULL == dest || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(CESK_SET_INVALID == dest->set_idx)
	{
		LOG_ERROR("invalid set object");
		return -1;
	}
	cesk_set_info_entry_t *info;
	if(NULL == (info = _cesk_set_prepare_to_write(dest)))
	{
		LOG_ERROR("can not make the set #%d ready to be written", dest->set_idx);
		return -1;
	}
	/* here we need to check the duplicate element, because 
	 * the set is not empty and might contains the same element
	 * already
	 */
	if(NULL == _cesk_set_hash_find(dest->set_idx, addr))
	{
		/* if this is not a duplicate */
		cesk_set_node_t* node = (cesk_set_node_t*)(((char*)_cesk_set_hash_insert(dest->set_idx, addr)) - sizeof(cesk_set_node_t));
		node->data_entry->next = info->first;
		info->first = node;
		info->hashcode ^= addr * MH_MULTIPLY;   /* update the hash code */
		info->size ++;
		if(CESK_STORE_ADDR_IS_RELOC(addr)) info->reloc ++;
	}
	return 0;
}
/**
 * @brief prepare for merging two sets 
 **/
static inline int _cesk_set_prepare_merge(
		cesk_set_t* dest, 
		const cesk_set_t* sour, 
		cesk_set_info_entry_t** p_info_dst,
		cesk_set_info_entry_t** p_info_src)
{

	if(NULL == sour || NULL == dest) return -1;
	if(CESK_SET_INVALID == dest->set_idx ||
	   CESK_SET_INVALID == sour->set_idx)
		return -1;
	*p_info_dst = (cesk_set_info_entry_t*)_cesk_set_hash_find(dest->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == *p_info_dst) 
	{
		LOG_ERROR("can not find set #%d", dest->set_idx);
		return -1;
	}
	*p_info_src = (cesk_set_info_entry_t*)_cesk_set_hash_find(sour->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == *p_info_src)
	{
		LOG_ERROR("can not find set #%d", sour->set_idx);
		return -1;
	}
	if((*p_info_dst)->refcnt > 1)
	{
		LOG_DEBUG("set #%d has %d references, duplicate it before merging", dest->set_idx, (*p_info_dst)->refcnt);
		cesk_set_info_entry_t *new;
		uint32_t idx = _cesk_set_duplicate(*p_info_dst, &new);
		if(CESK_SET_INVALID == idx)
		{
			LOG_ERROR("error during duplicate the set");
			return -1;
		}
		dest->set_idx = idx;
		*p_info_dst = new;
	}
	return 0;
}
int cesk_set_merge_tags(cesk_set_t* dest, const cesk_set_t* sour)
{
	cesk_set_info_entry_t* info_src = NULL;
	cesk_set_info_entry_t* info_dst = NULL;
	if(_cesk_set_prepare_merge(dest, sour, &info_dst, &info_src) < 0)
	{
		LOG_ERROR("failed to prepare for merging");
		return -1;
	}
	info_dst->hashcode ^= tag_set_hashcode(info_dst->tags);
	tag_set_t* new_tags = tag_set_merge(info_dst->tags, info_src->tags);
	if(NULL == new_tags)
	{
		LOG_ERROR("failed to merge tags");
		return -1;
	}
	tag_set_free(info_dst->tags);
	info_dst->tags = new_tags;
	info_dst->hashcode ^= tag_set_hashcode(info_dst->tags);
	return 0;
}
const tag_set_t* cesk_set_get_tags(const cesk_set_t* set)
{
	if(NULL == set)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_set_info_entry_t* info = _cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info)
	{
		LOG_ERROR("can not find the info node for set #%d", set->set_idx);
		return NULL;
	}
	return info->tags;
}
int cesk_set_assign_tags(cesk_set_t* set, tag_set_t* tags)
{
	if(NULL == set || NULL == tags)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	cesk_set_info_entry_t* info = _cesk_set_prepare_to_write(set);
	if(NULL == info)
	{
		LOG_ERROR("can not make the set #%d ready to be written", set->set_idx);
		return -1;
	}
	if(NULL != info->tags)
		tag_set_free(info->tags);
	info->tags = tags;
	return 0;
}
int cesk_set_merge(cesk_set_t* dest, const cesk_set_t* sour)
{
	cesk_set_info_entry_t* info_src = NULL;
	cesk_set_info_entry_t* info_dst = NULL;
	if(_cesk_set_prepare_merge(dest, sour, &info_dst, &info_src) < 0)
	{
		LOG_ERROR("failed to prepare for merging");
		return -1;
	}
	cesk_set_node_t* ptr;
	for(ptr = info_src->first; NULL != ptr; ptr = ptr->data_entry->next)
	{
		uint32_t addr = ptr->addr;
		if(NULL == _cesk_set_hash_find(dest->set_idx, addr))
		{
			/* if this is not a duplicate */
			cesk_set_node_t* node = (cesk_set_node_t*)(((char*)_cesk_set_hash_insert(dest->set_idx, addr)) - sizeof(cesk_set_node_t));
			node->data_entry->next = info_dst->first;
			info_dst->first = node;
			info_dst->hashcode ^= addr * MH_MULTIPLY;
			info_dst->size ++;
			if(CESK_STORE_ADDR_IS_RELOC(addr)) info_dst->reloc ++;
		}
	}
	info_dst->hashcode ^= tag_set_hashcode(info_dst->tags);
	tag_set_t* new_tags = tag_set_merge(info_src->tags, info_dst->tags);
	if(NULL == new_tags)
	{
		LOG_ERROR("failed to merge tags");
		return -1;
	}
	tag_set_free(info_dst->tags);
	info_dst->tags = new_tags;
	info_dst->hashcode ^= tag_set_hashcode(info_dst->tags);
	return 0;
}
int cesk_set_contain(const cesk_set_t* set, uint32_t addr)
{
	if(NULL == set) return 0;
	if(addr == CESK_STORE_ADDR_NULL) return 0;
	return (NULL != _cesk_set_hash_find(set->set_idx, addr));
}
int cesk_set_equal(const cesk_set_t* first, const cesk_set_t* second)
{
	if(NULL == first || NULL == second) return first == second;
	if(cesk_set_hashcode(first) != cesk_set_hashcode(second)) return 0;
	if(cesk_set_size(first) != cesk_set_size(second)) return 0;
	cesk_set_iter_t iter_buf;
	cesk_set_iter_t *iter;
	cesk_set_info_entry_t* info_fst = (cesk_set_info_entry_t*)_cesk_set_hash_find(first->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info_fst)
	{
		LOG_ERROR("set #%d not found", first->set_idx);
		return -1;
	}
	cesk_set_info_entry_t* info_snd = (cesk_set_info_entry_t*)_cesk_set_hash_find(second->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info_snd)
	{
		LOG_ERROR("set #%d not found", second->set_idx);
		return -1;
	}
	int rc = tag_set_equal(info_fst->tags, info_snd->tags);
	if(rc < 0)
	{
		LOG_ERROR("can not compare the tag set of each set");
		return -1;
	}
	if(rc == 0) return 0;
	uint32_t addr;
	for(iter = _cesk_set_iter_from_info_node(info_fst, &iter_buf); iter != NULL && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(iter));)
		if(cesk_set_contain(second, addr) == 0) return 0;
	return 1;
}
hashval_t cesk_set_hashcode(const cesk_set_t* set)
{
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info) 
		return 0;
	return info->hashcode;
}
hashval_t cesk_set_compute_hashcode(const cesk_set_t* set)
{
	if(NULL == set) return 0;
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info) return 0;
	cesk_set_iter_t iter;
	if(NULL == _cesk_set_iter_from_info_node(info, &iter))
	{
		LOG_ERROR("can not acquire iterator for set %d", set->set_idx);
		return 0;
	}
	uint32_t ret = CESK_SET_EMPTY_HASH;
	uint32_t this;
	while(CESK_STORE_ADDR_NULL != (this = cesk_set_iter_next(&iter)))
	{
		ret ^= (this * MH_MULTIPLY);
	}
	ret ^= tag_set_hashcode(info->tags); 
	return ret;
}
uint32_t cesk_set_get_reloc(const cesk_set_t* set)
{
	cesk_set_info_entry_t* info = (cesk_set_info_entry_t*)_cesk_set_hash_find(set->set_idx, CESK_STORE_ADDR_NULL);
	if(NULL == info) return 0;
	return info->reloc;
}
const char* cesk_set_to_string(const cesk_set_t* set, char* buf, int sz)
{
	if(NULL == set)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	static char _buf[1024];
	if(NULL == buf) 
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
#define __PR(fmt, args...) do{\
	int pret = snprintf(p, buf + sz - p, fmt, ##args);\
	if(pret > buf + sz - p) pret = buf + sz - p;\
	p += pret;\
}while(0)
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not acquire interator for the set %d", set->set_idx);
		return NULL;
	}
	uint32_t this;
	int first = 1;
	while(CESK_STORE_ADDR_NULL != (this = cesk_set_iter_next(&iter)))
	{
		if(first)
			__PR("{"PRSAddr"", this);
		else
			__PR(","PRSAddr"", this);
		first = 0;
	}
	if(first) __PR("{");
	__PR("}");
#undef __PR
	return buf;
}
