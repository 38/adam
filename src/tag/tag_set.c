#include <tag/tag_set.h>
#include <tag/tag_fs.h>
#define HASH_INIT 0x376514fbu
/**
 * @brief the structure of an item in a tag set
 **/
typedef struct {
	uint32_t tid;   /*!< the tag id */
	uint32_t resol; /*!< the resolution of this item */
} _tag_set_item_t;
/**
 * @brief the implementation of the tag set
 **/
struct _tag_set_t {
	uint32_t refcnt;    /*!< the reference counter */
	uint32_t size;      /*!< how many items in the set */
	hashval_t hashcode; /*!< current hashcode */
	_tag_set_item_t data[0];   /*!< the actual data set */
};
CONST_ASSERTION_LAST(tag_set_t, data);
CONST_ASSERTION_SIZE(tag_set_t, data, 0);
/** @brief the only empty in tag system **/
static tag_set_t _tag_set_empty = {
	.refcnt = 0,
	.size = 0,
	.hashcode = HASH_INIT,
}; 
/**
 * @brief compute the hashcode for a signle set item
 * @param item the set item
 * @return the hashcode
 **/
static inline hashval_t _tag_set_item_hashcode(const _tag_set_item_t item)
{
	return item.tid * (MH_MULTIPLY) + (item.resol * item.resol + 0x23fc * item.resol + MH_MULTIPLY * item.resol);
}
/**
 * @brief allocate a new set with N slots
 * @param N the number of slots
 * @param the newly allocated memory for the set, NULL indicates error
 **/
static inline tag_set_t* _tag_set_new(size_t N)
{
	tag_set_t* ret = (tag_set_t*)malloc(sizeof(tag_set_t) + sizeof(_tag_set_item_t) * N);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate a new tag set with %zu slots", N);
		return NULL;
	}
	ret->size = 0;
	ret->refcnt = 0;
	ret->hashcode = HASH_INIT; /* just a magic number */
	return ret;
}
/**
 * @brief increase the reference counter of a set
 * @param set the target set
 * @return nothing
 **/
static inline void _tag_set_incref(tag_set_t* set)
{
	set->refcnt ++;
}
/**
 * @brief decrease the reference counter of a set
 * @param set the target set
 * @return nothing
 **/
static inline void _tag_set_decref(tag_set_t* set)
{
	if(0 == --set->refcnt)
	{
		LOG_DEBUG("tag set at host memory %p is dead, free it", set);
		free(set);
	}
}
/**
 * @brief duplicate the tag set
 * @param the input tag set
 * @return the result tag set
 **/
