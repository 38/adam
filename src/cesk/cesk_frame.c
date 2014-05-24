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
 *
 * 		 this problem can be handled by adding a timestamp in set, and each element is either new(current tick) or old(less than current tick)
 *
 * 		 And each element holds the oldest timestamp it can hold. 
 *
 * 		 And we can also implment a `new' tag, which indicates this register referencing the new value rather
 * 		 than the old value. If the register continas this value, we should override all value with new as timestamp
 * 		 should be replace.
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
				if(this->built_in)
				{
					LOG_FATAL("TODO dfs thru a built_in class");
				}
				else
				{
					int j;
					for(j = 0; j < this->num_members; j ++)
					{
						next_addr = this->addrtab[j];
						_cesk_frame_store_dfs(next_addr, store, f);
					}
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
	int ret = 0;
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
					LOG_DEBUG("allocating object %s at store address @%x", cesk_value_to_string(rec->arg.value, NULL, 0) ,rec->addr);
					if(cesk_reloc_addr_init(reloctab, frame->store, rec->addr, rec->arg.value) == CESK_STORE_ADDR_NULL)
						LOG_WARNING("can not initialize relocated address @%x in store %p", rec->addr, frame->store);
					else
						ret ++;
					break;
				case CESK_DIFF_REUSE:
					LOG_DEBUG("setting reuse flag at store address @%x to %u", rec->addr, rec->arg.boolean);
					if(rec->arg.boolean == cesk_store_get_reuse(frame->store, rec->addr)) 
					{
						LOG_DEBUG("the value reuse bit is already as excepted, no operation needed");
						break;
					}
					ret ++;
					if(rec->arg.boolean)
					{
						if(cesk_store_set_reuse(frame->store, rec->addr) < 0)
							LOG_WARNING("can not set reuse bit for @%x in store %p", rec->addr, frame->store);
					}
					else
					{
						if(cesk_store_clear_reuse(frame->store, rec->addr) < 0)
							LOG_WARNING("can not clear reuse bit for @%x in store %p", rec->addr, frame->store);
					}
					break;
				case CESK_DIFF_REG:
					LOG_DEBUG("setting register #%u to value %s", rec->addr, cesk_set_to_string(rec->arg.set, NULL, 0));
					if(cesk_set_equal(frame->regs[rec->addr], rec->arg.set))
					{
						LOG_DEBUG("the register value is already the target value, no operation needed");
						break;
					}
					ret ++;
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
					LOG_DEBUG("setting store address @%x to value %s", rec->addr, cesk_value_to_string(rec->arg.value, NULL, 0));
					cesk_value_const_t* oldval = cesk_store_get_ro(frame->store, rec->addr);
					if(NULL != oldval && CESK_TYPE_SET == oldval->type && cesk_set_equal(rec->arg.value->pointer.set, oldval->pointer.set))
					{
						LOG_DEBUG("the target value is already there, no nothing to patch");
						break;
					}
					ret ++;
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
						cesk_store_release_rw(frame->store, rec->addr);
						break;
					}
					uint32_t addr;
					while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
						cesk_store_incref(frame->store, addr);
					cesk_store_release_rw(frame->store, rec->addr);
					break;
				case CESK_DIFF_DEALLOC:
					LOG_DEBUG("ignore deallocation section");
					break;
			}
		}
	}
	return ret;
}
#define _SNAPSHOT(var, type, addr, value) do{\
	if(NULL != var && cesk_diff_buffer_append(var, type, addr, value) < 0)\
	{\
		LOG_ERROR("can not append diff record to diff buffer %s", #var);\
		return -1;\
	}\
}while(0)
int cesk_frame_register_move(
		cesk_frame_t* frame, 
		uint32_t dst_reg, 
		uint32_t src_reg, 
		cesk_diff_buffer_t* diff_buf, 
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size || src_reg >= frame->size) 
	{
		LOG_WARNING("invalid instruction, invalid register reference");
		return -1;
	}
	
	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);
	
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

	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;
}
int cesk_frame_register_clear(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	/* the register is empty ? */
	if(cesk_set_size(frame->regs[dst_reg]) == 0)
	{
		return 0;
	}
	
	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);
	
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
	
	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);
	
	return 0;
}

int cesk_frame_register_push(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{

	if(NULL == frame || dst_reg >= frame->size)
	{
		LOG_WARNING("bad instruction, invalid register number");
		return -1;
	}
	
	if(cesk_set_contain(frame->regs[dst_reg], src_addr) == 1)
	{
		return 0;
	}

	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	if(cesk_set_push(frame->regs[dst_reg], src_addr) < 0)
	{
		LOG_ERROR("can not push value @ %x to register %d", src_addr, dst_reg);
		return -1;
	}

	cesk_store_incref(frame->store, src_addr);

	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;
}

int cesk_frame_register_load(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}

	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	/* clear the register first */
	if(cesk_frame_register_clear(frame, dst_reg, NULL, NULL) < 0)
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

	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;
}
int cesk_frame_register_append_from_store(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size)
	{
		LOG_WARNING("invalid argument");
		return -1;
	}

	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	cesk_value_const_t* value = cesk_store_get_ro(frame->store, src_addr);
	
	if(NULL == value)
	{
		LOG_ERROR("can not aquire the value @ %x", src_addr);
		return -1;
	}
	if(CESK_TYPE_SET != value->type)
	{
		LOG_ERROR("can not load a non-set value to a register");
		return -1;
	}
	/* get the set object */
	const cesk_set_t* set = value->pointer.set;

	cesk_set_iter_t iter;

	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator for set @ %x", src_addr);
		return -1;
	}

	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		if(cesk_frame_register_push(frame, dst_reg, addr, NULL, NULL) < 0)
		{
			LOG_WARNING("can not push value %x to register %d", addr, dst_reg);
		}
	}

	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;

}
int cesk_frame_register_load_from_object(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_reg,
		const char* clspath,
		const char* fldname,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || dst_reg >= frame->size || src_reg >= frame->size)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}

	_SNAPSHOT(inv_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	const cesk_set_t* src_set = frame->regs[src_reg];
	cesk_set_iter_t iter;

	if(NULL == cesk_set_iter(src_set, &iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", src_reg);
		return -1;
	}

	if(cesk_frame_register_clear(frame, dst_reg, NULL, NULL) < 0)
	{
		LOG_ERROR("can not erase the destination register #%d before we start", dst_reg);
		return -1;
	}

	uint32_t obj_addr;
	while(CESK_STORE_ADDR_NULL != (obj_addr = cesk_set_iter_next(&iter)))
	{
		cesk_value_const_t* obj_val = cesk_store_get_ro(frame->store, obj_addr);
		if(NULL == obj_val)
		{
			LOG_WARNING("can not aquire readonly pointer to store address @%x", obj_addr);
			continue;
		}
		const cesk_object_t* object = obj_val->pointer.object;
		if(NULL == object)
		{
			LOG_WARNING("ignore NULL object pointer at store address @%x", obj_addr);
			continue;
		}
		uint32_t fld_addr;
		if(cesk_object_get_addr(object, clspath, fldname, &fld_addr) < 0)
		{
			LOG_WARNING("failed to fetch field %s/%s at store address @%x, ignoring this object",
			            clspath,
						fldname,
						obj_addr);
			continue;
		}
		if(cesk_frame_register_append_from_store(frame, dst_reg, fld_addr, NULL, NULL) < 0)
		{
			LOG_WARNING("can not append field value in %s/%s at store address @%x", 
			            clspath,
						fldname,
						obj_addr);
			continue;
		}
	}
	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;
}
uint32_t cesk_frame_store_new_object(
		cesk_frame_t* frame,
		cesk_reloc_table_t* reloctab,
		const dalvik_instruction_t* inst,
		const char* clspath,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || NULL == inst || NULL == clspath || NULL == reloctab)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}

	LOG_DEBUG("creat new object from class %s", clspath);
	/* allocate address */
	uint32_t addr = cesk_reloc_allocate(reloctab, frame->store, inst, CESK_STORE_ADDR_NULL, 0); 

	if(CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("can not allocate store address for object");
		return CESK_STORE_ADDR_NULL;
	}
	
	cesk_value_const_t* value;

	/* if there's some value already there */
	if((value = cesk_store_get_ro(frame->store, addr)) != NULL)
	{
		/* check validity of the address */
		if(value->type != CESK_TYPE_OBJECT)
		{
			LOG_ERROR("can not attach an object to a non-object address");
			return CESK_STORE_ADDR_NULL;
		}

		const cesk_object_t* object = value->pointer.object;
		if(NULL == object)
		{
			LOG_ERROR("invalid value");
			return CESK_STORE_ADDR_NULL; 
		}
		if(cesk_object_classpath(object) != clspath)
		{
			LOG_ERROR("address %x is for class %s but the new object is a instance of %s", 
					  addr, 
					  cesk_object_classpath(object), 
					  clspath);
			return CESK_STORE_ADDR_NULL;
		}
		/* reuse */
		_SNAPSHOT(inv_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE((uintptr_t)cesk_store_get_reuse(frame->store, addr)));

		if(cesk_store_set_reuse(frame->store, addr) < 0)
		{
			LOG_ERROR("can not reuse the address @ %x", addr);
			return CESK_STORE_ADDR_NULL;
		}

		/* push default zero to the class */
		int i;
		const cesk_object_struct_t* this = object->members;
		for(i = 0; i < object->depth; i ++)
		{
			if(!this->built_in)
			{
				int j;
				for(j = 0; j < this->num_members; j ++)
				{
					uint32_t addr = this->addrtab[j];
					if(CESK_STORE_ADDR_NULL == addr) 
					{
						LOG_WARNING("uninitialized field %s in an initialize object @%x", this->class.udef->members[j], addr);
						continue;
					}
					cesk_value_t* value = cesk_store_get_rw(frame->store, addr, 0);
					if(NULL == value)
					{
						LOG_WARNING("can not get writable pointer to store address @%x in store %p", addr, frame->store);
						continue;
					}
					if(CESK_TYPE_SET != value->type)
					{
						LOG_WARNING("expecting a set at store address @%x in store %p", addr, frame->store);
						continue;
					}
					_SNAPSHOT(inv_buf, CESK_DIFF_STORE, addr, value);
					if(cesk_set_push(value->pointer.set, CESK_STORE_ADDR_ZERO) < 0)
					{
						LOG_WARNING("can not push default zero to the class");
						continue;
					}
					_SNAPSHOT(diff_buf, CESK_DIFF_STORE, addr, value);

					cesk_store_release_rw(frame->store, addr);
				}
				/* no default zero for a built-in class */
			}
			CESK_OBJECT_STRUCT_ADVANCE(this);
		}

		_SNAPSHOT(diff_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE(1));
	}
	else
	{
		/* a fresh address, use it */
		cesk_value_t* new_val = cesk_value_from_classpath(clspath);
		if(NULL == new_val)
		{	
			LOG_ERROR("can not create new object from class %s", clspath);
			return CESK_STORE_ADDR_NULL;
		}
		if(cesk_store_attach(frame->store, addr, new_val) < 0)
		{
			LOG_ERROR("failed to attach new object to address %x", addr);
			return CESK_STORE_ADDR_NULL;
		}
		/* allocate the value set */
		cesk_object_t* object = new_val->pointer.object;
		cesk_object_struct_t* this = object->members;
		int i;
		for(i = 0; i < object->depth; i ++)
		{
			if(!this->built_in)
			{
				int j;
				for(j = 0; j < this->num_members; j ++)
				{
					uint32_t faddr = cesk_reloc_allocate(
							reloctab, 
							frame->store, 
							inst, 
							CESK_STORE_ADDR_NULL, 
							CESK_OBJECT_FIELD_OFS(object, this->addrtab + j));
					if(CESK_STORE_ADDR_NULL == faddr)
					{
						LOG_ERROR("can not allocate value set for field %s/%s", this->class.path->value, this->class.udef->members[j]);
						return -1;
					}
					/* this must be a fresh address, make a new value for it */
					cesk_value_t* fvalue = cesk_value_empty_set();
					if(NULL == fvalue)
					{
						LOG_ERROR("can not construct init value for a field");
						return -1;
					}
					/* push default zero to it */
					if(cesk_set_push(fvalue->pointer.set, CESK_STORE_ADDR_ZERO) < 0)
					{
						LOG_ERROR("can not push default zero to the set");
						return -1;
					}
					if(cesk_store_attach(frame->store, faddr, fvalue) < 0)
					{
						LOG_ERROR("can not attach the new value to store");
						return -1;
					}
					this->addrtab[j] = faddr;
					if(cesk_store_incref(frame->store, faddr) < 0)
					{
						LOG_ERROR("can not maniuplate the reference counter at store address @%x", faddr);
						return -1;
					}
					cesk_store_release_rw(frame->store, faddr);
					_SNAPSHOT(diff_buf, CESK_DIFF_ALLOC, faddr, fvalue);
					_SNAPSHOT(inv_buf, CESK_DIFF_DEALLOC, faddr, NULL);
				}
			}
			/* no need to initialze value for a built-in class */
			CESK_OBJECT_STRUCT_ADVANCE(this);
		}
		/* because the attach function auqire a writable pointer automaticly, 
		 * Therefore, you should release the address first
		 */
		cesk_store_release_rw(frame->store, addr);
		_SNAPSHOT(diff_buf, CESK_DIFF_ALLOC, addr, new_val);
		_SNAPSHOT(inv_buf, CESK_DIFF_DEALLOC, addr, NULL);
	}
	return addr;
}

int cesk_frame_store_put_field(
		cesk_frame_t* frame,
		uint32_t dst_addr,
		uint32_t src_reg,
		const char* clspath,
		const char* fldname,
		int keep_old_vlaue,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || frame->size <= src_reg)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}

	cesk_value_t* value = cesk_store_get_rw(frame->store, dst_addr, 0);
	
	if(NULL == value)
	{
		LOG_ERROR("can not find object @%x", dst_addr);
		return -1;
	}

	if(value->type != CESK_TYPE_OBJECT)
	{
		LOG_ERROR("try to put a member from a non-object address %x", dst_addr);
		return -1;
	}
	
	cesk_object_t* object = value->pointer.object;

	uint32_t* paddr = cesk_object_get(object, clspath, fldname);

	if(NULL == paddr)
	{
		LOG_ERROR("can not get field %s/%s", clspath, fldname);
		return -1;
	}

	cesk_value_t* value_set = NULL;

	if(*paddr == CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("field name is supposed not to be NULL");
		return -1;
	}

	value_set = cesk_store_get_rw(frame->store, *paddr, 0);
	if(NULL == value_set)
	{
		LOG_ERROR("can not find the value set for field %s/%s", clspath, fldname);
		return -1;
	}

	_SNAPSHOT(inv_buf, CESK_DIFF_STORE, *paddr, cesk_value_fork(value_set));  /* because we need refcount to be 1 */

	if(keep_old_vlaue || cesk_store_get_reuse(frame->store, dst_addr) == 1)
	{
		/* this address is used by mutliple objects, so we can not dicard old value */
		/* get the address of the value set */
		LOG_DEBUG("this is a reused object, just keep the old value");
	}
	else
	{
		/* we are going to drop the old value, so that we should release the writable pointer first*/
		cesk_store_release_rw(frame->store, *paddr);
		
		/* this address is used by single object, so we will lose the old value after we write the field */
		LOG_DEBUG("this address is used by signle object, drop the old value");
		value_set = cesk_value_empty_set();
		if(NULL == value_set)
		{
			LOG_ERROR("can not create an empty set for field %s/%s", clspath, fldname);
			return -1;
		}

		/* because cesk_store_attach is responsible for decref to the old value,
		 * And if the set is used only by this object, this will trigger the dispose
		 * function, and this will eventually decref all address that this set 
		 * refered */
		if(cesk_store_attach(frame->store, *paddr, value_set) < 0)   /* we just cover the old set with an empty set */
		{
			LOG_ERROR("can not attach empty set to address %x", *paddr);
			return -1;
		}
	}
	if(NULL == value_set)
	{
		LOG_ERROR("unknown error, but value_set should not be NULL");
		return -1;
	}
	cesk_set_t* set = value_set->pointer.set;

	/* of course, the refcount should be increased */
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(frame->regs[src_reg], &iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", src_reg);
		return -1;
	}
	uint32_t tmp_addr;
	while(CESK_STORE_ADDR_NULL != (tmp_addr = cesk_set_iter_next(&iter)))
	{
		/* the duplicated one do not need incref */
		if(cesk_set_contain(set, tmp_addr) == 1) continue;
		if(cesk_store_incref(frame->store, tmp_addr) < 0)
		{
			LOG_WARNING("can not incref at address @%x", tmp_addr);
		}
	}

	/* okay, append the value of registers to the set */
	if(cesk_set_merge(set, frame->regs[src_reg]) < 0)
	{
		LOG_ERROR("can not merge set");
		return -1;
	}

	/* ok take the snapshot here */
	_SNAPSHOT(diff_buf, CESK_DIFF_STORE, *paddr, cesk_value_fork(value_set));

	/* release the write pointer */
	cesk_store_release_rw(frame->store, *paddr);
	cesk_store_release_rw(frame->store, dst_addr);
	return 0;
}

/**
 * @brief load content of set to an array
 * @return number of address loaded
 */
static inline int _cesk_frame_load_set(const cesk_set_t* set, uint32_t* buf, size_t size)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator the set");
		return -1;
	}
	int ret = 0;
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)) && ret < size)
	{
		buf[ret++] = addr;
	}
	return ret;
}
int cesk_frame_register_peek(const cesk_frame_t* frame, uint32_t regid, uint32_t* buf, size_t size)
{
	if(NULL == frame || regid >= frame->size || NULL == buf)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	return _cesk_frame_load_set(frame->regs[regid], buf, size);
}
int cesk_frame_store_peek_field(const cesk_frame_t* frame, 
		uint32_t addr, 
		const char* classpath, const char* fieldname, 
		uint32_t* buf, size_t size)
{
	if(frame == NULL || 
	   NULL == classpath || NULL == fieldname ||
	   NULL == buf)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	cesk_value_const_t* value = cesk_store_get_ro(frame->store, addr);
	if(NULL == value)
	{
		LOG_ERROR("can not get value @0x%x", addr);
		return -1;
	}
	if(CESK_TYPE_OBJECT != value->type)
	{
		LOG_ERROR("type error, an object expected @0x%x", addr);
		return -1;
	}
	const cesk_object_t* object = value->pointer.object;
	if(NULL == object)
	{
		LOG_ERROR("bad object pointer");
		return -1;
	}
	uint32_t faddr;
	if(cesk_object_get_addr(object, classpath, fieldname, &faddr) < 0)
	{
		LOG_ERROR("can not get field %s/%s of object @0x%x", classpath, fieldname, addr);
		return -1;
	}
	cesk_value_const_t* set_value = cesk_store_get_ro(frame->store, faddr);
	if(CESK_TYPE_SET != set_value->type)
	{
		LOG_ERROR("bad set pointer");
		return -1;
	}
	return _cesk_frame_load_set(set_value->pointer.set, buf, size);
}
