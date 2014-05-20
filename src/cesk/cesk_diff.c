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
	int _index;                         /*!< the index of current element, used for heap manipulation */ 
	int offset[CESK_DIFF_NTYPES + 1];   /*!< the size of each segment */
	_cesk_diff_node_t data[0];          /*!< the data section */
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
	ret->converted = 0;
	ret->buffer = vector_new(sizeof(_cesk_diff_node_t));
	return ret;
}

void cesk_diff_buffer_free(cesk_diff_buffer_t* mem)
{
	if(NULL == mem) return;
	int i, sz;
	sz = vector_size(mem->buffer);
	LOG_DEBUG("this buffer is dead before it has been converted, so return the reference count");
	for(i = mem->converted; i < sz; i ++)
	{
		_cesk_diff_node_t* node;
		node = (_cesk_diff_node_t*)vector_get(mem->buffer, i);
		if(NULL == node || NULL == node->value) continue;
		cesk_value_decref(node->value);
	}
}
int cesk_diff_buffer_add(cesk_diff_buffer_t* buffer, int type, uint32_t addr, cesk_value_t* value)
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
	else
	{
		if(NULL == value) LOG_ERROR("invalid value field, suppose to be non-NULL");
		return -1;
	}
	_cesk_diff_node_t node = {
		.type = type,
		.addr = addr,
		.value = value
	};
	if(NULL != value) cesk_value_incref(value);
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
	for(i = 0; i < sz; i ++)
	{
		_cesk_diff_node_t* node = (_cesk_diff_node_t*)vector_get(buffer->buffer, i);
		if(NULL == node)
		{
			LOG_ERROR("can not fetch element #%d from the diff buffer", i);
			goto ERR;
		}
		if(prev_type != node->type || prev_addr != node->addr)
		{
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
					LOG_WARNING("clear the reuse flag the same address twice during a signle execution of a block might be a mistake");
					break;
				case CESK_DIFF_REG:
				case CESK_DIFF_STORE:
					/* don't forget release the reference count for the old value */
					cesk_value_decref(ret->data[diff_size].value);
					/* ok, let's move on to the next one */
					ret->data[diff_size].value = node->value;
					break;
				default:
					LOG_ERROR("unknown diff type %d", node->type);
					goto ERR;
			}
		}
		prev_type = node->type;
		prev_addr = node->addr;
		buffer->converted = i;
	}
	/* ok, we need to flush the last item */
	diff_size ++;
	size[prev_type] ++;
	buffer->converted = i;
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
			if(NULL != diff->data[ofs].value)
			{
				/* the diff occupies one reference count, so release it */
				cesk_value_decref(diff->data[ofs].value);
			}
		}
	}
	free(diff);
}
cesk_diff_t* _cesk_diff_allocate_result(int N, cesk_diff_t* args[])
{
	size_t size = 0;
	int i;
	for(i = 0; i < N; i ++)
	{
		const cesk_diff_t* diff = args[i];
		size += diff->offset[CESK_DIFF_NTYPES];
	}
	cesk_diff_t* ret = (cesk_diff_t*) malloc(sizeof(cesk_diff_t) + size * sizeof(_cesk_diff_node_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result diff");
		return NULL;
	}
	memset(ret->offset, 0, sizeof(ret->offset));
	return ret;
}
/**
 * @brief compare two heap node, if the type do not match the value of this node is infinity
 * @param this the first node
 * @param that the second node
 * @return the result of comparasion >0 means this > that, = 0 means this = that, < 0 means this < that
 **/
static inline int _cesk_diff_heap_node_cmp(const cesk_diff_t* this, const cesk_diff_t* that, int type)
{
	const _cesk_diff_node_t* thisnode = this->data + this->_index;
	const _cesk_diff_node_t* thatnode = that->data + that->_index;
	if(thisnode->type != type) return 1;    /* any order is ok */
	if(thatnode->type != type) return -1;   /* for the same reason */
	return thisnode->addr - thatnode->addr;
}
/**
 * @brief maintain the binary heap property for heap rooted in node R 
 * @param heap binary heap
 * @param R root node
 * @param N the heap size
 * @param type the expected type for this operation (e.g. if currently merging 
 *        allocation section, that means we should treat the node with non-allocation
 *        type as infitity
 * @return nothing
 **/
static inline void _cesk_diff_heap_heapify(const cesk_diff_t* heap[], int R, int N, int type)
{
	for(;R < N;)
	{
		int M = R;
		if(R * 2 + 1 < N && _cesk_diff_heap_node_cmp(heap[R * 2 + 1], heap[M], type) < 0) 
			M = R * 2 + 1;
		if(R * 2 + 2 < N && _cesk_diff_heap_node_cmp(heap[R * 2 + 2], heap[M], type) < 0) 
			M = R * 2 + 2;
		if(M == R) break;
		const cesk_diff_t* tmp = heap[M];
		heap[M] = heap[R];
		heap[R] = tmp;
		R = M;
	}
}
cesk_diff_t* cesk_diff_union(int N, cesk_diff_t* args[])
{
	cesk_diff_t* ret = _cesk_diff_allocate_result(N, args);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result");
		return NULL;
	}
	int section, offset;
	for(offset = section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
	}
	//TODO
	return NULL + offset;
}
