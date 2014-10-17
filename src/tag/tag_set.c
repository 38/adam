#include <tag/tag_set.h>
#include <tag/tag_fs.h>
#include <tag/tag_tracker.h>
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
	uint32_t id;   /*!< a unique ID of this tag */
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
	.id = 0,
	.hashcode = HASH_INIT,
};

/**
 * @brief the convertor function
 **/
static tag_set_strreason_callback_t _tag_strreason[TAG_SET_MAX_TAGS];
static tag_set_to_string_callback_t _tag_tostring[TAG_SET_MAX_TAGS];
static uint32_t next_id = 0;
static uint32_t next_set_idx = 1;
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
 * @return the newly allocated memory for the set, NULL indicates error
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
	ret->id = (next_set_idx ++);
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
 * @param set the input tag set
 * @return the result tag set
 **/
static inline tag_set_t* _tag_set_duplicate(tag_set_t* set, int fresh_idx)
{
	if(NULL == set) return NULL;
	tag_set_t* ret = _tag_set_new(set->size);
	if(NULL == ret) return NULL;
	memcpy(ret->data, set->data, sizeof(_tag_set_item_t) * set->size);
	ret->hashcode = set->hashcode;
	ret->size = set->size;
	if(fresh_idx)
	{
		ret->id = next_set_idx ++;
		LOG_DEBUG("TAG_TRACKER: Create tag_set #%u%s from #%u%s", ret->id, tag_set_to_string(ret, NULL, 0), set->id, tag_set_to_string(ret, NULL, 0));
		if(tag_tracker_register_tagset(ret->id, ret, &set->id, 1) < 0)
		{
			LOG_WARNING("failed to track the set");
		}
	}
	else
		ret->id = set->id;
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
	if(NULL == tags)
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
		ret->data[i].resol = resols?resols[i]:0;
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
	LOG_DEBUG("TAG_TRACKER: Tag Set Creation #%u", ret->id);
	if(tag_tracker_register_tagset(ret->id, ret, NULL, 0) < 0)
	{
		LOG_WARNING("can not track the set");
	}
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
static inline const char* _tag_id_to_string(uint32_t id)
{
	if(id >= next_id || NULL == _tag_tostring[id]) return "unknown-tag";
	return _tag_tostring[id](id);
}
const char* tag_set_to_string(const tag_set_t* ts, char* buf, size_t sz)
{
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
	__PR("[");
	int i;
	for(i = 0; i < ts->size; i ++)
		__PR("%s ", _tag_id_to_string(ts->data[i].tid));
	__PR("]");
#undef __PR
	return buf;
}
/**
 * @note notice that the newly created tag set will always has a refcnt of 1 
 **/
tag_set_t* tag_set_merge(const tag_set_t* first, const tag_set_t* second)
{
	if(NULL == first || NULL == second) return NULL;
	if(0 == first->size && 0 == second->size) return tag_set_empty();
	/* try light weight copy if possible */
	if(first == NULL || first->size == 0) 
	{
		tag_set_t* ret = (tag_set_t*)second;
		_tag_set_incref(ret);
		return _tag_set_duplicate(ret, 1);
	}
	if(second == NULL || second->size == 0) 
	{
		tag_set_t* ret = (tag_set_t*)first;
		_tag_set_incref(ret);
		return _tag_set_duplicate(ret, 1);
	}
	if(second == first) 
	{
		tag_set_t* ret = (tag_set_t*)first;
		_tag_set_incref(ret);
		return _tag_set_duplicate(ret, 1);
	}
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
	LOG_DEBUG("TAG_TRACKER: Tag set Creation tag_set #%u from #%u and #%u", ret->id, first->id, second->id);
	uint32_t input[] = {first->id, second->id};
	if(tag_tracker_register_tagset(ret->id, ret, input, 2) < 0)
	{
		LOG_WARNING("can not track the set");
	}
	_tag_set_incref(ret);
	return ret;
}
int tag_set_contains(const tag_set_t* set, uint32_t what)
{
	if(0 == set->size || what < set->data[0].tid || set->data[set->size-1].tid < what) return 0;
	int l = 0, r = set->size;
	while(r - l > 1)
	{
		int m = (l + r) / 2;
		if(set->data[m].tid <= what) l = m;
		else r = m;
	}
	return set->data[l].tid == what;
}
tag_set_t* tag_set_change_resolution(tag_set_t* set, uint32_t tagid, uint32_t value)
{
	tag_set_t* prev_set = NULL;
	/* before we actual modify the set, we should make sure we can modify it safely */
	if(set->refcnt > 1)
	{
		prev_set = set;
		LOG_DEBUG("modifying a tag set using by multiple values, copy it first");
		set = _tag_set_duplicate(set, 0);
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
	while(r - l > 1)
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

void tag_set_register_handler(uint32_t* tagid, tag_set_to_string_callback_t to_string, tag_set_strreason_callback_t strreason)
{
	*tagid = next_id ++;
	_tag_strreason[*tagid] = strreason;
	_tag_tostring[*tagid] = to_string;
}

const char* tag_set_reason_code_to_string(uint32_t why)
{
	uint32_t tid = why >> 16;
	uint32_t code = why & 0xffff;
	if(tid >= next_id) return "(invalid tag)";
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
uint32_t tag_set_id(const tag_set_t* set)
{
	return set->id;
}
