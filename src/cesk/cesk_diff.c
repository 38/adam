/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @details the layout of diff
 * 		Allocation Section:   [addr, init_value]
 *		Reuse Section     :   [addr, new_bit]
 *		Register Section  :   [regid, set]
 *		Store Section     :   [addr,  value]
 *		Deallocate Section:   [addr]
 * @todo review the code, and the idea of diff
 **/
#include <stdio.h> 

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
	uint32_t addr;           /*!< the address to operate, in the Register Segment, this means the register number */
	int time;           /*!< the time stamp of this node */
	void* value;        /*!< the value of this node */
};

cesk_diff_buffer_t* cesk_diff_buffer_new(uint8_t reverse, uint32_t merge)
{
	cesk_diff_buffer_t* ret = (cesk_diff_buffer_t*)malloc(sizeof(cesk_diff_buffer_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for this buffer");
		return NULL;
	}
	ret->converted = 0;
	ret->reverse = reverse; 
	ret->merge = merge;
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
#define __PR(fmt, args...) do{\
	int pret = snprintf(p, buf + sz - p, fmt, ##args);\
	if(pret > buf + sz - p) pret = buf + sz - p;\
	p += pret;\
}while(0)
static inline const char* _cesk_diff_record_to_string(int type, int addr, const void* value, char* buf, int sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	switch(type)
	{
		case CESK_DIFF_ALLOC:
			__PR("(allocate "PRSAddr" %s)", addr, cesk_value_to_string((cesk_value_t*)value, NULL, 0));
			break;
		case CESK_DIFF_DEALLOC:
			__PR("(deallocate "PRSAddr")", addr);
			break;
		case CESK_DIFF_REG:
			if(CESK_FRAME_REG_IS_STATIC(addr))
				__PR("(register f%d %s)", CESK_FRAME_REG_STATIC_IDX(addr), cesk_set_to_string((cesk_set_t*)value, NULL, 0));
			else
				__PR("(register v%d %s)", addr, cesk_set_to_string((cesk_set_t*)value, NULL, 0));
			break;
		case CESK_DIFF_STORE:
			__PR("(store "PRSAddr" %s)", addr, cesk_value_to_string((cesk_value_t*)value, NULL, 0));
			break;
		case CESK_DIFF_REUSE:
			__PR("(reuse "PRSAddr" %d)", addr, (value != NULL));
			break;
		default:
			__PR("(unknown-record)");
	}
	return buf;
}
const char* cesk_diff_to_string(const cesk_diff_t* diff, char* buf, int sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	int i;
	for(i = 0; i < CESK_DIFF_NTYPES; i ++)
	{
		__PR("[");
		int j;
		for(j = diff->offset[i]; j < diff->offset[i + 1]; j ++)
		{
			__PR("%s", _cesk_diff_record_to_string(i, diff->data[j].addr, diff->data[j].arg.generic, NULL, 0));
			if(j != diff->offset[i + 1] - 1) __PR(" ");
		}
		__PR("]");
	}
	return buf;
}
#undef __PR
int cesk_diff_buffer_append(cesk_diff_buffer_t* buffer, int type, uint32_t addr, const void* value)
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
		.value = (void*)value,
		.time = vector_size(buffer->buffer)
	};
	if(buffer->reverse) node.time = -node.time;  /* so we reverse the time order */
	if(CESK_DIFF_STORE == type || CESK_DIFF_ALLOC == type) cesk_value_incref((cesk_value_t*)value);
	else if(CESK_DIFF_REG == type) node.value = cesk_set_fork((cesk_set_t*)value);
	LOG_DEBUG("append a new record to the buffer %s, timestamp = %d", _cesk_diff_record_to_string(type, addr, value, NULL, 0), node.time);
	return vector_pushback(buffer->buffer, &node);
}
const void* cesk_diff_buffer_append_peek(cesk_diff_buffer_t* buffer, int type, uint32_t addr, const void* value)
{
	if(cesk_diff_buffer_append(buffer, type, addr, value) < 0)
	{
		LOG_ERROR("can not append the record to buffer");
		return NULL;
	}
	else
	{
		uint32_t sz = vector_size(buffer->buffer);
		_cesk_diff_node_t* node = (_cesk_diff_node_t*)vector_get(buffer->buffer, sz - 1);
		if(NULL == node) 
		{
			LOG_ERROR("can not get the last appended node");
			return NULL;
		}
		else
		{
			return node->value;
		}
	}
}
/**
 * @todo use faster sorting algorihtm
 **/
static int _cesk_diff_buffer_cmp(const void* left, const void* right)
{
	const _cesk_diff_node_t* lnode = (const _cesk_diff_node_t*)left;
	const _cesk_diff_node_t* rnode = (const _cesk_diff_node_t*)right;
	if(lnode->type != rnode->type) return lnode->type - rnode->type;
	if(lnode->addr != rnode->addr) //return lnode->addr - rnode->addr;
	{
		if(lnode->addr < rnode->addr) return -1;
		else if(lnode->addr == rnode->addr) return 0;
		return 1;
	}
	return lnode->time - rnode->time;
}
/**
 * @brief remove the garbage allocation
 * @details If the newly created object is dereferenced in this block, the 
 *          diff will contains an allocation instruction at that address, 
 *          but without corresponding register/store reference. In this case
 *          we should delete those block 
 * @param diff the diff package
 * @return the result, < 0 indicates an error
 **/
