/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @details the layout of diff
 * 		Allocation Section:   <addr, init_value>
 *		Reuse Section     :   <addr, new_bit>
 *		Register Section  :   <regid, value>
 *		Store Section     :   <addr, id>
 *		Deallocate Section:   <addr>
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
	int _index;                         /*!< the index of current element, used for heap manipulation */ 
	int offset[CESK_DIFF_NTYPES + 1];   /*!< the size of each segment */
	_cesk_diff_node_t data[0];          /*!< the data section */
};
CONST_ASSERTION_LAST(cesk_diff_t, data);
CONST_ASSERTION_SIZE(cesk_diff_t, data, 0);

cesk_diff_buffer_t* cesk_diff_buffer_new(uint8_t reverse)
{
	cesk_diff_buffer_t* ret = (cesk_diff_buffer_t*)malloc(sizeof(cesk_diff_buffer_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for this buffer");
		return NULL;
	}
	ret->converted = 0;
	ret->reverse = reverse; 
	ret->buffer = vector_new(sizeof(_cesk_diff_node_t));
	return ret;
}

void cesk_diff_buffer_free(cesk_diff_buffer_t* mem)
{
	if(NULL == mem) return;
	int i, sz;
	sz = vector_size(mem->buffer);
	for(i = mem->converted; i < sz; i ++)
	{
		int ofs = i;
		if(mem->reverse)
			ofs = (sz - 1) - i;
		_cesk_diff_node_t* node;
		node = (_cesk_diff_node_t*)vector_get(mem->buffer, ofs);
		if(NULL == node || NULL == node->value) continue;
		cesk_value_decref(node->value);
	}
	vector_free(mem->buffer);
	free(mem);
}
int cesk_diff_buffer_append(cesk_diff_buffer_t* buffer, int type, uint32_t addr, cesk_value_t* value)
{
	if(NULL == buffer) 
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(CESK_DIFF_STORE != type && CESK_DIFF_REG != type)
	{
		if(NULL != value) LOG_WARNING("invalid value field, suppose to be NULL");
		value = NULL;
	}
	else if(CESK_DIFF_REUSE != type)
	{
		if(NULL == value) LOG_ERROR("invalid value field, suppose to be non-NULL");
		return -1;
	}
	_cesk_diff_node_t node = {
		.type = type,
		.addr = addr,
		.value = value
	};
	if(NULL != value && CESK_DIFF_REUSE != type) cesk_value_incref(value);
	return vector_pushback(buffer->buffer, &node);
}
/**
 * @todo use faster sorting algorihtm
 **/
static int _cesk_diff_buffer_cmp(const void* left, const void* right)
{
	const _cesk_diff_node_t* lnode = (const _cesk_diff_node_t*)left;
	const _cesk_diff_node_t* rnode = (const _cesk_diff_node_t*)right;
	if(lnode->type != rnode->type) return lnode->type - rnode->type;
	return lnode->addr - rnode->addr;
}
cesk_diff_t* cesk_diff_from_buffer(cesk_diff_buffer_t* buffer)
{
	if(NULL == buffer)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	if(0 != buffer->converted)
	{
		LOG_ERROR("this buffer is already convered to the diff type, aborting");
		return NULL;
	}
	cesk_diff_t* ret;
	size_t sz;
	sz = vector_size(buffer->buffer);
	ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t) + sizeof(_cesk_diff_node_t) * sz);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the diff");
		return NULL;
	}
	memset(ret->offset, 0, sizeof(ret->offset));
	ret->_index = 0;
	qsort(buffer->buffer->data, sz, buffer->buffer->size, _cesk_diff_buffer_cmp);
	int prev_type = 0;
	int prev_addr = CESK_STORE_ADDR_NULL;
	int i, diff_size = -1;  /* because of the first dumb buffer, we should skip that */
	int size[CESK_DIFF_NTYPES] = {};
	size[0] = -1;
	for(i = 0; i < sz; i ++)
	{
		_cesk_diff_node_t* node = (_cesk_diff_node_t*)vector_get(buffer->buffer, (buffer->reverse)?(sz - 1 - i):(i));
		if(NULL == node)
		{
			LOG_ERROR("can not fetch element #%d from the diff buffer", i);
			goto ERR;
		}
		if(prev_type != node->type || prev_addr != node->addr)
		{
			LOG_DEBUG("new diff record id = %d, type = %d, addr = @%x, value = %p",
					  diff_size,
					  ret->data[diff_size].type, 
					  ret->data[diff_size].addr,
					  ret->data[diff_size].value);
			/* we are moving to the next, so flush the buffer */
			diff_size ++;
			size[prev_type] ++;
			/* prepare the next buffer */
			ret->data[diff_size].type = node->type;
			ret->data[diff_size].addr = node->addr;
			ret->data[diff_size].value = NULL;
		}
		/* so we must merge the buffer and current node */
		if(ret->data[diff_size].value == NULL)   /* this is a new buffer */
			ret->data[diff_size].value = node->value;
		else    /* this buffer is used, so merge the value in the buffer and node*/
		{
			switch(node->type)
			{
				case CESK_DIFF_ALLOC:
					LOG_WARNING("allocate the same address twice during a signle execution of a block might be a mistake");
					break;
				case CESK_DIFF_DEALLOC:
					LOG_WARNING("deallocate the same address twice during a signle execution of a block might be a mistake");
					break;
				case CESK_DIFF_REUSE:
					ret->data[diff_size].value = node->value;
					break;
				case CESK_DIFF_REG:
				case CESK_DIFF_STORE:
					/* don't forget release the reference count for the old value */
					cesk_value_decref(ret->data[diff_size].value);
					/* ok, let's move on to the next one */
					ret->data[diff_size].value = node->value;
					/* why we do not increase refcount is because we acually transfer the ownership of
					 * this refcount from buffer to the diff. So we do not actually create a reference*/
					break;
				default:
					LOG_ERROR("unknown diff type %d", node->type);
					goto ERR;
			}
		}
		prev_type = node->type;
		prev_addr = node->addr;
		buffer->converted ++;
	}
	/* ok, we need to flush the last item */
	LOG_DEBUG("new diff record id = %d, type = %d, addr = @%x, value = %p",
			  diff_size,
			  ret->data[diff_size].type, 
			  ret->data[diff_size].addr,
			  ret->data[diff_size].value);
	diff_size ++;
	size[prev_type] ++;
	ret->offset[0] = 0;
	for(i = 1; i <= CESK_DIFF_NTYPES; i ++)
		ret->offset[i] = ret->offset[i-1] + size[i-1];
	return ret;
