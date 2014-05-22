/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @details the layout of diff
 * 		Allocation Section:   <addr, init_value>
 *		Reuse Section     :   <addr, new_bit>
 *		Register Section  :   <regid, set>
 *		Store Section     :   <addr,  value>
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
	int time;           /*!< the time stamp of this node */
	void* value;        /*!< the value of this node */
};

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
		_cesk_diff_node_t* node;
		node = (_cesk_diff_node_t*)vector_get(mem->buffer, i);
		if(NULL == node || NULL == node->value) continue;
		switch(node->type)
		{
			case CESK_DIFF_ALLOC:
			case CESK_DIFF_STORE:
				cesk_value_decref((cesk_value_t*)node->value);
				break;
			case CESK_DIFF_REG:
				cesk_set_free((cesk_set_t*)node->value);
				break;
		}
	}
	vector_free(mem->buffer);
	free(mem);
}
static inline void _cesk_diff_print_record(int type, int addr, void* value)
{
	switch(type)
	{
		case CESK_DIFF_ALLOC:
			LOG_DEBUG("(allocate @%x %s)", addr, cesk_value_to_string((cesk_value_t*)value, NULL));
			break;
		case CESK_DIFF_DEALLOC:
			LOG_DEBUG("(deallocate @%x)", addr);
			break;
		case CESK_DIFF_REG:
			LOG_DEBUG("(register v%d %s)", addr, cesk_set_to_string((cesk_set_t*)value, NULL));
			break;
		case CESK_DIFF_STORE:
			LOG_DEBUG("(store @%x %s)", addr, cesk_value_to_string((cesk_value_t*)value, NULL));
			break;
		case CESK_DIFF_REUSE:
			LOG_DEBUG("(reuse @%x %d)", addr, (value != NULL));
			break;
		default:
			LOG_DEBUG("(unknown-record)");
	}
}
int cesk_diff_buffer_append(cesk_diff_buffer_t* buffer, int type, uint32_t addr, void* value)
{
	if(NULL == buffer) 
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(CESK_DIFF_DEALLOC == type)
	{
		if(NULL != value) LOG_WARNING("invalid value field, suppose to be NULL");
		value = NULL;
	}
	else if(CESK_DIFF_REUSE != type && NULL == value)
	{
		LOG_ERROR("invalid value field, suppose to be non-NULL");
		return -1;
	}
	_cesk_diff_node_t node = {
		.type = type,
		.addr = addr,
		.value = value,
		.time = vector_size(buffer->buffer)
	};
	if(buffer->reverse) node.time = -node.time;  /* so we reverse the time order */
	if(CESK_DIFF_STORE == type || CESK_DIFF_ALLOC == type) cesk_value_incref((cesk_value_t*)value);
	else if(CESK_DIFF_REG == type) node.value = cesk_set_fork((cesk_set_t*)value);
	LOG_DEBUG("append a new record to the buffer");
	_cesk_diff_print_record(type, addr, value);
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
	if(lnode->addr != rnode->addr) return lnode->addr - rnode->addr;
	return lnode->time - rnode->time;
}
cesk_diff_t* cesk_diff_from_buffer(cesk_diff_buffer_t* buffer)
{
	LOG_DEBUG("construct diff package from a buffer");
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
	
	/* first we need to sort the buffer */
	size_t sz = vector_size(buffer->buffer);
	qsort(buffer->buffer->data, sz, buffer->buffer->elem_size, _cesk_diff_buffer_cmp);

	/* then we determine the size of the diff */
	size_t size = 0;
	int i, prev_type = 0;
	uint32_t prev_addr = CESK_STORE_ADDR_NULL;
	for(i = 0; i < sz; i ++)
	{
		_cesk_diff_node_t* node = (_cesk_diff_node_t*)vector_get(buffer->buffer, i);

		_cesk_diff_print_record(node->type, node->addr, node->value);

		if(prev_type != node->type || prev_addr != node->addr)
			size ++;
		prev_type = node->type;
		prev_addr = node->addr;
	}
	LOG_DEBUG("maximum %zu slots for result", size);

	/* allocate memory for the result */
	cesk_diff_t* ret;
	ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t) + sizeof(_cesk_diff_node_t) * size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the diff");
		return NULL;
	}
	memset(ret->offset, 0, sizeof(ret->offset));
	ret->_index = 0;

	LOG_DEBUG("constructing the diff package");
	/* then we start to scan the buffer, and build our result */
	int section;
	for(i = 0, section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		ret->offset[section + 1] = ret->offset[section];
		/* for each record in this buffer which should be in current section*/
		int first = 1;
		for(;i < sz;i ++)
		{
			_cesk_diff_node_t* node = vector_get(buffer->buffer, i);
			if(node->type != section) break;
			if(first)
			{
				ret->data[ret->offset[section + 1]].addr = node->addr;
				ret->data[ret->offset[section + 1]].arg.generic = node->value;
			}
			else
			{
				if(prev_addr != node->addr)
				{
					/* we flush the the previous record, and start another one */
					_cesk_diff_print_record(section, 
					                        ret->data[ret->offset[section + 1]].addr, 
											ret->data[ret->offset[section + 1]].arg.generic);
					ret->offset[section + 1] ++;
					ret->data[ret->offset[section + 1]].addr = node->addr;
					ret->data[ret->offset[section + 1]].arg.generic = node->value;
				}
				else
				{
					switch(section)
					{
						case CESK_DIFF_ALLOC:
							LOG_WARNING("ignore the duplicated allocation record at the same store address @%x", prev_addr);
							/* we have to drop the reference */
							cesk_value_decref(node->value);
							break;
						case CESK_DIFF_DEALLOC:
							LOG_WARNING("ignore the dumplicated deallocation record at the same store address @%x", prev_addr);
							break;
						case CESK_DIFF_REUSE:
							ret->data[ret->offset[section + 1]].arg.boolean = (node->value != NULL);
							break;
						case CESK_DIFF_REG:
							if(NULL != ret->data[ret->offset[section + 1]].arg.set)
								cesk_set_free(ret->data[ret->offset[section + 1]].arg.set);
							ret->data[ret->offset[section + 1]].arg.set = (cesk_set_t*)node->value;
							break;
						case CESK_DIFF_STORE:
							if(NULL != ret->data[ret->offset[section + 1]].arg.value)
								cesk_value_decref(ret->data[ret->offset[section + 1]].arg.value);
							ret->data[ret->offset[section + 1]].arg.value = (cesk_value_t*) node->value;
							break;
						default:
							LOG_WARNING("unknown type of record");
					}
				}
			}
			prev_addr = node->addr;
			first = 0;
			buffer->converted ++;
		}
		if(!first)
		{
			_cesk_diff_print_record(section, 
									ret->data[ret->offset[section + 1]].addr, 
									ret->data[ret->offset[section + 1]].arg.generic);
			ret->offset[section + 1] ++;
		}
	}
	return ret;
}
void cesk_diff_free(cesk_diff_t* diff)
{
	if(NULL == diff) return;
	int sec, ofs = 0;
	for(sec = 0; sec < CESK_DIFF_NTYPES; sec ++)
	{
		for(ofs = diff->offset[sec]; ofs < diff->offset[sec + 1]; ofs ++)
		{
			switch(sec)
			{
				case CESK_DIFF_ALLOC:
				case CESK_DIFF_STORE:
					cesk_value_decref(diff->data[ofs].arg.value);
					break;
				case CESK_DIFF_REG:
					cesk_set_free(diff->data[ofs].arg.set);
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
	size_t size = 0;
	int i, section;
	for(i = 0; i < N; i ++)
		args[i]->_index = args[i]->offset[0];
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		uint32_t prev_addr = CESK_STORE_ADDR_NULL;
		for(;;)
		{
			uint32_t cur_addr = CESK_STORE_ADDR_NULL;
			int cur_idx = 0;
			for(i = 0; i < N; i ++)
				if(args[i]->_index < args[i]->offset[section + 1] &&
				   args[i]->data[args[i]->_index].addr < cur_addr)
				{
					cur_addr = args[i]->data[args[i]->_index].addr;
					cur_idx = i;
				}
			if(CESK_STORE_ADDR_NULL == cur_addr) break;
			args[cur_idx]->_index ++;
			if(prev_addr != cur_addr) size ++;
			prev_addr = cur_addr;
		}
	}
	cesk_diff_t* ret = (cesk_diff_t*) malloc(sizeof(cesk_diff_t) + size * sizeof(_cesk_diff_node_t));
	LOG_DEBUG("created a new diff struct with %zu slots", size);
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
	LOG_DEBUG("build diff package from applying existing package");
	/* for each section */
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		ret->offset[section + 1] = ret->offset[section];   /* the initial size of this section should be 0 */
		for(;;)
		{
			/* find the mininal address */
			uint32_t cur_addr = CESK_STORE_ADDR_NULL;
			for(i = 0; i < N; i ++)
				if(args[i]->_index < args[i]->offset[section + 1] &&
				   args[i]->data[args[i]->_index].addr < cur_addr)
					cur_addr = args[i]->data[args[i]->_index].addr;
			if(CESK_STORE_ADDR_NULL == cur_addr) break;
			/* merge the address */
			for(i = 0; i < N; i ++)
				if(args[i]->_index < args[i]->offset[section + 1] &&
				   args[i]->data[args[i]->_index].addr == cur_addr)
				{
					cesk_diff_rec_t* node = args[i]->data + args[i]->_index;
					switch(section)
					{
						case CESK_DIFF_ALLOC:
						case CESK_DIFF_REG:
						case CESK_DIFF_STORE:
						case CESK_DIFF_REUSE:
							ret->data[ret->offset[section + 1]].arg.generic = node->arg.generic;
						case CESK_DIFF_DEALLOC:
							ret->data[ret->offset[section + 1]].addr = cur_addr;
							break;
						default:
							LOG_WARNING("unknown diff type");

					}
					args[i]->_index ++;
				}
			/* fix the reference counter, becuase the ownership of the value is not correct */
			switch(section)
			{
				case CESK_DIFF_ALLOC:
				case CESK_DIFF_STORE:
					cesk_value_incref(ret->data[ret->offset[section + 1]].arg.value);
					break;
				case CESK_DIFF_REG:
					ret->data[ret->offset[section + 1]].arg.set = 
						cesk_set_fork(ret->data[ret->offset[section + 1]].arg.set);
					break;
			}
			_cesk_diff_print_record(section, 
									ret->data[ret->offset[section + 1]].addr, 
									ret->data[ret->offset[section + 1]].arg.generic);
		
			ret->offset[section + 1] ++;
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
			else
				LOG_DEBUG("elimiate allocation record %d", dealloc_ptr);
			matches = 0;
		}
		if(ret->data[dealloc_ptr].addr != ret->data[alloc_ptr].addr)
		{
			ret->data[--alloc_free] = ret->data[alloc_ptr];
		}
		else
		{
			LOG_DEBUG("elimiate allocation record %d", alloc_ptr);
			matches = 1;
		}
	}
	for(;dealloc_ptr >= dealloc_begin && ret->data[dealloc_ptr].addr > ret->data[alloc_ptr].addr; dealloc_ptr --)
	{
		if(!matches)
			ret->data[--dealloc_free] = ret->data[dealloc_ptr];
		else
			LOG_DEBUG("elimiate allocation record %d", dealloc_ptr);
		matches = 0;
	}
	for(dealloc_ptr = dealloc_free; dealloc_ptr < dealloc_end; dealloc_ptr ++)
		ret->data[dealloc_ptr - dealloc_free + dealloc_begin] = ret->data[dealloc_ptr];
	ret->offset[0] = alloc_free;
	ret->offset[CESK_DIFF_NTYPES] = dealloc_end - dealloc_free + dealloc_begin;
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

	/* initialize the _index for each input */
	int i;
	for(i = 0; i < N; i ++)
		diffs[i]->_index = diffs[i]->offset[0];
	ret->offset[0] = 0;
	/* start working */
	int section;
	cesk_set_t* result;
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		ret->offset[section + 1] = ret->offset[section];
		for(;;)
		{
			uint32_t cur_addr = CESK_STORE_ADDR_NULL;
			int      cur_i = 0, count = 0;
			for(i = 0; i < N; i ++)
				if(diffs[i]->_index < diffs[i]->offset[section + 1] &&
				   diffs[i]->data[diffs[i]->_index].addr <= cur_addr)
				{
					uint32_t this_addr = diffs[i]->data[diffs[i]->_index].addr;
					if(cur_addr != this_addr) count = 1;
					else count ++;
					cur_addr = this_addr;
					cur_i = i;
				}

			if(CESK_STORE_ADDR_NULL == cur_addr) break;
			ret->data[ret->offset[section + 1]].addr = cur_addr;
			
			switch(section)
			{
				case CESK_DIFF_DEALLOC:
					ret->data[ret->offset[section + 1]].arg.generic = NULL;
					break;
				case CESK_DIFF_ALLOC:
					ret->data[ret->offset[section + 1]].arg.value = diffs[cur_i]->data[diffs[cur_i]->_index].arg.value;
					cesk_value_incref(ret->data[ret->offset[section + 1]].arg.value);
					break;
				case CESK_DIFF_REUSE:
					ret->data[ret->offset[section + 1]].arg.boolean = diffs[cur_i]->data[diffs[cur_i]->_index].arg.boolean;
					break;
				case CESK_DIFF_REG:
					if(1 == count)
					{
						ret->data[ret->offset[section + 1]].arg.set = 
							cesk_set_fork(diffs[cur_i]->data[diffs[cur_i]->_index].arg.set);
						break;
					}
					result = cesk_set_empty_set();
					for(i = 0; i < N; i ++)
					{
						const cesk_set_t* that = current_frame[i]->regs[cur_addr];
						if(cesk_set_merge(result, that) < 0)
							LOG_WARNING("failed to merge the value of register together");
					}
					ret->data[ret->offset[section + 1]].arg.set = result; 
					break;
				case CESK_DIFF_STORE:
					if(1 == count)
					{
						ret->data[ret->offset[section + 1]].arg.value =
							diffs[cur_i]->data[diffs[cur_i]->_index].arg.value;
						cesk_value_incref(ret->data[ret->offset[section + 1]].arg.value);
					}
					/* the only way to modify object value is using allocation instruction
					 * so there's no need for processing object value at this point */
					result = cesk_set_empty_set();
					for(i = 0; i < N; i ++)
					{
						const cesk_value_const_t *val = cesk_store_get_ro(current_frame[i]->store, cur_addr);
						if(CESK_TYPE_SET != val->type)
						{
							LOG_WARNING("ignore non-set value at store address @%x in store %p",
							            cur_addr, 
							            current_frame[i]->store);
							continue;
						}
						if(cesk_set_merge(result, val->pointer.set) < 0)
							LOG_WARNING("failed to merge the value of store together");
					}
					if(NULL == (ret->data[ret->offset[section + 1]].arg.value = cesk_value_from_set(result)))
					{
						LOG_WARNING("can not create value struct for the new value");
						continue;
					}
					cesk_value_incref(ret->data[ret->offset[section + 1]].arg.value);
					break;
				default:
					LOG_WARNING("unknown type of diff record");
			}
			for(i = 0; i < N; i ++)
				if(diffs[i]->_index < diffs[i]->offset[section + 1] &&
				   diffs[i]->data[diffs[i]->_index].addr == cur_addr)
					diffs[i]->_index ++;
			ret->offset[section + 1]++;
		}
	}
	return ret;
}

