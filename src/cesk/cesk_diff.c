/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @todo review the code, and the idea of diff
 **/
#include <const_assertion.h>
#include <vector.h>

#include <cesk/cesk_diff.h>
#include <cesk/cesk_value.h>
/* previous defs */
typedef struct _cesk_diff_node_t _cesk_diff_node_t;
/**
 * @brief a node in the diff segment
 **/
struct _cesk_diff_node_t{
	int type;           /*!< which type this diff item is, useful when sorting */
	int addr;           /*!< the address to operate, in the Register Segment, this means the register number */
	cesk_value_t* value;/*!< the value of this node */
};
/**
 * @brief the diff package
 * @note the diff items is sorted so that we can merge it by a signle scan
 **/
struct _cesk_diff_t{
	int size[CESK_DIFF_NTYPES];   /*!< the size of each segment */
	_cesk_diff_node_t data[0]; /*!< the data section */
};
CONST_ASSERTION_LAST(cesk_diff_t, data);
CONST_ASSERTION_SIZE(cesk_diff_t, data, 0);

cesk_diff_buffer_t* cesk_diff_buffer_new()
{
	cesk_diff_buffer_t* ret = (cesk_diff_buffer_t*)malloc(sizeof(cesk_diff_buffer_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for this buffer");
		return NULL;
	}
	ret->converted = 1;
	ret->buffer = vector_new(sizeof(_cesk_diff_node_t));
	return ret;
}

void cesk_diff_buffer_free(cesk_diff_buffer_t* mem)
{
	if(NULL == mem) return;
	if(!mem->converted)
	{
		int i, sz;
		sz = vector_size(mem->buffer);
		LOG_DEBUG("this buffer is dead before it has been converted, so return the reference count");
		for(i = 0; i < sz; i ++)
		{
			_cesk_diff_node_t* node;
			node = (_cesk_diff_node_t*)vector_get(mem->buffer, i);
			if(NULL == node || NULL == node->value) continue;
			cesk_value_decref(node->value);
		}
	}
}
int cesk_diff_buffer_add(cesk_diff_buffer_t* buffer, int type, uint32_t addr, cesk_value_t* value)
{
	if(NULL == buffer) 
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	_cesk_diff_node_t node = {
		.type = type,
		.addr = addr,
		.value = value
	};
	return vector_pushback(buffer->buffer, &node);
}
cesk_diff_t* cesk_diff_from_buffer(cesk_diff_buffer_t* buffer)
{
	/* TODO */
	return NULL;
}