ERR:
	if(NULL != ret) free(ret);
	return NULL;
}
void cesk_diff_free(cesk_diff_t* diff)
{
	if(NULL == diff) return;
	int sec, ofs = 0;
	for(sec = 0; sec < CESK_DIFF_NTYPES; sec ++)
	{
		for(ofs = diff->offset[sec]; ofs < diff->offset[sec + 1]; ofs ++)
		{
			if(NULL != diff->data[ofs].value && diff->data[ofs].type != CESK_DIFF_REUSE)
			{
				/* the diff occupies one reference count, so release it */
				cesk_value_decref(diff->data[ofs].value);
			}
		}
	}
	free(diff);
}
/**
 * @brief allocate a memory for the result
 * @param N how many inputs
 * @param args the arguments
 * @return the newly created memory, NULL indcates error
 **/
static inline cesk_diff_t* _cesk_diff_allocate_result(int N, cesk_diff_t* args[])
{
	size_t size = -1;
	int i;
	for(i = 0; i < N; i ++)
		args[i]->_index = args[i]->offset[0];
	int prev_type = -1;
	uint32_t prev_addr = CESK_STORE_ADDR_NULL;
	for(;;)
	{
		uint32_t cur_addr = CESK_STORE_ADDR_NULL;
		int cur_type = CESK_DIFF_NTYPES;
		int cur_idx = 0;
		for(i = 0; i < N; i ++)
		{
			if(args[i]->_index >= args[i]->offset[CESK_DIFF_NTYPES]) 
				continue;
			if(args[i]->data[args[i]->_index].type < cur_type)
			{
				cur_type = args[i]->data[args[i]->_index].type;
				cur_addr = args[i]->data[args[i]->_index].addr;
				cur_idx = i;
			}
			else if(args[i]->data[args[i]->_index].type == cur_type && 
					args[i]->data[args[i]->_index].addr < cur_addr)
			{
				cur_addr = args[i]->data[args[i]->_index].addr;
				cur_idx = i;
			}
		}
		if(prev_type != cur_type || prev_addr != cur_addr)
			size ++;
		prev_type = cur_type;
		prev_addr = cur_addr;
		args[cur_idx]->_index ++;
	}
	size ++;
	cesk_diff_t* ret = (cesk_diff_t*) malloc(sizeof(cesk_diff_t) + size * sizeof(_cesk_diff_node_t));
	LOG_DEBUG("created a new diff struct with %d slots", size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result diff");
		return NULL;
	}
	memset(ret->offset, 0, sizeof(ret->offset));
	return ret;
}
cesk_diff_t* cesk_diff_apply(int N, cesk_diff_t** args)
{
	int i;
	cesk_diff_t* ret = _cesk_diff_allocate_result(N, args);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the newly created diff");
		return NULL;
	}
	/* init the index */
	for(i = 0; i < N; i ++)
		args[i]->_index = args[i]->offset[0];
	/* ok, let's go */
	int section;
	/* for each section */
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		ret->offset[section + 1] = ret->offset[section];   /* the initial size of this section should be 0 */
		for(;;)
		{
			uint32_t cur_addr = CESK_STORE_ADDR_NULL;
			for(i = 0; i < N; i ++)
				if(args[i]->_index < args[i]->offset[CESK_DIFF_NTYPES] &&
				   args[i]->data[args[i]->_index].addr < cur_addr)
					cur_addr = args[i]->data[args[i]->_index].addr;
			if(CESK_STORE_ADDR_NULL == cur_addr)
				break;
			/* merge it */
			ret->data[ret->offset[section + 1]++].value = NULL;
			for(i = 0; i < N; i ++)
				if(args[i]->_index < args[i]->offset[CESK_DIFF_NTYPES] &&
				   args[i]->data[args[i]->_index].addr == cur_addr)
				{
					_cesk_diff_node_t* node = args[i]->data + args[i]->_index;
					if(ret->data[ret->offset[section + 1] - 1].value == NULL)
					{
						ret->data[ret->offset[section + 1] - 1] = *node;
						cesk_value_incref(node->value);
					}
					else
					{
						switch(node->type)
						{
							case CESK_DIFF_ALLOC:
								LOG_WARNING("allocate the same address twice during a signle execution of a block might be a mistake");
								break;
							case CESK_DIFF_DEALLOC:
								LOG_WARNING("deallocate the same address twice during a signle execution of a block might be a mistake");
								break;
							case CESK_DIFF_REUSE:
								ret->data[ret->offset[section + 1] - 1].value = node->value;
								break;
							case CESK_DIFF_REG:
							case CESK_DIFF_STORE:
								cesk_value_decref(ret->data[ret->offset[section + 1] - 1].value);
								ret->data[ret->offset[section + 1] -1].value = node->value;
								cesk_value_incref(node->value);
								break;
							default:
								LOG_WARNING("unknown diff type");

						}
					}
					args[i]->_index ++;
				}
		}
	}
	/* now we eliminate allocation-deallocation pairs */
	int alloc_begin = ret->offset[CESK_DIFF_ALLOC];
	int alloc_end   = ret->offset[CESK_DIFF_ALLOC + 1];
	int dealloc_begin = ret->offset[CESK_DIFF_DEALLOC];
	int dealloc_end = ret->offset[CESK_DIFF_DEALLOC + 1];
	int alloc_ptr, dealloc_ptr, alloc_free = alloc_end, dealloc_free = dealloc_end;
	int matches = 0;
	for(alloc_ptr = alloc_end - 1, dealloc_ptr = dealloc_end - 1; alloc_ptr >= alloc_begin; alloc_ptr --)
	{
		for(;dealloc_ptr >= dealloc_begin && ret->data[dealloc_ptr].addr > ret->data[alloc_ptr].addr; dealloc_ptr --)
		{
			if(!matches) 
				ret->data[--dealloc_free] = ret->data[dealloc_ptr];
			matches = 0;
		}
		if(ret->data[dealloc_ptr].addr != ret->data[alloc_ptr].addr)
		{
			ret->data[--alloc_free] = ret->data[alloc_ptr];
			matches = 1;
		}
	}
	for(dealloc_ptr = dealloc_free; dealloc_ptr < dealloc_end; dealloc_ptr ++)
		ret->data[dealloc_ptr - dealloc_free + dealloc_begin] = ret->data[dealloc_ptr];
	ret->offset[0] = alloc_free;
	ret->offset[CESK_DIFF_NTYPES] = dealloc_ptr;
	return ret;
}
cesk_diff_t* cesk_diff_factorize(int N, cesk_diff_t** diffs, const cesk_frame_t** current_frame)
{
	cesk_diff_t* ret = _cesk_diff_allocate_result(N, diffs);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result");
		return NULL;
	}

	return ret;
}