static inline int _cesk_diff_gc(cesk_diff_t* diff)
{
	int alloc_begin = diff->offset[CESK_DIFF_ALLOC];
	int alloc_end = diff->offset[CESK_DIFF_ALLOC + 1];
	int reg_begin = diff->offset[CESK_DIFF_REG];
	int reg_end = diff->offset[CESK_DIFF_REG + 1];
	int store_begin = diff->offset[CESK_DIFF_STORE];
	int store_end = diff->offset[CESK_DIFF_STORE + 1];
	int reuse_begin = diff->offset[CESK_DIFF_REUSE];
	int reuse_end = diff->offset[CESK_DIFF_REUSE + 1];
	int dealloc_begin = diff->offset[CESK_DIFF_DEALLOC];
	int dealloc_end = diff->offset[CESK_DIFF_DEALLOC + 1];
	int alloc_ptr, reg_ptr, store_ptr, reuse_ptr,dealloc_ptr, alloc_free, store_free, reuse_free;
	
	static uint32_t flags[CESK_STORE_ADDR_RELOC_SIZE];
	static uint32_t store_flags[CESK_STORE_ADDR_RELOC_SIZE];
	static uint32_t tick = 0;
	tick ++;

	for(store_ptr = store_begin; store_ptr < store_end; store_ptr ++)
	{
		cesk_value_t* value = diff->data[store_ptr].arg.value;
		if(NULL == value)
		{
			LOG_WARNING("ignore invalid value");
			continue;
		}
		/* if this is a value */
		if(CESK_TYPE_SET == value->type)
		{
			cesk_set_t* set = value->pointer.set;
			cesk_set_iter_t iter;
			if(NULL == cesk_set_iter(set, &iter))
			{
				LOG_WARNING("can not aquire the set iterator");
				continue;
			}
			uint32_t addr;
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
			{
				if(CESK_STORE_ADDR_IS_RELOC(addr))
				{
					uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
					flags[idx] = tick;
					LOG_DEBUG("address "PRSAddr" is safe", addr);
				}
			}
		}
		else
		{
			cesk_object_t* object = value->pointer.object;
			cesk_object_struct_t* this = object->members;
			int i;
			for(i = 0; i < object->depth; i ++)
			{
				/* if this is an instance of a built-in class */
				if(this->built_in)
				{
					uint32_t buf[128];
					uint32_t offset = 0;
					int rc;
					for(;;)
					{
						rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
						if(rc < 0)
						{
							LOG_WARNING("failed to get the address list");
							break;
						}
						if(rc == 0) break;
						offset += rc;
						int j;
						for(j = 0; j < rc; j ++)
						{
							uint32_t addr = buf[j];
							if(CESK_STORE_ADDR_IS_RELOC(addr))
							{
								uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
								flags[idx] = tick;
								LOG_DEBUG("address "PRSAddr" is safe", addr);
							}
						}
					}
				}
				/* otherwise this is an instance of user defined class */
				else
				{
					int j;
					for(j = 0; j < this->num_members; j ++)
					{
						uint32_t addr = this->addrtab[j];
						if(CESK_STORE_ADDR_IS_RELOC(addr))
						{
							uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
							flags[idx] = tick;
							LOG_DEBUG("address "PRSAddr" is safe", addr);
						}
					}
				}
				CESK_OBJECT_STRUCT_ADVANCE(this);
			}
		}
	}
	for(reg_ptr = reg_begin; reg_ptr < reg_end; reg_ptr ++)
	{
		cesk_set_t* set = diff->data[reg_ptr].arg.set;
		if(NULL == set)
		{
			LOG_WARNING("invalid register value");
			continue;
		}
		cesk_set_iter_t iter;
		if(NULL == cesk_set_iter(set, &iter))
		{
			LOG_WARNING("can not aquire the set iteartor");
			continue;
		}
		uint32_t addr;
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
		{
			if(CESK_STORE_ADDR_IS_RELOC(addr))
			{
				uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
				flags[idx] = tick;
				LOG_DEBUG("address "PRSAddr" is safe", addr);
			}
		}
	}
	/* because some allocation is used by other allocation, therefore, we need to keep them */
	for(alloc_ptr = alloc_begin; alloc_ptr < alloc_end; alloc_ptr ++)
	{
		uint32_t addr = diff->data[alloc_ptr].addr;
		cesk_value_t* value = diff->data[alloc_ptr].arg.value;
		if(!CESK_STORE_ADDR_IS_RELOC(addr))
		{
			LOG_WARNING("no way to allocate an new object in non-relocated address, must be an mistake");
			continue;
		}
		uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
		if(flags[idx] == tick)
		{
			switch(value->type)
			{
				case CESK_TYPE_OBJECT:
					{
						cesk_object_t* obj = value->pointer.object;
						cesk_object_struct_t* this = obj->members;
						int i;
						for(i = 0; i < obj->depth; i ++)
						{
							if(this->built_in)
							{
								uint32_t buf[128];
								uint32_t offset = 0;
								int rc;
								for(;;)
								{
									rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
									if(rc < 0)
									{
										LOG_WARNING("failed to tranverse the address list for built-in class %s", this->class.path->value);
										break;
									}
									if(rc == 0) break;
									int j;
									for(j = 0; j < rc; j ++)
									{
										if(CESK_STORE_ADDR_IS_RELOC(buf[j]))
										{
											uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(buf[j]);
											flags[idx] = tick;
											LOG_DEBUG("address "PRSAddr" is safe", buf[j]);
										}
									}
								}
							}
							else
							{
								int j;
								for(j = 0; j < this->num_members; j ++)
								{
									uint32_t maddr = this->addrtab[j];
									if(CESK_STORE_ADDR_IS_RELOC(maddr))
									{
										uint32_t midx = CESK_STORE_ADDR_RELOC_IDX(maddr);
										flags[midx] = tick;
										LOG_DEBUG("address "PRSAddr" is safe", maddr);
									}
								}
							}
							CESK_OBJECT_STRUCT_ADVANCE(this);
						}
					}
					break;
				case CESK_TYPE_SET:
					{
						cesk_set_t* set = value->pointer.set;
						cesk_set_iter_t iter;
						if(NULL == cesk_set_iter(set, &iter))
						{
							LOG_WARNING("can not aquire iterator for the set");
							break;
						}
						uint32_t eaddr;
						while(CESK_STORE_ADDR_NULL != (eaddr = cesk_set_iter_next(&iter)))
						{
							if(CESK_STORE_ADDR_IS_RELOC(eaddr))
							{
								uint32_t eidx = CESK_STORE_ADDR_RELOC_IDX(eaddr);
								flags[eidx] = tick;
								LOG_DEBUG("address "PRSAddr" is safe", eaddr);
							}
						}
					}
					break;
				default:
					LOG_WARNING("ignore invalid value type");
			}
		}
	}
	alloc_ptr = alloc_end - 1;
	alloc_free = alloc_end - 1;
	for(;alloc_ptr >= alloc_begin; alloc_ptr --)
	{
		int alive = 0;
		uint32_t addr = diff->data[alloc_ptr].addr;
		uint32_t idx;
		if(CESK_STORE_ADDR_IS_RELOC(addr))
		{
			idx = CESK_STORE_ADDR_RELOC_IDX(addr);
			if(flags[idx] == tick) alive = 1;
		}
		else alive = 1;

		if(alive)
		{
			LOG_DEBUG("keep living allocation record "PRSAddr"", addr);
			diff->data[alloc_free--] = diff->data[alloc_ptr];
		}
		else
		{
			LOG_DEBUG("delete garbage allocation record "PRSAddr"", addr);
			/* mark store ops on this address can be deleted */
			store_flags[idx] = tick;
			cesk_value_decref(diff->data[alloc_ptr].arg.value);
		}
	}
	diff->offset[CESK_DIFF_ALLOC] = alloc_free + 1;
	for(dealloc_ptr = dealloc_begin; dealloc_ptr < dealloc_end; dealloc_ptr ++)
	{
		uint32_t addr = diff->data[dealloc_ptr].addr;
		if(CESK_STORE_ADDR_RELOC_IDX(addr))
		{
			uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
			store_flags[idx] = tick;
		}
	}

	store_free = store_begin;
	for(store_ptr = store_begin; store_ptr < store_end; store_ptr ++)
	{
		int alive = 1;
		uint32_t addr = diff->data[store_ptr].addr;
		if(CESK_STORE_ADDR_IS_RELOC(addr))
		{
			uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
			if(store_flags[idx] == tick) alive = 0;
		}
		if(alive)
			diff->data[store_free ++] = diff->data[store_ptr];
		else
		{
			LOG_DEBUG("delete store operation on a garbage address "PRSAddr"", addr);
			cesk_value_decref(diff->data[store_ptr].arg.value);
		}
	}
	int i, j = store_free;
	/* compact the arrary*/
	for(i = store_end; i < diff->offset[CESK_DIFF_NTYPES]; i ++)
		diff->data[j++] = diff->data[i];
	/* update offset */
	for(i = CESK_DIFF_STORE + 1; i <= CESK_DIFF_NTYPES; i ++)
		diff->offset[i] -= (store_end - store_free);

	reuse_free = reuse_begin;
	for(reuse_ptr = reuse_begin; reuse_ptr < reuse_end; reuse_ptr ++)
	{
		int alive = 1;
		uint32_t addr = diff->data[reuse_ptr].addr;
		if(CESK_STORE_ADDR_IS_RELOC(addr))
		{
			uint32_t idx = CESK_STORE_ADDR_RELOC_IDX(addr);
			if(store_flags[idx] == tick) 
				alive = 0;
		}
		if(alive)
			diff->data[reuse_free ++] = diff->data[reuse_ptr];
	}
	j = reuse_free;
	/* compact the arrary*/
	for(i = reuse_end; i < diff->offset[CESK_DIFF_NTYPES]; i ++)
		diff->data[j++] = diff->data[i];
	/* update offset */
	for(i = CESK_DIFF_REUSE + 1; i <= CESK_DIFF_NTYPES; i ++)
		diff->offset[i] -= (reuse_end - reuse_free);
	return 0;
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

		if(prev_type != node->type || prev_addr != node->addr)
			size ++;
		prev_type = node->type;
		prev_addr = node->addr;
	}
	LOG_DEBUG("maximum %zu slots in result", size);

	/* allocate memory for the result */
	cesk_diff_t* ret;
	ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t) + sizeof(_cesk_diff_node_t) * size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the diff");
		return NULL;
	}
	ret->refcnt = 1;
	memset(ret->offset, 0, sizeof(ret->offset));
	ret->_index = 0;
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
					LOG_DEBUG("append new record %s",_cesk_diff_record_to_string(
						section, 
						ret->data[ret->offset[section + 1]].addr, 
						ret->data[ret->offset[section + 1]].arg.generic, NULL, 0));
					ret->offset[section + 1] ++;
					ret->data[ret->offset[section + 1]].addr = node->addr;
					ret->data[ret->offset[section + 1]].arg.generic = node->value;
				}
				else
				{
					switch(section)
					{
						case CESK_DIFF_ALLOC:
							LOG_DEBUG("ignore the duplicated allocation record at the same store address "PRSAddr"", prev_addr);
							/* we have to drop the reference */
							if(NULL != ret->data[ret->offset[section + 1]].arg.value)
								cesk_value_decref(ret->data[ret->offset[section + 1]].arg.value);
							ret->data[ret->offset[section + 1]].arg.value = (cesk_value_t*) node->value;
							break;
						case CESK_DIFF_DEALLOC:
							LOG_WARNING("ignore the dumplicated deallocation record at the same store address "PRSAddr"", prev_addr);
							break;
						case CESK_DIFF_REUSE:
							if(buffer->merge)
								ret->data[ret->offset[section + 1]].arg.boolean |= (node->value != NULL);
							else
								ret->data[ret->offset[section + 1]].arg.boolean = (node->value != NULL);
							break;
						case CESK_DIFF_REG:
							if(buffer->merge)
							{
								if(NULL == ret->data[ret->offset[section + 1]].arg.set)
									ret->data[ret->offset[section + 1]].arg.set = (cesk_set_t*)node->value;
								else
								{
									if(cesk_set_merge(ret->data[ret->offset[section + 1]].arg.set, (cesk_set_t*)node->value) < 0)
									{
										LOG_WARNING("can not merge the result register and return register, some value is ignored");
										continue;
									}
									cesk_set_free((cesk_set_t*)node->value);
								}
							}
							else
							{
								if(NULL != ret->data[ret->offset[section + 1]].arg.set)
									cesk_set_free(ret->data[ret->offset[section + 1]].arg.set);
								ret->data[ret->offset[section + 1]].arg.set = (cesk_set_t*)node->value;
							}
							break;
						case CESK_DIFF_STORE:
							if(buffer->merge)
							{
								if(NULL == ret->data[ret->offset[section + 1]].arg.value)
									ret->data[ret->offset[section + 1]].arg.value = (cesk_value_t*) node->value;
								else
								{
									int type = ((cesk_value_t*)node->value)->type;
									cesk_value_t* value = ret->data[ret->offset[section + 1]].arg.value;
									/* if current value is used by multiple users, fork it before merge */
									if(value->refcnt > 1)
									{
										cesk_value_t* new_value = cesk_value_fork(value);
										if(NULL == new_value)
										{
											LOG_WARNING("can not fork the old value to make changes, item ignored");
											break;
										}
										cesk_value_decref(value);
										cesk_value_incref(new_value);
										value = new_value;
										ret->data[ret->offset[section + 1]].arg.value = value;
									}

									/* now we can safely merge the value without affecting other value */
									if(type == CESK_TYPE_OBJECT)
									{
										/* only built-in parts needs to be merged */
										cesk_object_t *dest = value->pointer.object;
										const cesk_object_t* sour = ((cesk_value_const_t*)node->value)->pointer.object;
										if(NULL == dest->builtin || NULL == sour->builtin)
										{
											LOG_WARNING("impossible to merge a non-built-in value");
										}
										else if(dest->builtin->class.bci->class != dest->builtin->class.bci->class)
										{
											LOG_WARNING("trying to merge two values of different types");
										}
										else
										{
											cesk_object_struct_t *dest_struct = dest->builtin;
											const cesk_object_struct_t *sour_struct = sour->builtin;
											for(;;)
											{
												int rc = bci_class_merge(dest_struct, sour_struct, dest->builtin->class.bci->class);
												if(rc == 0) break;
												else if(rc < 0)
												{
													LOG_WARNING("can not merge the bci class %s", dest_struct->class.path->value);
													break;
												}
												CESK_OBJECT_STRUCT_ADVANCE(dest_struct);
												CESK_OBJECT_STRUCT_ADVANCE(sour_struct);
											}
										}
									}
									else if(type == CESK_TYPE_SET)
									{
										if(cesk_set_merge(value->pointer.set, ((cesk_value_t*)node->value)->pointer.set) < 0)
										{
											LOG_WARNING("can not merge return store diff item the result store item, some values in the store is ignored");
										}
									}
									cesk_value_decref((cesk_value_t*) node->value);
								}
							}
							else
							{
								if(NULL != ret->data[ret->offset[section + 1]].arg.value)
									cesk_value_decref(ret->data[ret->offset[section + 1]].arg.value);
								ret->data[ret->offset[section + 1]].arg.value = (cesk_value_t*) node->value;
							}
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
			LOG_DEBUG("append new record %s",_cesk_diff_record_to_string(
				section, 
				ret->data[ret->offset[section + 1]].addr, 
				ret->data[ret->offset[section + 1]].arg.generic, NULL, 0));
			ret->offset[section + 1] ++;
		}
	}
	if(_cesk_diff_gc(ret) < 0)
		LOG_WARNING("can not run diff_gc");
	LOG_DEBUG("result : %s", cesk_diff_to_string(ret, NULL, 0));
	return ret;
}
void cesk_diff_free(cesk_diff_t* diff)
{
	if(NULL == diff) return;
	if(0 == (--diff->refcnt))
	{
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
}
/**
 * @brief allocate a memory for the result
 * @param N how many inputs
 * @param args the arguments
 * @param new_reuse wether or not the caller is going to create new reuse record
 * @return the newly created memory, NULL indcates error
 **/
static inline cesk_diff_t* _cesk_diff_allocate_result(int N, cesk_diff_t* args[], int new_reuse)
{
	size_t size = 0;
	int i, section;
	for(i = 0; i < N; i ++)
		args[i]->_index = args[i]->offset[0];
	uint32_t nreuse = 0;
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
		if(new_reuse && CESK_DIFF_ALLOC == section) nreuse = size / 2;
	}
	size += nreuse;
	cesk_diff_t* ret = (cesk_diff_t*) malloc(sizeof(cesk_diff_t) + size * sizeof(_cesk_diff_node_t));
	LOG_DEBUG("created a new diff struct with %zu slots", size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result diff");
		return NULL;
	}
	memset(ret->offset, 0, sizeof(ret->offset));
	ret->refcnt = 1;
	return ret;
}
cesk_diff_t* cesk_diff_apply(int N, cesk_diff_t** args)
{
	int i;
	if(0 == N)
		return cesk_diff_empty();
	cesk_diff_t* ret = _cesk_diff_allocate_result(N, args, 0);
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
			LOG_DEBUG("new diff record %s",
				_cesk_diff_record_to_string(section, 
					ret->data[ret->offset[section + 1]].addr, 
					ret->data[ret->offset[section + 1]].arg.generic, NULL, 0));
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
	if(alloc_end - alloc_begin > 0 && dealloc_end - dealloc_begin > 0)
	{
		for(alloc_ptr = alloc_end - 1, dealloc_ptr = dealloc_end - 1; alloc_ptr >= alloc_begin; alloc_ptr --)
		{
			for(;dealloc_ptr >= dealloc_begin && ret->data[dealloc_ptr].addr > ret->data[alloc_ptr].addr; dealloc_ptr --)
			{
				if(!matches)
					ret->data[--dealloc_free] = ret->data[dealloc_ptr];
				else
					LOG_DEBUG("elimiate deallocation record %d at store address "PRSAddr"", dealloc_ptr, ret->data[dealloc_ptr].addr);
				matches = 0;
			}
			if(ret->data[dealloc_ptr].addr != ret->data[alloc_ptr].addr)
			{
				ret->data[--alloc_free] = ret->data[alloc_ptr];
			}
			else
			{
				LOG_DEBUG("elimiate allocation record %d at store address "PRSAddr"", alloc_ptr, ret->data[alloc_ptr].addr);
				/* we should elimiate the reference also */
				cesk_value_decref(ret->data[alloc_ptr].arg.value);
				matches = 1;
			}
		}

		for(;dealloc_ptr >= dealloc_begin && ret->data[dealloc_ptr].addr > ret->data[alloc_ptr].addr; dealloc_ptr --)
		{
			if(!matches)
				ret->data[--dealloc_free] = ret->data[dealloc_ptr];
			else
				LOG_DEBUG("elimiate deallocation record %d", dealloc_ptr);
			matches = 0;
		}
		for(dealloc_ptr = dealloc_free; dealloc_ptr < dealloc_end; dealloc_ptr ++)
			ret->data[dealloc_ptr - dealloc_free + dealloc_begin] = ret->data[dealloc_ptr];
		ret->offset[0] = alloc_free;
		ret->offset[CESK_DIFF_NTYPES] = dealloc_end - dealloc_free + dealloc_begin;
	}
	if(_cesk_diff_gc(ret) < 0)
		LOG_WARNING("can not run diff_gc");
	LOG_DEBUG("result : %s", cesk_diff_to_string(ret, NULL, 0));
	return ret;
}
cesk_diff_t* cesk_diff_factorize(int N, cesk_diff_t** diffs, const cesk_frame_t** current_frame)
{
	if(0 == N)
		return cesk_diff_empty();
	cesk_diff_t* ret = _cesk_diff_allocate_result(N, diffs, 1);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the result");
		return NULL;
	}

	/* initialize the _index for each input */
	int i;
	uint32_t max_n_alloc_reuse = 0;
	for(i = 0; i < N; i ++)
	{
		diffs[i]->_index = diffs[i]->offset[0];
		uint32_t alloc_sz = diffs[i]->offset[CESK_DIFF_ALLOC + 1] - diffs[i]->offset[CESK_DIFF_ALLOC];
		if(max_n_alloc_reuse < alloc_sz) max_n_alloc_reuse = alloc_sz;
	}
	ret->offset[0] = 0;
	/* start working */
	int section;
	cesk_set_t* result;
	uint32_t alloc_reuse_addr[max_n_alloc_reuse];
	uint32_t n_alloc_reuse = 0, alloc_reuse_ptr = 0;
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

			if(CESK_DIFF_REUSE == section)
			{
				for(;alloc_reuse_ptr < n_alloc_reuse && alloc_reuse_addr[alloc_reuse_ptr] < cur_addr; alloc_reuse_ptr ++)
				{
					ret->data[ret->offset[section + 1]].addr = alloc_reuse_addr[alloc_reuse_ptr];
					ret->data[ret->offset[section + 1]].arg.generic = CESK_DIFF_REUSE_VALUE(1);
					ret->offset[section + 1] ++;
				}
				if(alloc_reuse_ptr < n_alloc_reuse && alloc_reuse_addr[alloc_reuse_ptr] == cur_addr)
					alloc_reuse_ptr ++;
			}
			if(CESK_STORE_ADDR_NULL == cur_addr) break;
			ret->data[ret->offset[section + 1]].addr = cur_addr;
			
			const cesk_value_const_t* first = NULL;
			uint32_t error = 0;
			cesk_value_t* result_value = NULL;

			switch(section)
			{
				case CESK_DIFF_DEALLOC:
					ret->data[ret->offset[section + 1]].arg.generic = NULL;
					break;
				case CESK_DIFF_ALLOC:
					ret->data[ret->offset[section + 1]].arg.value = diffs[cur_i]->data[diffs[cur_i]->_index].arg.value;
					cesk_value_incref(ret->data[ret->offset[section + 1]].arg.value);
					if(count > 1)
						alloc_reuse_addr[n_alloc_reuse ++] = cur_addr;
					break;
				case CESK_DIFF_REUSE:
					ret->data[ret->offset[section + 1]].arg.generic = NULL;
					ret->data[ret->offset[section + 1]].arg.boolean = diffs[cur_i]->data[diffs[cur_i]->_index].arg.boolean;
					break;
				case CESK_DIFF_REG:
					result = cesk_set_empty_set();
					/* if current address is a reference to a real register */
					if(!CESK_FRAME_REG_IS_STATIC(cur_addr))
					{
						for(i = 0; i < N; i ++)
						{
							const cesk_set_t* that = current_frame[i]->regs[cur_addr];
							if(cesk_set_merge(result, that) < 0)
								LOG_WARNING("failed to merge the value of register together");
						}
					}
					/* this address is a reference to a static field */
					else
					{
						for(i = 0; i < N; i ++)
						{
							const cesk_set_t* that = cesk_static_table_get_ro(current_frame[i]->statics, cur_addr, 0);
							if(NULL != that && cesk_set_merge(result, that) < 0)
								LOG_WARNING("failed to merge the value of static field together");
						}
					}
					ret->data[ret->offset[section + 1]].arg.set = result; 
					break;
				case CESK_DIFF_STORE:
					/* the only way to modify object value is using allocation instruction
					 * so there's no need for processing object value at this point */
					
					first = NULL;
					for(i = 0; i < N && NULL == first; i ++)
						first = cesk_store_get_ro(current_frame[i]->store, cur_addr);
					if(NULL == first)
					{
						LOG_ERROR("nothing to change, ignore this record");
						error = 1;
						break;
					}
					if(CESK_TYPE_SET == first->type)
					{
						result = cesk_set_fork(first->pointer.set);
						for(; i < N; i ++)
						{
							const cesk_value_const_t *val = cesk_store_get_ro(current_frame[i]->store, cur_addr);
							if(NULL == val) continue;
							if(CESK_TYPE_SET != val->type)
							{
								LOG_WARNING("ignore non-set value at store address "PRSAddr" in store %p",
											cur_addr, 
											current_frame[i]->store);
								continue;
							}
							if(cesk_set_merge(result, val->pointer.set) < 0)
								LOG_WARNING("failed to merge the value of store together");
						}
						result_value = cesk_value_from_set(result);
					}
					else
					{
						if(NULL == first->pointer.object->builtin)
						{
							LOG_WARNING("An object that do not contains built-in structure supposed not to occur in the store section");
							continue;
						}
						result_value = cesk_value_fork((const cesk_value_t*)first);
						if(NULL == result_value)
						{
							LOG_WARNING("can not fork the value that is subject to modification");
							break;
						}
						for(; i < N; i ++)
						{
							const cesk_value_const_t* val = cesk_store_get_ro(current_frame[i]->store, cur_addr);
							if(NULL == val) continue;
							if(CESK_TYPE_OBJECT != val->type)
							{
								LOG_WARNING("ignore non-object value at store address "PRSAddr" in store %p",
											 cur_addr,
											 current_frame[i]->store);
							}
							else if(NULL == val->pointer.object->builtin || 
							   val->pointer.object->builtin->class.bci->class != result_value->pointer.object->builtin->class.bci->class)
							{
								LOG_WARNING("ignore two objects that are not in the same type");
							}
							else if(bci_class_merge(result_value->pointer.object->builtin->bcidata, 
											   val->pointer.object->builtin,
											   result_value->pointer.object->builtin->class.bci->class) < 0)
							{
								LOG_WARNING("can not merge the built-in object value");
							}
						}
					}

					if(NULL == (ret->data[ret->offset[section + 1]].arg.value = result_value))
					{
						LOG_WARNING("can not create value struct for the new value");
						error = 1;
						break;
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
			if(!error) ret->offset[section + 1]++;
		}
	}
	LOG_DEBUG("result : %s", cesk_diff_to_string(ret, NULL, 0));
	return ret;
}

