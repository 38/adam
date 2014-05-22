/**
 * @file cesk_frame.c
 * @brief operations for one stack frame
 * @todo  ISSUE: because the reuse flag, we can not discard the value that should be discard.
 * 		  e.g:
 *
 * 		        (new-instance v1 some/Class)
 *
 * 		        (const v0 10)
 *
 * 		        (iput  v0 v1 some/Class)
 *
 * 		        (iput  v1 some_actual_value)
 *
 * 		 in this case, in fact v1 = 10 is not a possible value. However in our 
 * 		 program it just keep the value 10.
 */
#include <log.h>
#include <cesk/cesk_frame.h>
#include <cesk/cesk_store.h>

cesk_frame_t* cesk_frame_new(uint16_t size)
{
	if(0 == size)
	{
		LOG_ERROR("a frame without register? are you kidding me");
		return NULL;
	}
	size += 2;   /* because we need a result register and an expcetion register */
	LOG_DEBUG("create a frame with %d registers", size);
	cesk_frame_t* ret = (cesk_frame_t*)malloc(sizeof(cesk_frame_t) + size * sizeof(cesk_set_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory");
		return NULL;
	}
	ret->size = size;
	int i;
	for(i = 0; i < ret->size; i ++)
	{
		ret->regs[i] = cesk_set_empty_set();  /* every register is empty */
		if(NULL == ret->regs[i])
		{
			LOG_ERROR("can not create an empty set");
			goto ERROR;
		}
	}
	ret->store = cesk_store_empty_store();    /* the store constains nothing */
	return ret;
ERROR:
	if(NULL != ret)
	{
		if(NULL != ret->store)
		{
			cesk_store_free(ret->store); 
		}
		int i;
		for(i = 0; i < ret->size; i ++)
		{
			if(NULL != ret->regs[i])
			{
				cesk_set_free(ret->regs[i]);
			}
		}
	}
	return NULL;
}
cesk_frame_t* cesk_frame_fork(const cesk_frame_t* frame)
{
	if(NULL == frame) 
	{
		LOG_ERROR("invalid argument");
		return 0;
	}
	int i;
	cesk_frame_t* ret = (cesk_frame_t*)malloc(sizeof(cesk_frame_t) + frame->size * sizeof(cesk_set_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory");
		return NULL;
	}
	ret->size = frame->size;
	for(i = 0; i < frame->size; i ++)
	{
		ret->regs[i] = cesk_set_fork(frame->regs[i]);
	}
	ret->store = cesk_store_fork(frame->store);
	return ret;
}

void cesk_frame_free(cesk_frame_t* frame)
{
	if(NULL == frame) return;
	int i;
	for(i = 0; i < frame->size; i ++)
	{
		cesk_set_free(frame->regs[i]);
	}
	cesk_store_free(frame->store);
	free(frame);
}

int cesk_frame_equal(const cesk_frame_t* first, const cesk_frame_t* second)
{
	if(NULL == first || NULL == second) return first == second;
	if(cesk_frame_hashcode(first) != cesk_frame_hashcode(second)) return 0;
	if(first->size != second->size) return 0;   /* if number of registers not same */
	int i;
	for(i = 0; i < first->size; i ++)
	{
		if(0 == cesk_set_equal(first->regs[i], second->regs[i]))
		{
			/* if the register are not equal */
			return 0;
		}
	}
	return cesk_store_equal(first->store, second->store);
}
/** @brief depth first search the store, and figure out what is unreachable from the register */
static inline void _cesk_frame_store_dfs(uint32_t addr,cesk_store_t* store, uint8_t* f)
{
#define BITAT(f,n) (((f)[n/8]&(1<<(n%8))) != 0)
	if(CESK_STORE_ADDR_NULL == addr) return;
	/* constants do not need to collect */
	if(CESK_STORE_ADDR_IS_CONST(addr)) return;
	/* relocated address should be translated here */
	if(CESK_STORE_ADDR_IS_RELOC(addr))
	{
		if(NULL == store->alloc_tab)
		{
			LOG_WARNING("got an relocated address (@0x%x) without an allocation table, ingore it", addr);
			return;
		}
		if(CESK_STORE_ADDR_NULL == (addr = cesk_alloctab_query(store->alloc_tab, store, addr)))
		{
			LOG_WARNING("can not find the object address associated with relocated address @0x%x", addr);
			return;
		}
	}
	if(1 == BITAT(f, addr)) return;
	/* set the flag */
	f[addr/8] |= (1<<(addr%8));
	cesk_value_const_t* val = cesk_store_get_ro(store, addr);
	if(NULL == val) return;
	cesk_set_iter_t iter_buf;
	cesk_set_iter_t* iter;
	uint32_t next_addr;
	const cesk_object_struct_t* this;
	const cesk_object_t* obj;
	int i;
	switch(val->type)
	{
		case CESK_TYPE_SET:
			for(iter = cesk_set_iter(val->pointer.set, &iter_buf);
				CESK_STORE_ADDR_NULL != (next_addr = cesk_set_iter_next(iter));)
				_cesk_frame_store_dfs(next_addr, store, f);
			break;
		case CESK_TYPE_OBJECT:
			obj = val->pointer.object;
			this = obj->members;
			for(i = 0; i < obj->depth; i ++)
			{
				int j;
				for(j = 0; j < this->num_members; j ++)
				{
					next_addr = this->valuelist[j];
					_cesk_frame_store_dfs(next_addr, store, f);
				}
				CESK_OBJECT_STRUCT_ADVANCE(this);
			}
			break;
#if 0
		case CESK_TYPE_ARRAY:
			LOG_INFO("fixme : array support");
#endif
	}
}
int cesk_frame_gc(cesk_frame_t* frame)
{
	LOG_DEBUG("start run gc on frame@%p", frame);
	cesk_store_t* store = frame->store;
	size_t nslot = store->nblocks * CESK_STORE_BLOCK_NSLOTS;
	uint8_t *fb = (uint8_t*)malloc(nslot / 8 + 1);     /* the flag bits */
	memset(fb, 0, nslot / 8 + 1);
	int i;
	for(i = 0; i < frame->size; i ++)
	{
		cesk_set_iter_t iter;
		if(NULL == cesk_set_iter(frame->regs[i], &iter))
		{
			LOG_WARNING("can not aquire iterator for a register %d", i);
			continue;
		}
		uint32_t addr;
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
			_cesk_frame_store_dfs(addr, store, fb);
	}
	uint32_t addr = 0;
	for(addr = 0; addr < nslot; addr ++)
	{
		if(BITAT(fb,addr) == 0 && cesk_store_get_ro(store, addr) != NULL)
		{
			cesk_store_attach(store,addr,NULL);
			cesk_store_clear_refcnt(store, addr);
		}
	}
	free(fb);
	return 0;
}
hashval_t cesk_frame_hashcode(const cesk_frame_t* frame)
{
	hashval_t ret = CESK_FRAME_INIT_HASH;
	hashval_t mul = MH_MULTIPLY;
	int i;
	for(i = 0; i < frame->size; i ++)
	{
		ret ^= mul * cesk_set_hashcode(frame->regs[i]);
		mul *= MH_MULTIPLY;
	}
	ret ^= cesk_store_hashcode(frame->store);
	return ret;
}
hashval_t cesk_frame_compute_hashcode(const cesk_frame_t* frame)
{
	hashval_t ret = CESK_FRAME_INIT_HASH;
	hashval_t mul = MH_MULTIPLY;
	int i;
	for(i = 0; i < frame->size; i ++)
	{
		ret ^= mul * cesk_set_compute_hashcode(frame->regs[i]);
		mul *= MH_MULTIPLY;
	}
	ret ^= cesk_store_compute_hashcode(frame->store);
	return ret;
}
/**
 * @brief free the register, this function should be called before modifying the value of register to a new set
 * @param frame 
 * @param reg the register number
 * @return < 0 indicates error
 **/
static inline int _cesk_frame_free_reg(cesk_frame_t* frame, uint32_t reg)
{
	cesk_set_iter_t iter;
	
	if(NULL == cesk_set_iter(frame->regs[reg], &iter))
	{
		LOG_ERROR("can not aquire iterator for destination register %d", reg);
		return -1;
	}
	
	uint32_t set_addr;
	
	while(CESK_STORE_ADDR_NULL != (set_addr = cesk_set_iter_next(&iter)))
		cesk_store_decref(frame->store, set_addr);
	
	cesk_set_free(frame->regs[reg]);

	return 0;
}
/**
 * @brief make the new value in the register referencing to the store
 * @param frame 
 * @param reg
 * @return < 0 indicates error 
 **/
static inline int _cesk_frame_init_reg(cesk_frame_t* frame, uint32_t reg)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(frame->regs[reg], &iter))
	{
		LOG_ERROR("can not aquire iterator for the register %d", reg);
		return -1;
	}
	
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
		cesk_store_incref(frame->store, addr);
	return 0;
}
int cesk_frame_apply_diff(cesk_frame_t* frame, const cesk_diff_t* diff, const cesk_reloc_table_t* reloctab)
{
	if(NULL == frame || NULL == diff)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	int section;
	for(section = 0; section < CESK_DIFF_NTYPES; section ++)
	{
		int offset;
		for(offset = diff->offset[section]; offset < diff->offset[section + 1]; offset ++)
		{
			const cesk_diff_rec_t* rec = diff->data + offset;
			switch(section)
			{
				case CESK_DIFF_ALLOC:
					if(cesk_reloc_addr_init(reloctab, frame->store, rec->addr, rec->arg.value) == CESK_STORE_ADDR_NULL)
						LOG_WARNING("can not initialize relocated address @%x in store %p", rec->addr, frame->store);
					break;
				case CESK_DIFF_REUSE:
					if(cesk_store_set_reuse(frame->store, rec->arg.boolean) < 0)
						LOG_WARNING("can not set reuse bit for @%x in store %p", rec->addr, frame->store);
					break;
				case CESK_DIFF_REG:
					if(_cesk_frame_free_reg(frame, rec->addr) < 0)
					{
						LOG_WARNING("can not clean the old value in the register %d", rec->addr);
						break;
					}
					if(NULL == (frame->regs[rec->addr] = cesk_set_fork(rec->arg.set)))
					{
						LOG_WARNING("can not set the new value for register %d", rec->addr);
						break;
					}
					if(_cesk_frame_init_reg(frame, rec->addr) < 0)
					{
						LOG_WARNING("can not make reference from the register %d", rec->addr);
						break;
					}
					break;
				case CESK_DIFF_STORE:
					if(cesk_store_attach(frame->store, rec->addr, rec->arg.value) < 0)
					{
						LOG_WARNING("can not attach value to store address @%x in store %p", rec->addr, frame->store);
						break;
					}
					/* update the refcnt */
					cesk_set_iter_t iter;
					if(cesk_set_iter(rec->arg.value->pointer.set, &iter) < 0)
					{
						LOG_WARNING("can not aquire the iterator for set");
						break;
					}
					uint32_t addr;
					while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
						cesk_store_incref(frame->store, addr);
					break;
				case CESK_DIFF_DEALLOC:
					LOG_WARNING("no way to apply a deallocation to a store !");
					break;
			}
		}
	}
	return 0;
}

