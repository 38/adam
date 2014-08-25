/**
 * @file dalvik_label.c
 * @brief impkementation of label table which map label --> label index and jump table which maps label id --> instruction id
 **/
#include <dalvik/dalvik_label.h>
#include <string.h>
#include <log.h>
#include <debug.h>

#ifdef PARSER_COUNT
int dalvik_label_count = 0;
#endif

/**
 * @brief map label id --> jump table 
 **/
uint32_t dalvik_label_jump_table[DAVLIK_LABEL_POOL_SIZE];

/**
 * @brief the type for label map (label name --> label index)
 **/
typedef struct _dalvik_label_map_t {
	const char* label;
	uint32_t    idx;
	struct _dalvik_label_map_t* next;
} dalvik_label_map_t;

/**
 * @brief the hash table for the label map
 **/
static dalvik_label_map_t* _dalvik_label_map_table[DAVLIK_LABEL_POOL_SIZE];
/**
 * @brief how many label are there in the table
 **/
static int _dalvik_label_count;

int dalvik_label_init(void)
{
	_dalvik_label_count = 0;
	memset(_dalvik_label_map_table, 0, sizeof(_dalvik_label_map_table));
	memset(dalvik_label_jump_table, -1, sizeof(dalvik_label_jump_table)); 
	LOG_DEBUG("Dalvik Label Pool initialized");
	return 0;
}
void dalvik_label_finalize(void)
{
	int i;
	for(i = 0; i < DAVLIK_LABEL_POOL_SIZE; i ++)
	{
		dalvik_label_map_t* ptr;
		for(ptr = _dalvik_label_map_table[i]; ptr; )
		{
			dalvik_label_map_t* this = ptr;
			ptr = ptr->next;
			free(this);
		}
	}
}
int dalvik_label_exists(const char* label)
{
	int idx = ((uintptr_t)label * MH_MULTIPLY)%DAVLIK_LABEL_POOL_SIZE;
	dalvik_label_map_t* ptr;
	for(ptr = _dalvik_label_map_table[idx]; ptr; ptr = ptr->next)
		if(ptr->label == label) return 1;
	return 0;
}
int dalvik_label_get_label_id(const char* label)
{
	int idx = ((uintptr_t)label * MH_MULTIPLY)%DAVLIK_LABEL_POOL_SIZE;
	dalvik_label_map_t* ptr;
	for(ptr = _dalvik_label_map_table[idx]; ptr; ptr = ptr->next)
		if(ptr->label == label) 
		{
			LOG_DEBUG("Find label map %s --> %d", label, ptr->idx);
			return ptr->idx;
		}
	LOG_DEBUG("Creating new mapping for label %s", label);

#ifdef PARSER_COUNT
	dalvik_label_count ++;
#endif
	/* if label not found, create one */

	ptr = (dalvik_label_map_t*)malloc(sizeof(dalvik_label_map_t));
	if(NULL == ptr) 
	{
		LOG_ERROR("can not create mapping %s -> %d", label, _dalvik_label_count);
		return -1;
	}
	if(_dalvik_label_count >= DAVLIK_LABEL_POOL_SIZE) 
	{
		LOG_ERROR("too many labels");
		return -1;
	}
	ptr->label = label;
	ptr->idx   = _dalvik_label_count ++;
	ptr->next  = _dalvik_label_map_table[idx];
	_dalvik_label_map_table[idx] = ptr;
	LOG_DEBUG("Find label map %s --> %d", label, ptr->idx);
	return ptr->idx;
}
