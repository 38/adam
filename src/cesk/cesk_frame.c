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
#include <stdio.h>
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
	LOG_DEBUG("create an empty frame with %d registers", size);
	cesk_frame_t* ret = (cesk_frame_t*)malloc(sizeof(cesk_frame_t) + size * sizeof(cesk_set_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory");
		return NULL;
	}
	memset(ret, 0, sizeof(cesk_frame_t) + size * sizeof(cesk_set_t*));
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
	if(NULL == ret->store) 
	{
		LOG_ERROR("can not create an empty store for the new frame");
		goto ERROR;
	}
	ret->statics = cesk_static_table_fork(NULL); /* a static field table with initial values */ 
	if(NULL == ret->statics)
	{
		LOG_ERROR("can not create a initial static field table for the new frame");
		goto ERROR;
	}
	return ret;
ERROR:
	if(NULL != ret)
	{
		if(NULL != ret->store)
		{
			cesk_store_free(ret->store); 
		}
		if(NULL != ret->statics)
		{
			cesk_static_table_free(ret->statics);
		}
		int i;
		for(i = 0; i < ret->size; i ++)
		{
			if(NULL != ret->regs[i])
			{
				cesk_set_free(ret->regs[i]);
			}
		}

		free(ret);
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
	int i,j;
	cesk_frame_t* ret = (cesk_frame_t*)malloc(sizeof(cesk_frame_t) + frame->size * sizeof(cesk_set_t*));
	memset(ret, 0, sizeof(cesk_frame_t) + frame->size * sizeof(cesk_set_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory");
		return NULL;
	}
	ret->size = frame->size;
	for(i = 0; i < frame->size; i ++)
	{
		ret->regs[i] = cesk_set_fork(frame->regs[i]);
		if(NULL == ret->regs[i])
		{
			LOG_ERROR("can not fork the set for the new register");
			goto ERR;
		}
	}
	ret->store = cesk_store_fork(frame->store);
	if(NULL == ret->store)
	{
		LOG_ERROR("can not fork the frame store");
		goto ERR;
	}
	ret->statics = cesk_static_table_fork(frame->statics);
	if(NULL == ret->statics)
	{
		LOG_ERROR("can not fork the static field table");
		goto ERR;
	}
	return ret;
ERR:
	if(NULL != ret)
	{
		for(j = 0; j < i; i ++)
		{
			if(NULL != ret->regs[i]) cesk_set_free(ret->regs[i]);
		}
		if(NULL != ret->store) cesk_store_free(ret->store);
		if(NULL != ret->statics) cesk_static_table_fork(ret->statics);
		free(ret);
	}
	return NULL;
}
cesk_frame_t* cesk_frame_make_invoke(const cesk_frame_t* frame, uint32_t nregs, uint32_t nargs, cesk_set_t** args)
{
	/* make sure the invocation argument is valid */
	if(NULL == frame || nregs > 65536 || nargs > nregs || NULL == args)
	{
		LOG_ERROR("invalid argument");
		return 0;
	}
	/* memory allocation */
	cesk_frame_t* ret = (cesk_frame_t*)malloc(sizeof(cesk_frame_t) + (nregs + 2) * sizeof(cesk_set_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory");
		return NULL;
	}
	/* we have result and exception resiter */
	ret->size = nregs + 2;
	/* register init */
	int i;
	/* first part is the register that does not carry a parameter */
	for(i = 0; i < ret->size - nargs; i ++)
	{
		ret->regs[i] = cesk_set_empty_set();
		if(NULL == ret->regs[i])
		{
			LOG_ERROR("can not create an empty set for the register %d", i);
			goto ERR;
		}
	}
	/* second part is the parameters */
	for(;i < ret->size; i ++)
	{
		ret->regs[i] = args[i - ret->size + nargs];
		if(NULL == ret->regs[i])
		{
			LOG_ERROR("invalid argument list");
			goto ERR;
		}
	}
	/* store init */
	ret->store = cesk_store_fork(frame->store);
	if(NULL == ret->store)
	{
		LOG_ERROR("can not fork the frame store");
		goto ERR;
	}
	/* apply the relocation table to the register */
	for(i = 0; i < ret->size; i ++)
	{
		cesk_set_t* reg = ret->regs[i];
		cesk_set_iter_t iter;
		if(cesk_set_iter(reg, &iter) == NULL)
		{
			LOG_ERROR("can not aquire iterator for the regsiter #%d", i);
			goto ERR;
		}
		uint32_t addr;
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
		{
			if(CESK_STORE_ADDR_IS_RELOC(addr))
			{
				uint32_t naddr = cesk_alloctab_query(ret->store->alloc_tab, ret->store, addr);
				if(CESK_STORE_ADDR_NULL == naddr)
				{
					LOG_ERROR("can not find the relocated address "PRSAddr"", addr);
					goto ERR;
				}
				if(cesk_set_modify(reg, addr, naddr) < 0)
				{
					LOG_ERROR("can not update relocated address "PRSAddr" --> "PRSAddr"", addr, naddr);
					goto ERR;
				}
			}
		}
	}
	/* in addition, we should copy the static field */
	ret->statics = cesk_static_table_fork(frame->statics);
	if(NULL == ret->statics)
	{
		LOG_ERROR("can not copy the static field table");
		goto ERR;
	}
	/* apply the allocation table toe the store */
	if(cesk_store_apply_alloctab(ret->store) < 0)
	{
		LOG_ERROR("failed to apply the allocation table to the store frake");
		goto ERR;
	}
	/*run garbage collector*/
	if(cesk_frame_gc(ret) < 0)
	{
		LOG_ERROR("can not run garbage collector on the new store");
		goto ERR;
	}
	return ret;
ERR:
	if(NULL != ret)
	{
		for(i = 0; i < ret->size; i ++)
			if(NULL != ret->regs[i])
				cesk_set_free(ret->regs[i]);
		if(NULL != ret->store)
			cesk_store_free(ret->store);
		if(NULL != ret->statics)
			cesk_static_table_free(ret->statics);
		free(ret);
	}
	return NULL;
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
	cesk_static_table_free(frame->statics);
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
		/* if the register are not equal, they are not the same frame */
		if(0 == cesk_set_equal(first->regs[i], second->regs[i]))
		{
			return 0;
		}
	}
	return cesk_static_table_equal(first->statics, second->statics) && cesk_store_equal(first->store, second->store);
}
/** 
 * @brief depth first search the store, and figure out what is unreachable from the register
 * @param addr the start address
 * @param store the target store
 * @param f the bit map used to flag reachibilities of each addresses
 **/
static inline void _cesk_frame_store_dfs(uint32_t addr, cesk_store_t* store, uint8_t* f)
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
				/* if this instance is a built-in instance */
				if(this->built_in)
				{
					uint32_t buf[1024];
					uint32_t offset = 0;
					for(;;)
					{
						int rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
						if(rc < 0)
						{
							LOG_WARNING("can not get the address list");
							continue;
						}
						else
						{
							if(rc == 0) break;
							offset += rc;
							int i;
							for(i = 0; i < rc; i ++)
								_cesk_frame_store_dfs(buf[i], store, f);
						}
					}
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
	/* traverse from register */
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
	/* traverse from static fields */
	cesk_static_table_iter_t iter;
	if(NULL != cesk_static_table_iter(frame->statics, &iter))
	{
		const cesk_set_t* cur_set;
		for(;NULL != (cur_set = cesk_static_table_iter_next(&iter, NULL));)
		{
			cesk_set_iter_t iter;
			if(NULL == cesk_set_iter(cur_set, &iter))
			{
				LOG_WARNING("can not aquire set iterator for the static field value set");
				continue;
			}
			uint32_t addr;
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
				_cesk_frame_store_dfs(addr, store, fb);
		}
	}
	else
	{
		LOG_WARNING("can not aquire iterator for the static field table");
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
	/* finally, dereference all unused block in the end of the store */
	if(cesk_store_compact_store(frame->store) < 0)
	{
		LOG_WARNING("failed to compact the store, something might be wrong");
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
 * @brief free a value set, this function should be called before modifying the value of register to a new set
 * @param frame 
 * @param set the set to be freed
 * @return < 0 indicates error
 **/
static inline int _cesk_frame_free_set(cesk_frame_t* frame, cesk_set_t* set)
{
	cesk_set_iter_t iter;
	
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator for the vlaue set");
		return -1;
	}
	
	uint32_t set_addr;
	
	while(CESK_STORE_ADDR_NULL != (set_addr = cesk_set_iter_next(&iter)))
		cesk_store_decref(frame->store, set_addr);
	
	cesk_set_free(set);

	return 0;
}
/**
 * @brief make the new value in the register referencing to the store
 * @param frame 
 * @param set 
 * @return < 0 indicates error 
 **/
static inline int _cesk_frame_set_ref(cesk_frame_t* frame, const cesk_set_t* set)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator for set");
		return -1;
	}
	
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
		cesk_store_incref(frame->store, addr);
	return 0;
}
int cesk_frame_apply_diff(
	cesk_frame_t* frame, 
	const cesk_diff_t* diff, 
	const cesk_reloc_table_t* reloctab, 
	cesk_diff_buffer_t* fwdbuf,
	cesk_diff_buffer_t* invbuf)
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
					LOG_DEBUG("allocating object %s at store address "PRSAddr"", cesk_value_to_string(rec->arg.value, NULL, 0) ,rec->addr);
					if(cesk_reloc_addr_init(reloctab, frame->store, rec->addr, rec->arg.value) == CESK_STORE_ADDR_NULL)
					{
						LOG_ERROR("can not initialize relocated address "PRSAddr" in store %p", rec->addr, frame->store);
						return -1;
					}
					else
						ret ++;
					if(NULL != invbuf && cesk_diff_buffer_append(invbuf, CESK_DIFF_DEALLOC, rec->addr, NULL) < 0)
					{
						LOG_ERROR("can not append a new record to the inverse diff buf");
						return -1;
					}
					if(NULL != fwdbuf && cesk_diff_buffer_append(fwdbuf, CESK_DIFF_ALLOC, rec->addr, rec->arg.value) < 0)
					{
						LOG_ERROR("can not append a new record to the foward diff buffer");
						return -1;
					}
					break;
				case CESK_DIFF_REUSE:
					LOG_DEBUG("setting reuse flag at store address "PRSAddr" to %u", rec->addr, rec->arg.boolean);
					if(rec->arg.boolean == cesk_store_get_reuse(frame->store, rec->addr)) 
					{
						LOG_DEBUG("the value reuse bit is already as excepted, no operation needed");
						break;
					}
					ret ++;
					if(rec->arg.boolean)
					{
						if(cesk_store_set_reuse(frame->store, rec->addr) < 0)
						{
							LOG_ERROR("can not set reuse bit for "PRSAddr" in store %p", rec->addr, frame->store);
							return -1;
						}
						if(NULL != invbuf && cesk_diff_buffer_append(invbuf, CESK_DIFF_REUSE, rec->addr, CESK_DIFF_REUSE_VALUE(0)) < 0)
						{
							LOG_ERROR("can not append a new record for the inverse diff buf");
							return -1;
						}
						if(NULL != fwdbuf && cesk_diff_buffer_append(fwdbuf, CESK_DIFF_REUSE, rec->addr, CESK_DIFF_REUSE_VALUE(1)) < 0)
						{
							LOG_ERROR("can not append a new record to the foward diff buffer");
							return -1;
						}
					}
					else
					{
						if(cesk_store_clear_reuse(frame->store, rec->addr) < 0)
						{
							LOG_ERROR("can not clear reuse bit for "PRSAddr" in store %p", rec->addr, frame->store);
							return -1;
						}
						if(NULL != invbuf && cesk_diff_buffer_append(invbuf, CESK_DIFF_REUSE, rec->addr, CESK_DIFF_REUSE_VALUE(1)) < 0)
						{
							LOG_ERROR("can not append a new record for the inverse diff buf");
							return -1;
						}
						if(NULL != fwdbuf && cesk_diff_buffer_append(fwdbuf, CESK_DIFF_REUSE, rec->addr, CESK_DIFF_REUSE_VALUE(0)) < 0)
						{
							LOG_ERROR("can not append a new record to the foward diff buffer");
							return -1;
						}
					}
					break;
				case CESK_DIFF_REG:
					LOG_DEBUG("setting register #%u to value %s", rec->addr, cesk_set_to_string(rec->arg.set, NULL, 0));
					if(cesk_set_equal(frame->regs[rec->addr], rec->arg.set))
					{
						LOG_DEBUG("the register value is already the target value, no operation needed");
						break;
					}
					if(NULL != invbuf && cesk_diff_buffer_append(invbuf, CESK_DIFF_REG, rec->addr, frame->regs[rec->addr]) < 0)
					{
						LOG_ERROR("can not append a new record for the inverse diff buf");
						return -1;
					}
					if(NULL != fwdbuf && cesk_diff_buffer_append(fwdbuf, CESK_DIFF_REG, rec->addr, rec->arg.set) < 0)
					{
						LOG_ERROR("can not append a new record to the foward diff buffer");
						return -1;
					}
					ret ++;
					/* we need to make reference to the new values before the old register is freed, because
					 * if we don't do this, some value which contains in both old register value and new register
					 * value will die at after this, which leads the new register reference to a invalid address,
					 * But if we initialize the refcount at this point, this won't happen */
					if(_cesk_frame_set_ref(frame, rec->arg.set) < 0)
					{
						LOG_ERROR("can not make reference from the register %d", rec->addr);
						return -1;
					}

					if(_cesk_frame_free_set(frame, frame->regs[rec->addr]) < 0)
					{
						LOG_ERROR("can not clean the old value in the register %d", rec->addr);
						return -1;
					}
					if(NULL == (frame->regs[rec->addr] = cesk_set_fork(rec->arg.set)))
					{
						LOG_ERROR("can not set the new value for register %d", rec->addr);
						return -1;
					}
					break;
				case CESK_DIFF_STORE:
					LOG_DEBUG("setting store address "PRSAddr" to value %s", rec->addr, cesk_value_to_string(rec->arg.value, NULL, 0));
					cesk_value_const_t* oldval = cesk_store_get_ro(frame->store, rec->addr);
					if(oldval->type != rec->arg.value->type)
					{
						LOG_ERROR("the diff record value and the target value was supposed to be the same type, but they are not");
						return -1;
					}
					/* TODO: elimiate this dirty hack */
					if(NULL != oldval && cesk_value_equal(rec->arg.value, (const cesk_value_t*)oldval))
					{
						LOG_DEBUG("the target value is already there, no nothing to patch");
						break;
					}
					if(NULL != invbuf && cesk_diff_buffer_append(invbuf, CESK_DIFF_STORE, rec->addr, oldval) < 0)
					{
						LOG_ERROR("can not append a new record for the inverse diff buf");
						return -1;
					}
					if(NULL != fwdbuf && cesk_diff_buffer_append(fwdbuf, CESK_DIFF_STORE, rec->addr, rec->arg.value) < 0)
					{
						LOG_ERROR("can not append a new record to the foward diff buffer");
						return -1;
					}
				
					/* update the refcnt first, so that we can keep the value we want */
					/* if this value is a type */
					if(CESK_TYPE_SET == oldval->type)
					{
						cesk_set_iter_t iter;
						if(cesk_set_iter(rec->arg.value->pointer.set, &iter) < 0)
						{
							LOG_ERROR("can not aquire the iterator for set");
							cesk_store_release_rw(frame->store, rec->addr);
							return -1;
						}
						uint32_t addr;
						while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
							cesk_store_incref(frame->store, addr);
					}
					else
					{
						int i;
						const cesk_object_t* object = rec->arg.value->pointer.object;
						const cesk_object_struct_t* this = object->members;
						for(i = 0; i < object->depth; i ++)
						{
							if(this->built_in)
							{
								uint32_t offset = 0;
								uint32_t buf[128];
								for(;;)
								{
									int rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
									if(rc < 0)
									{
										LOG_ERROR("can not get the address list for this object");
									}
									if(rc == 0) break;
									offset += rc;
									int j;
									for(j = 0; j < rc; j ++)
										cesk_store_incref(frame->store, buf[j]);
								}
							}
							else
							{
								int j;
								for(j = 0; j < this->num_members; j ++)
									cesk_store_incref(frame->store, this->addrtab[j]);
							}
							CESK_OBJECT_STRUCT_ADVANCE(this);
						}
					}

					/* replace the value */
					if(cesk_store_attach(frame->store, rec->addr, rec->arg.value) < 0)
					{
						LOG_ERROR("can not attach value to store address "PRSAddr" in store %p", rec->addr, frame->store);
						return -1;
					}
					cesk_store_release_rw(frame->store, rec->addr);
					ret ++;
					break;
				case CESK_DIFF_DEALLOC:
					LOG_DEBUG("ignore deallocation section");
					break;
			}
		}
	}
	/* and we should go over the allocation section again to fix the reference counter in the store */
	int offset;
	for(offset = diff->offset[CESK_DIFF_ALLOC]; offset < diff->offset[CESK_DIFF_ALLOC + 1]; offset ++)
	{
		int i;
		const cesk_value_t* val = diff->data[offset].arg.value;
		const cesk_object_t* obj;
		const cesk_object_struct_t* this;
		const cesk_set_t *set;
		cesk_set_iter_t iter;
		switch(val->type)
		{
			case CESK_TYPE_OBJECT:
				obj = val->pointer.object;
				this = obj->members;
				for(i = 0; i < obj->depth; i ++)
				{
					if(this->built_in)
					{
						uint32_t offset = 0;
						uint32_t buf[128];
						for(;;)
						{
							int rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
							if(rc < 0)
							{
								LOG_ERROR("can not get the address list for this object");
							}
							if(rc == 0) break;
							offset += rc;
							int j;
							for(j = 0; j < rc; j ++)
								cesk_store_incref(frame->store, buf[j]);
						}
					}
					else
					{
						int j;
						for(j = 0; j < this->num_members; j ++)
							cesk_store_incref(frame->store, this->addrtab[j]);
					}
					CESK_OBJECT_STRUCT_ADVANCE(this);
				}
				break;
			case CESK_TYPE_SET:
				set = val->pointer.set;
				if(NULL == cesk_set_iter(set, &iter))
				{
					LOG_ERROR("can not aquire set iterator for set");
					return -1;
				}
				uint32_t addr;
				while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
					cesk_store_incref(frame->store, addr);
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

	/* increase the refcount here, otherwise the object in use might die after _cesk_frame_free_reg */
	if(_cesk_frame_set_ref(frame, frame->regs[src_reg]) < 0)
	{
		LOG_ERROR("can not incref for the new register value");
		return -1;
	}

	/* as once we write one register, the previous infomation store in the register is lost */
	if(_cesk_frame_free_set(frame, frame->regs[dst_reg]) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", dst_reg);
		return -1;
	}
	/* and then we just fork the vlaue of source */
	frame->regs[dst_reg] = cesk_set_fork(frame->regs[src_reg]);

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
	
	if(_cesk_frame_free_set(frame, frame->regs[dst_reg]) < 0)
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
		uint32_t incref,
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

	if(incref) cesk_store_incref(frame->store, src_addr);

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

	/* incref first, the reason see comment in cesk_frame_apply_diff the part operates the register */
	if(cesk_store_incref(frame->store, src_addr) < 0)
	{
		LOG_ERROR("can not decref for the cell "PRSAddr"", src_addr);
		return -1;
	}

	/* clear the register first */
	if(cesk_frame_register_clear(frame, dst_reg, NULL, NULL) < 0)
	{
		LOG_ERROR("can not clear the value in register %d", dst_reg);
		return -1;
	}

	if(cesk_set_push(frame->regs[dst_reg], src_addr) < 0)
	{
		LOG_ERROR("can not push address "PRSAddr" to register %d", src_addr, dst_reg);
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
		if(cesk_frame_register_push(frame, dst_reg, addr, 1, NULL, NULL) < 0)
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

	/* we firstly clear the the value set, but keep the refcount */
	cesk_set_t* old_val = frame->regs[dst_reg];
	if(cesk_set_size(old_val) > 0)
	{
		if(NULL == (frame->regs[dst_reg] = cesk_set_empty_set()))
		{
			LOG_ERROR("can not allocate empty set for cleared register %d", dst_reg);
			return -1;
		}
	}

	uint32_t obj_addr;
	while(CESK_STORE_ADDR_NULL != (obj_addr = cesk_set_iter_next(&iter)))
	{
		/* constant address should be ignored */
		if(CESK_STORE_ADDR_IS_CONST(obj_addr))
		{
			/* TODO: set exception register */
			LOG_DEBUG("throw exception java.lang.nullPointerException");
			continue;
		}
		cesk_value_const_t* obj_val = cesk_store_get_ro(frame->store, obj_addr);
		if(NULL == obj_val)
		{
			LOG_WARNING("can not aquire readonly pointer to store address "PRSAddr"", obj_addr);
			continue;
		}
		const cesk_object_t* object = obj_val->pointer.object;
		if(NULL == object)
		{
			LOG_WARNING("ignore NULL object pointer at store address "PRSAddr"", obj_addr);
			continue;
		}
		uint32_t fld_addr;
		const bci_class_t* builtin_class;
		const void* builtin_data;
		if(cesk_object_get_addr(object, clspath, fldname, &builtin_class, &builtin_data, &fld_addr) < 0)
		{
			cesk_set_t* set;
			if(NULL != builtin_class && NULL != builtin_data && NULL != (set = bci_class_get_field(builtin_data, fldname, builtin_class)))
			{
				frame->regs[dst_reg] = set;
				cesk_set_iter_t iter;
				uint32_t addr;
				if(cesk_set_iter(set, &iter) == NULL)
				{
					LOG_WARNING("can not aquire set iterator");
					continue;	
				}
				while(CESK_STORE_ADDR_NULL == (addr = cesk_set_iter_next(&iter)))
					cesk_store_incref(frame->store, addr);
				continue;
			}
			LOG_WARNING("failed to fetch field %s/%s at store address "PRSAddr", ignoring this object",
						clspath,
						fldname,
						obj_addr);
			continue;
		}
		/* this is a user defined class */
		else
		{
			if(cesk_frame_register_append_from_store(frame, dst_reg, fld_addr, NULL, NULL) < 0)
			{
				LOG_WARNING("can not append field value in %s/%s at store address "PRSAddr"", 
							clspath,
							fldname,
							obj_addr);
				continue;
			}
		}
	}
	/* now we fix the refcount */
	if(old_val != frame->regs[dst_reg] && _cesk_frame_free_set(frame, old_val) < 0)
	{
		LOG_ERROR("can not fix the refcount after a reigster assignment");
		return -1;
	}
	_SNAPSHOT(diff_buf, CESK_DIFF_REG, dst_reg, frame->regs[dst_reg]);

	return 0;
}
uint32_t cesk_frame_store_new_object(
		cesk_frame_t* frame,
		cesk_reloc_table_t* reloctab,
		const dalvik_instruction_t* inst,
		const cesk_alloc_param_t*   alloc_param,
		const char* clspath,
		const void* bci_init_param,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf)
{
	if(NULL == frame || NULL == inst || NULL == alloc_param || NULL == clspath || NULL == reloctab)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}

	LOG_DEBUG("creat new object from class %s", clspath);
	/* allocate a relocated address for the new object */
	cesk_alloc_param_t param = *alloc_param;
	param.inst = dalvik_instruction_get_index(inst);
	param.offset = 0;
	uint32_t addr = cesk_reloc_allocate(reloctab, frame->store, &param, 0); 

	if(CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("can not allocate store address for object");
		return CESK_STORE_ADDR_NULL;
	}
	

	/* because the built-in class might change the object itself for reuse, so that we need make a r/w pointer here */
	cesk_value_t* value;

	/* if there's some value already exist */
	if((value = cesk_store_get_rw(frame->store, addr, 0)) != NULL)
	{
		/* constant allocation? ignore */
		if(DVM_CONST == inst->opcode) 
		{
			LOG_DEBUG("ignore the second time allocation of a constant value");
			return addr;
		}
		
		/* check validity of the address */
		if(value->type != CESK_TYPE_OBJECT)
		{
			LOG_ERROR("can not attach an object to a non-object address");
			return CESK_STORE_ADDR_NULL;
		}

		cesk_object_t* object = value->pointer.object;
		if(NULL == object)
		{
			LOG_ERROR("invalid value");
			return CESK_STORE_ADDR_NULL; 
		}
		if(cesk_object_classpath(object) != clspath)
		{
			LOG_ERROR("address %x is for class %s but the new object is a instance of %s (but suppose to be homogenous)", 
					  addr, 
					  cesk_object_classpath(object), 
					  clspath);
			return CESK_STORE_ADDR_NULL;
		}

		/* push default zero to the class */
		int i;
		cesk_object_struct_t* this = object->members;
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
						LOG_WARNING("uninitialized field %s in an initialize object "PRSAddr"", this->class.udef->members[j], addr);
						continue;
					}
					
					/* reuse */
					_SNAPSHOT(inv_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE((uintptr_t)cesk_store_get_reuse(frame->store, addr)));

					if(cesk_store_set_reuse(frame->store, addr) < 0)
					{
						LOG_ERROR("can not reuse the address @ %x", addr);
						return CESK_STORE_ADDR_NULL;
					}

					cesk_value_t* value = cesk_store_get_rw(frame->store, addr, 0);
					if(NULL == value)
					{
						LOG_WARNING("can not get writable pointer to store address "PRSAddr" in store %p", addr, frame->store);
						continue;
					}
					if(CESK_TYPE_SET != value->type)
					{
						LOG_WARNING("expecting a set at store address "PRSAddr" in store %p", addr, frame->store);
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
					
					_SNAPSHOT(diff_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE(1));
				}
				/* no default zero for a built-in class, but we need to re-initialize the class */
			}
			else
			{
				if(bci_class_initialize(this->bcidata, bci_init_param, this->class.bci->class) < 0)
				{
					LOG_WARNING("failed to initlaize the built-in class");
				}
			}
			CESK_OBJECT_STRUCT_ADVANCE(this);
		}

		_SNAPSHOT(inv_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE((uintptr_t)cesk_store_get_reuse(frame->store, addr)));
		cesk_store_set_reuse(frame->store, addr);
		_SNAPSHOT(diff_buf, CESK_DIFF_REUSE, addr, CESK_DIFF_REUSE_VALUE(1));
		cesk_store_release_rw(frame->store, addr);
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
					param.offset = CESK_OBJECT_FIELD_OFS(object, this->addrtab + j);
					uint32_t faddr = cesk_reloc_allocate(reloctab, frame->store, &param, 0);
					/* set the reloc flag */
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
						LOG_ERROR("can not maniuplate the reference counter at store address "PRSAddr"", faddr);
						return -1;
					}
					new_val->reloc = 1;
					cesk_store_release_rw(frame->store, faddr);
					_SNAPSHOT(diff_buf, CESK_DIFF_ALLOC, faddr, fvalue);
					_SNAPSHOT(inv_buf, CESK_DIFF_DEALLOC, faddr, NULL);
				}
			}
			else
			{
				if(bci_class_initialize(this->bcidata, bci_init_param, this->class.bci->class) < 0)
				{
					LOG_WARNING("failed to initlaize the built-in class");
				}
			}
			CESK_OBJECT_STRUCT_ADVANCE(this);
		}
		/* because the attach function auqire a writable pointer automaticly, 
		 * Therefore, you should release the address first */
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
		LOG_ERROR("can not find object "PRSAddr"", dst_addr);
		return -1;
	}

	if(value->type != CESK_TYPE_OBJECT)
	{
		LOG_ERROR("try to put a member from a non-object address %x", dst_addr);
		return -1;
	}
	
	cesk_object_t* object = value->pointer.object;

	const bci_class_t* builtin_class;
	void* builtin_data;

	uint32_t* paddr = cesk_object_get(object, clspath, fldname, &builtin_class, &builtin_data);

	if(NULL == paddr)
	{
		_SNAPSHOT(inv_buf, CESK_DIFF_STORE, dst_addr, value);
		if(bci_class_put_field(builtin_data, fldname, frame->regs[src_reg], frame->store, keep_old_vlaue, builtin_class) < 0)
		{
			LOG_ERROR("can not get field %s/%s", clspath, fldname);
			return -1;
		}
		int rc;
		if((rc = bci_class_get_relocation_flag(builtin_data, builtin_class)) < 0)
		{
			LOG_ERROR("can not get the relocation flag");
			return -1;
		}
		if(rc > 0) value->reloc = 1;
		_SNAPSHOT(inv_buf, CESK_DIFF_STORE, dst_addr, value);
	}
	else
	{
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

		_SNAPSHOT(inv_buf, CESK_DIFF_STORE, *paddr, value_set);  /* because we need refcount to be 1 */

		if(keep_old_vlaue || cesk_store_get_reuse(frame->store, *paddr) == 1)
		{
			/* this address is used by mutliple objects, so we can not discard old value */
			/* get the address of the value set */
			LOG_DEBUG("this is a reused object, just keep the old value");
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
					LOG_WARNING("can not incref at address "PRSAddr"", tmp_addr);
				}
				if(CESK_STORE_ADDR_IS_RELOC(tmp_addr))
					value_set->reloc = 1;
			}

			/* okay, append the value of registers to the set */
			if(cesk_set_merge(set, frame->regs[src_reg]) < 0)
			{
				LOG_ERROR("can not merge set");
				return -1;
			}

			if(cesk_set_get_reloc(set) > 0) value_set->reloc = 1;

			/* ok take the snapshot here */
			_SNAPSHOT(diff_buf, CESK_DIFF_STORE, *paddr, value_set);
		}
		else
		{
			/* we first incref */
			cesk_set_iter_t iter;
			if(NULL == cesk_set_iter(frame->regs[src_reg], &iter))
			{
				LOG_ERROR("can not aquire iterator for register %d", src_reg);
				return -1;
			}
			uint32_t tmp_addr;
			while(CESK_STORE_ADDR_NULL != (tmp_addr = cesk_set_iter_next(&iter)))
			{
				if(cesk_store_incref(frame->store, tmp_addr) < 0)
				{
					LOG_WARNING("can not incref at address "PRSAddr"", tmp_addr);
				}
			}

			cesk_store_release_rw(frame->store, *paddr);

			/* make a copy of source register */
			cesk_set_t* set = cesk_set_fork(frame->regs[src_reg]);
			if(NULL == set)
			{
				LOG_ERROR("can not copy the source register #%u", src_reg);
				return -1;
			}
			/* using this copy make a value */
			cesk_value_t* newval = cesk_value_from_set(set);
			if(NULL == set)
			{
				cesk_set_free(set);
				LOG_ERROR("can not make a new value");
				return -1;
			}
			if(cesk_set_get_reloc(newval->pointer.set) > 0) newval->reloc = 1;
			if(cesk_store_attach(frame->store, *paddr, newval) < 0)
			{
				LOG_ERROR("can not set new value to the field %s.%s", clspath, fldname);
				return -1;
			}
			_SNAPSHOT(diff_buf, CESK_DIFF_STORE, *paddr, newval);

		}

		/* release the write pointer */
		cesk_store_release_rw(frame->store, *paddr);
	}
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
	if(cesk_object_get_addr(object, classpath, fieldname, NULL, NULL, &faddr) < 0)
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
#define __PR(fmt, args...) do{\
	int pret = snprintf(p, buf + sz - p, fmt, ##args);\
	if(pret > buf + sz - p) pret = buf + sz - p;\
	p += pret;\
}while(0)
const char* cesk_frame_to_string(const cesk_frame_t* frame, char* buf, size_t sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	int i;
	__PR("[register ");
	for(i = 0; i < frame->size; i ++)
		__PR("(v%d %s) ", i, cesk_set_to_string(frame->regs[i], NULL, 0));
	__PR("] [store %s]", cesk_store_to_string(frame->store, NULL, 0));
	return buf;
}
void cesk_frame_print_debug(const cesk_frame_t* frame)
#if LOG_LEVEL >= 6
{
	int i;
	LOG_DEBUG("Registers");
	for(i = 0; i < frame->size; i ++)
		LOG_DEBUG("\tv%d\t%s", i, cesk_set_to_string(frame->regs[i], NULL, 0));
	LOG_DEBUG("Store");
	cesk_store_print_debug(frame->store);
}
#else
{}
#endif
#undef __PR