cesk_diff_t* cesk_diff_empty()
{
	static cesk_diff_t* ret = NULL;
	if(NULL == ret)
	{
		ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t));
		if(NULL == ret)
		{
			LOG_ERROR("can not allocate memory for an empty diff");
			return NULL;
		}
		memset(ret, 0, sizeof(cesk_diff_t));
		ret->refcnt ++;   /* keep this object alive */
	}
	else 
		ret->refcnt ++;
	return ret;
}

cesk_diff_t* cesk_diff_fork(cesk_diff_t* diff)
{
	if(NULL == diff) return NULL;
	diff->refcnt ++;
	return diff;
}
cesk_diff_t* cesk_diff_prepare_to_write(cesk_diff_t* diff)
{
	int i, j, k = 0;
	if(NULL == diff) return NULL;
	/* it's safe to make type cast */
	if(1 == diff->refcnt) return (cesk_diff_t*)diff;
	/* otherwise make a copy */
	size_t size = diff->offset[CESK_DIFF_NTYPES] - diff->offset[0];
	cesk_diff_t* ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t) + sizeof(cesk_diff_rec_t) * size);
	if(NULL == ret)
	{
		LOG_ERROR("failed to allocate new value for the copy of input diff");
		return NULL;
	}
	ret->offset[0] = 0;
	uint32_t section;
	uint32_t offset;
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		ret->offset[section + 1] = ret->offset[section];
		for(offset = diff->offset[section]; offset < diff->offset[section + 1]; offset ++)
		{
			uint32_t current = ret->offset[section + 1]++;
			ret->data[current] = diff->data[offset];
			switch(section)
			{
				case CESK_DIFF_ALLOC:
				case CESK_DIFF_STORE:
					{
						cesk_value_t* newval = cesk_value_fork(ret->data[current].arg.value);
						if(NULL == newval)
						{
							LOG_ERROR("can not fork value");
							goto ERR;
						}
						cesk_value_incref(newval);
						ret->data[current].arg.value = newval;
					}
					break;
				case CESK_DIFF_REG:
					ret->data[current].arg.set = cesk_set_fork(ret->data[current].arg.set);
					if(NULL == ret->data[current].arg.set)
					{
						LOG_ERROR("can not fork set");
						goto ERR;
					}
					break;
				case CESK_DIFF_REUSE:
				case CESK_DIFF_DEALLOC:
					/* do nothing */
					break;
			}
		}
	}
	ret->refcnt = 1;
	cesk_diff_free(diff);
	return ret;
	
