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
		case CESK_TYPE_ARRAY:
			LOG_INFO("fixme : array support");
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
			cesk_store_attach_oa(store,addr,NULL);
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
/** @brief  this function is used for other function to do following things:
 * 		1. derefer all address this function refered 
 * 		2. free the set 
 */
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

int cesk_frame_register_put(cesk_frame_t* frame, uint32_t reg, cesk_set_t* set)
{
	if(NULL == frame && NULL == set)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(_cesk_frame_free_reg(frame, reg) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", reg);
		return -1;
	}
	frame->regs[reg] = set;
	return 0;
}
uint32_t cesk_frame_store_put(cesk_frame_t* frame, cesk_value_t* value, const dalvik_instruction_t* inst)
{
	if(NULL == frame || NULL == inst)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	uint32_t addr = cesk_store_allocate_oa(frame->store, inst, CESK_STORE_ADDR_NULL, 0);
	if(CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("can not allocate memory for the object");
		return CESK_STORE_ADDR_NULL;
	}
	if(cesk_store_attach_oa(frame->store, addr, value) < 0)
	{
		LOG_ERROR("can not attach the value to address @0x%x", addr);
		return CESK_STORE_ADDR_NULL;
	}
	return addr; 
}
int cesk_frame_store_object_reuse(cesk_frame_t* frame, uint32_t addr, uint8_t value)
{
	if(NULL == frame || CESK_STORE_ADDR_NULL == addr) 
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	int oldval = cesk_store_get_reuse(frame->store, addr);
	if(oldval < 0)
	{
		LOG_ERROR("can not get the old value of reuse bit");
		return -1;
	}
	int newval = (value != 0);
	if(oldval == newval) return 0;
	if(newval)
	{
		if(cesk_store_set_reuse(frame->store, addr) < 0)
		{
			LOG_ERROR("can not change the value of reuse bit");
			return -1;
		}
	}
	else
	{
		if(cesk_store_clear_reuse(frame->store, addr) < 0)
		{
			LOG_ERROR("can not change the value of reuse bit");
			return -1;
		}
	}
	return 0;
}