static inline tag_set_t* _tag_set_duplicate(tag_set_t* set)
{
	if(NULL == set) return NULL;
	tag_set_t* ret = _tag_set_new(set->size);
	if(NULL == ret) return NULL;
	memcpy(ret->data, set->data, sizeof(_tag_set_item_t) * set->size);
	ret->hashcode = set->hashcode;
	ret->size = set->size;
	_tag_set_decref(set);
	_tag_set_incref(ret);
	return ret;
}
int tag_set_init()
{
	_tag_set_incref(&_tag_set_empty);
	
	tag_fs_init();

	return 0;
}
void tag_set_finalize()
{
	/* do nothing just follow the protocol */
}
tag_set_t* tag_set_empty()
{
	_tag_set_incref(&_tag_set_empty);
	return &_tag_set_empty;
}
tag_set_t* tag_set_from_array(const uint32_t* tags, const uint32_t* resols, size_t N)
{
	if(NULL == tags || NULL == resols)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	if(N == 0) return tag_set_empty();
	tag_set_t* ret = _tag_set_new(N);
	if(NULL == ret) return NULL;
	int i;
	for(i = 0; i < N; i ++)
	{
		ret->data[i].tid = tags[i];
		ret->data[i].resol = resols[i];
		ret->hashcode ^= _tag_set_item_hashcode(ret->data[i]);
		if(i > 0 && tags[i - 1] >= tags[i])
		{
			LOG_ERROR("tags array is not strictly increasing");
			free(ret);
			return NULL;
		}
	}
	ret->size = N;
	_tag_set_incref(ret);
	return ret;
}
tag_set_t* tag_set_fork(const tag_set_t* set)
{
	if(NULL == set) return NULL;
	tag_set_t* set_rw = (tag_set_t*)set;  /* really dirty */
	_tag_set_incref(set_rw);
	return set_rw;
}
void tag_set_free(tag_set_t* set)
{
	if(NULL != set) _tag_set_decref(set);
}
hashval_t tag_set_hashcode(const tag_set_t* set)
{
	return set->hashcode;
}
int tag_set_equal(const tag_set_t* first, const tag_set_t* second)
{
	if(NULL == first || NULL == second) return -1;
	if(first->size != second->size || first->hashcode != second->hashcode) return 0;
	uint32_t i;
	for(i = 0; i < first->size; i ++)
	{
		if(first->data[i].tid != second->data[i].tid ||
		   first->data[i].resol != second->data[i].resol) return 0;
	}
	return 1;
}
tag_set_t* tag_set_merge(const tag_set_t* first, const tag_set_t* second)
{
	/* try light weight copy if possible */
	if(first == NULL || first->size == 0) return tag_set_fork(second);
	if(second == NULL || second->size == 0) return tag_set_fork(first); 
	size_t N = first->size + second->size;
	tag_set_t* ret = _tag_set_new(N);
	if(NULL == ret) return NULL;
	/* do a merge sort */
	uint32_t i, j;
	for(i = j = 0; i < first->size || j < second->size; )
	{
		uint32_t current_id = 0xffffffffu;
		if(i < first->size && first->data[i].tid < current_id) 
			current_id = first->data[i].tid;
		if(j < second->size && second->data[j].tid < current_id) 
			current_id = second->data[j].tid;
		uint32_t resol = 0;
		if(i < first->size && current_id == first->data[i].tid)
		{
			resol |= first->data[i].resol;
			i ++;
		}
		if(j < second->size && current_id == second->data[j].tid)
		{
			resol |= second->data[j].resol;
			j ++;
		}
		if(resol == TAG_RES_NOTHING) continue;
		ret->data[ret->size].tid = current_id;
		ret->data[ret->size].resol = resol;
		ret->hashcode ^= _tag_set_item_hashcode(ret->data[ret->size]);
		ret->size ++;
		LOG_DEBUG("append item <%d, %d>", current_id, resol);
	}
	return ret;
}
tag_set_t* tag_set_change_resolution(tag_set_t* set, uint32_t tagid, uint32_t value)
{
	tag_set_t* prev_set = NULL;
	/* before we actual modify the set, we should make sure we can modify it safely */
	if(set->refcnt > 1)
	{
		prev_set = set;
		LOG_DEBUG("modifying a tag set using by multiple values, copy it first");
		set = _tag_set_duplicate(set);
		if(NULL == set)
		{
			LOG_ERROR("can not duplicate the tag set");
			return NULL;
		}
	}
	/* now we can modify it */
	/* perform a binary search on the input set to find the target tagid */
	if(0 == set->size || tagid < set->data[0].tid || tagid > set->data[set->size - 1].tid) goto ERR; 
	int l = 0, r = set->size;
	while(r - l < 1)
	{
		int m = (l + r) / 2;
		if(set->data[m].tid <= tagid) l = m;
		else r = m;
	}
	if(set->data[l].tid != tagid) goto ERR;
	set->hashcode ^= _tag_set_item_hashcode(set->data[l]);
	set->data[l].resol = value;
	set->hashcode ^= _tag_set_item_hashcode(set->data[l]);
	return set;
ERR:
	LOG_ERROR("can not find the target tag id %u, so I can not modify the tag set", tagid);
	if(NULL != prev_set)
	{
		_tag_set_decref(set);
		_tag_set_incref(prev_set);
	}
	return NULL;
}
/**
 * @brief the tag checker list 
 **/
static tag_set_checker_callback_t _tag_checker[TAG_NUM_OF_TAGS];
/**
 * @brief the convertor function
 **/
static tag_set_strreason_callback_t _tag_strreason[TAG_NUM_OF_TAGS];

void tag_set_register_handler(uint32_t tagid, tag_set_checker_callback_t checker, tag_set_strreason_callback_t strreason)
{
	_tag_checker[tagid] = checker;
	_tag_strreason[tagid] = strreason;
}
int tag_set_check_malicious(
		const tag_set_t* set, 
		const char* class, 
		const char* method,
		const dalvik_type_t* const * sig,
		const dalvik_type_t* rtype)
{
	if(NULL == set) return 0;
	if(NULL == class || NULL == method || NULL == sig || NULL == rtype) 
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	uint32_t i;
	for(i = 0; i < set->size; i ++)
	{
		if(NULL == _tag_checker[set->data[i].tid]) continue;
		int rc = _tag_checker[set->data[i].tid](set->data[i].tid, set->data[i].resol, class, method, sig, rtype);
		if(rc < 0)
		{
			LOG_WARNING("the tag checker for tagid %u returns a failure, ignored", set->data[i].tid);
			continue;
		}
		if(rc > 0) return (set->data[i].tid << 16) | rc;
	}
	return 0;
}

const char* tag_set_reason_code_to_string(int why)
{
	uint32_t tid = why >> 16;
	uint32_t code = why & 0xffff;
	if(tid >= TAG_NUM_OF_TAGS) return "(invalid tag)";
	if(NULL == _tag_strreason[tid]) return "(strreason function is not supported for this tag)";
	return _tag_strreason[tid](tid,code);
}
size_t tag_set_size(const tag_set_t* set)
{
	if(NULL == set) return 0;
	return set->size;
}
uint32_t tag_set_get_tagid(const tag_set_t* set, uint32_t k)
{
	if(NULL == set || set->size <= k) return 0xfffffffful;
	return set->data[k].tid;
}
uint32_t tag_set_get_resol(const tag_set_t* set, uint32_t k)
{
	if(NULL == set || set->size <= k) return 0xfffffffful;
	return set->data[k].resol;
}
hashval_t tag_set_compute_hashcode(const tag_set_t* set)
{
	hashval_t ret = HASH_INIT;
	uint32_t i;
	for(i = 0; i < set->size; i ++)
		ret ^= _tag_set_item_hashcode(set->data[i]);
	return ret;
}