ERR:
	for(i = 0; i <= section; i ++)
	{
		for(j = diff->offset[i]; j < offset && j < diff->offset[i + 1]; j ++, k ++)
		{
			switch(section)
			{
				case CESK_DIFF_STORE:
				case CESK_DIFF_ALLOC:
					cesk_value_decref(ret->data[k].arg.value);
					break;
				case CESK_DIFF_REG:
					cesk_set_free(ret->data[k].arg.set);
					break;
				case CESK_DIFF_REUSE:
				case CESK_DIFF_DEALLOC:
					/* do nothing */
					break;
			}
		}
	}
	free(ret);
	return NULL;
}
int cesk_diff_identity(const cesk_diff_t* diff, const cesk_diff_t* inv)
{
	if(NULL == diff || NULL == inv) return 1; 
	int i, j;
	/* verify CESK_DIFF_REUSE section */
	if(diff->offset[CESK_DIFF_REUSE + 1] - diff->offset[CESK_DIFF_REUSE] != inv->offset[CESK_DIFF_REUSE + 1] - inv->offset[CESK_DIFF_REUSE])
		return 0;
	for(i = diff->offset[CESK_DIFF_REUSE], j = inv->offset[CESK_DIFF_REUSE]; 
		i < diff->offset[CESK_DIFF_REUSE + 1] && j < inv->offset[CESK_DIFF_REUSE + 1]; 
		i ++, j ++)
		if(diff->data[i].addr != inv->data[j].addr || diff->data[i].arg.boolean != diff->data[j].arg.boolean)
			return 0;
	/* verify CESK_DIFF_STORE section */
	if(diff->offset[CESK_DIFF_STORE + 1] - diff->offset[CESK_DIFF_STORE] != inv->offset[CESK_DIFF_STORE + 1] - inv->offset[CESK_DIFF_STORE])
		return 0;
	for(i = diff->offset[CESK_DIFF_STORE], j = inv->offset[CESK_DIFF_STORE];
		i < diff->offset[CESK_DIFF_STORE + 1] && j < inv->offset[CESK_DIFF_STORE + 1];
		i ++, j ++)
		if(diff->data[i].addr != inv->data[j].addr || 0 == cesk_set_equal(diff->data[i].arg.set, inv->data[j].arg.set))
			return 0;
	/* verify CESK_DIFF_REG section */
	if(diff->offset[CESK_DIFF_REG + 1] - diff->offset[CESK_DIFF_REG] != inv->offset[CESK_DIFF_REG + 1] - inv->offset[CESK_DIFF_REG])
		return 0;
	for(i = diff->offset[CESK_DIFF_REG], j = inv->offset[CESK_DIFF_REG];
		i < diff->offset[CESK_DIFF_REG + 1] && j < inv->offset[CESK_DIFF_REG + 1];
		i ++, j ++)
		if(diff->data[i].addr != inv->data[j].addr || 0 == cesk_set_equal(diff->data[i].arg.set, inv->data[j].arg.set))
			return 0;
	/* verify CESK_DIFF_ALLOC and CESK_DIFF_DEALLOC, all allocation should be canceled by  */
	j = inv->offset[CESK_DIFF_DEALLOC];
	for(i = diff->offset[CESK_DIFF_ALLOC]; i < diff->offset[CESK_DIFF_ALLOC + 1] && j < inv->offset[CESK_DIFF_DEALLOC + 1]; i ++)
	{
		for(;j < inv->offset[CESK_DIFF_DEALLOC + 1]; j ++)
			if(diff->data[i].addr == diff->data[j].addr)
				break;
	}
	if(i != diff->offset[CESK_DIFF_DEALLOC + 1]) return 0;
	return 1;
}
int cesk_diff_sub(cesk_diff_t* dest, const cesk_diff_t* sour)
{
	if(NULL == dest || NULL == sour)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	/* if there's an allocation section */
	if(sour->offset[CESK_DIFF_ALLOC + 1] - sour->offset[CESK_DIFF_ALLOC] > 0)
	{
		int dest_begin = dest->offset[CESK_DIFF_ALLOC];
		int dest_end   = dest->offset[CESK_DIFF_ALLOC + 1];
		int sour_begin = sour->offset[CESK_DIFF_ALLOC];
		int sour_end   = sour->offset[CESK_DIFF_ALLOC + 1];
		int dest_ptr   = dest_end - 1;
		int i, j;
		j = sour_end - 1;
		for(i = dest_end - 1; i >= dest_begin; i --)
		{
			for(; j >= sour_begin && sour->data[j].addr > dest->data[i].addr; j --);
			if(sour->data[j].addr == dest->data[i].addr)
			{
				LOG_DEBUG("allocation "PRSAddr" is deleted", sour->data[j].addr); 
			}
			else
			{
				dest->offset[dest_ptr --] = dest->offset[i];
			}
		}
		dest->offset[CESK_DIFF_ALLOC] = dest_ptr + 1;
	}
	/* if there's an deallocation section */
	if(sour->offset[CESK_DIFF_DEALLOC + 1] - sour->offset[CESK_DIFF_DEALLOC] > 0)
	{
		int dest_begin = dest->offset[CESK_DIFF_DEALLOC];
		int dest_end   = dest->offset[CESK_DIFF_DEALLOC + 1];
		int sour_begin = dest->offset[CESK_DIFF_DEALLOC];
		int sour_end   = dest->offset[CESK_DIFF_DEALLOC + 1];
		int dest_ptr = dest_begin;
		int i, j;
		j = sour_begin;
		for(i = dest_begin; i < dest_end; i ++)
		{
			for(; j < sour_end && dest->data[i].addr > sour->data[j].addr; j ++);
			if(sour->data[j].addr == dest->data[i].addr)
			{
				LOG_ERROR("deallocation "PRSAddr" is deleted", sour->data[j].addr);
			}
			else
			{
				dest->offset[dest_ptr ++] = dest->offset[i];
			}
		}
		dest->offset[CESK_DIFF_DEALLOC + 1] = dest_ptr;
	}
	return 0;
}