int cesk_frame_register_move(
		cesk_frame_t* frame, 
		uint32_t dst_reg, 
		uint32_t src_reg, 
		cesk_diff_buffer_t* diff_buf, 
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size || src_reg >= frame->size || NULL == diff_buf || NULL == inv_buf) 
	{
		LOG_WARNING("invalid instruction, invalid register reference");
		return -1;
	}
	if(cesk_diff_buffer_append(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[src_reg]) < 0)
	{
		LOG_ERROR("can not append diff record to the diff buffer");
		return -1;
	}
	if(cesk_diff_buffer_append(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not append diff record to the diff buffer");
		return -1;
	}
	/* as once we write one register, the previous infomation store in the register is lost */
	if(_cesk_frame_free_reg(frame, dst_reg) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", dst_reg);
		return -1;
	}
	/* and then we just fork the vlaue of source */
	frame->regs[dst_reg] = cesk_set_fork(frame->regs[src_reg]);

	if(_cesk_frame_init_reg(frame, dst_reg) < 0)
	{
		LOG_ERROR("can not incref for the new register value");
		return -1;
	}
	return 0;
}
int cesk_frame_register_clear(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size || NULL == diff_buf || NULL == inv_buf)
	{
		LOG_WARNING("invalid argument");
		return -1;
	}
	/* the register is empty ? */
	if(cesk_set_size(frame->regs[dst_reg]) == 0)
	{
		return 0;
	}
	if(cesk_diff_buffer_append(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not append the diff to the inverse diff buffer");
		return -1;
	}
	if(_cesk_frame_free_reg(frame,dst_reg) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", dst_reg);
		return -1;
	}

	frame->regs[dst_reg] = cesk_set_empty_set();
	if(frame->regs[dst_reg] == NULL)
	{
		LOG_ERROR("can not create an empty set for register %d", dst_reg);
		return -1;
	}
	if(cesk_diff_buffer_append(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not append the diff to the diff buffer");
		return -1;
	}
	return 0;
}

int cesk_frame_register_load(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size || NULL == diff_buf || NULL == inv_buf)
	{
		LOG_WARNING("invalid argument");
		return -1;
	}
	if(cesk_diff_buffer_append(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not append the diff to the inverse diff buffer");
		return -1;
	}
	/* clear the register first */
	if(cesk_frame_register_clear(frame, dst_reg, diff_buf, inv_buf) < 0)
	{
		LOG_ERROR("can not clear the value in register %d", dst_reg);
		return -1;
	}

	if(cesk_set_push(frame->regs[dst_reg], src_addr) < 0)
	{
		LOG_ERROR("can not push address @%x to register %d", src_addr, dst_reg);
		return -1;
	}

	if(cesk_store_incref(frame->store, src_addr) < 0)
	{
		LOG_ERROR("can not decref for the cell @%x", src_addr);
		return -1;
	}
	if(cesk_diff_buffer_append(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not append the diff to the diff buffer");
		return -1;
	}
	return 0;
}

//TODO frame operations
