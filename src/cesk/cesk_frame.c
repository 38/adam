#include <log.h>
#include <cesk/cesk_frame.h>
#include <cesk/cesk_store.h>
cesk_frame_t* cesk_frame_new(uint16_t size)
{
    if(0 == size)
    {
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
cesk_frame_t* cesk_frame_fork(cesk_frame_t* frame)
{
    if(NULL == frame) 
    {
        LOG_ERROR("invalid argument");
        return 0;
    }
    int i;
    cesk_frame_t* ret = (cesk_frame_t*)(sizeof(cesk_frame_t) + frame->size * sizeof(cesk_set_t*));
    if(NULL == ret)
    {
        LOG_ERROR("can not allocate memory");
        return NULL;
    }
    ret->size = frame->size;
    for(i = 0; i < frame->size; i ++)
    {
        ret->regs[i] = cesk_set_fork(ret->regs[i]);
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

int cesk_frame_equal(cesk_frame_t* first, cesk_frame_t* second)
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
#define BITAT(f,n) (((f)[n/8]&(1<<(n%8))) > 0)
static inline void _cesk_frame_store_dfs(uint32_t addr,cesk_store_t* store, uint8_t* f)
{
	if(CESK_STORE_ADDR_NULL == addr) return;
	/* constants do not need to collect */
	if(CESK_STORE_ADDR_IS_CONST(addr)) return;
    if(1 == BITAT(f, addr)) return;
    /* set the flag */
    f[addr/8] |= (1<<(addr%8));
    const cesk_value_t* val = cesk_store_get_ro(store, addr);
    if(NULL == val) return;
    cesk_set_iter_t iter_buf;
    cesk_set_iter_t* iter;
    uint32_t next_addr;
    cesk_object_struct_t* this;
    cesk_object_t* obj;
    int i;
    switch(val->type)
    {
#if 0
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
            /* atmoic types, no reference */
            break;
#endif
        case CESK_TYPE_SET:
            for(iter = cesk_set_iter(*(cesk_set_t**)val->data, &iter_buf);
                CESK_STORE_ADDR_NULL != (next_addr = cesk_set_iter_next(iter));)
                _cesk_frame_store_dfs(next_addr, store, f);
            break;
        case CESK_TYPE_OBJECT:
            obj = *(cesk_object_t**)val->data;
            this = obj->members;
            for(i = 0; i < obj->depth; i ++)
            {
                int j;
                for(j = 0; j < this->num_members; j ++)
                {
                    next_addr = this->valuelist[j];
                    _cesk_frame_store_dfs(next_addr, store, f);
                }
                this = (cesk_object_struct_t *)(this->valuelist + this->num_members);
            }
            break;
        case CESK_TYPE_ARRAY:
            LOG_INFO("fixme : array support");
    }
}
int cesk_frame_gc(cesk_frame_t* frame)
{
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
hashval_t cesk_frame_hashcode(cesk_frame_t* frame)
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
hashval_t cesk_frame_compute_hashcode(cesk_frame_t* frame)
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
/* this function is used for other function to do following things:
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
int cesk_frame_register_move(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_reg, uint32_t src_reg)
{
	if(NULL == frame || dst_reg >= frame->size || src_reg >= frame->size) 
	{
		LOG_WARNING("invalid instruction, invalid register reference");
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
	return 0;
}
int cesk_frame_register_load(cesk_frame_t* frame, dalvik_instruction_t* inst ,uint32_t dst_reg, uint32_t addr)
{
	if(NULL == frame || dst_reg >= frame->size || NULL == inst)
	{
		LOG_WARNING("bad instruction, invalid register reference");
		return -1;
	}

	/* clear the register first */
	if(cesk_frame_register_clear(frame, inst, dst_reg) < 0)
	{
		LOG_ERROR("can not clear the value in register %d", dst_reg);
		return -1;
	}

	if(cesk_set_push(frame->regs[dst_reg], addr) < 0)
	{
		LOG_ERROR("can not push address @%x to register %d", addr, dst_reg);
		return -1;
	}

	if(cesk_store_incref(frame->store, addr) < 0)
	{
		LOG_ERROR("can not decref for the cell @%x", addr);
		return -1;
	}

	return 0;
}
int cesk_frame_register_load_from_store(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr)
{
	if(NULL == frame || dest >= frame->size || NULL == inst)
	{
		LOG_WARNING("bad load_from_store command");
		return -1;
	}
	if(CESK_STORE_ADDR_NULL == src_addr)
	{
		LOG_DEBUG("from NULL store address, return empty set");
		return cesk_frame_register_clear(frame, inst, dest);
	}

	const cesk_value_t* value = cesk_store_get_ro(frame->store, src_addr);
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
	cesk_set_t* set = *(cesk_set_t**)value->data;

	/* delete the old value first */
	if(_cesk_frame_free_reg(frame, dest) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", dest);
		return -1;
	}

	/* copy the content of the set to the register */
	frame->regs[dest] = cesk_set_fork(set);

	if(NULL == frame->regs[dest])
	{
		LOG_ERROR("can not load the value @ %x to register %d", src_addr, dest);
		return -1;
	}

	/* inc ref to all the values */
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(frame->regs[dest], &iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", dest);
		return -1;
	}
	uint32_t p;
	while(CESK_STORE_ADDR_NULL != (p = cesk_set_iter_next(&iter)))
	{
		if(cesk_store_incref(frame->store, p) < 0)
		{
			LOG_ERROR("can not increase refcount for object @ %x", p);
			return -1;
		}
	}

	return 0;
}
int cesk_frame_register_append_from_store(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr)
{
	if(NULL == frame ||
	   NULL == inst ||
	   CESK_STORE_ADDR_NULL == dest ||
	   CESK_STORE_ADDR_NULL == src_addr)
	{
		LOG_ERROR("invalid arguments for append_from_store");
		return -1;
	}
	const cesk_value_t* value = cesk_store_get_ro(frame->store, src_addr);
	
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
	cesk_set_t* set = *(cesk_set_t**)value->data;

	cesk_set_iter_t iter;

	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator for set @ %x", src_addr);
		return -1;
	}

	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		if(cesk_frame_register_push(frame, inst, dest, addr) < 0)
		{
			LOG_WARNING("can not push value %x to register %d", addr, dest);
		}
	}
	return 0;
}
int cesk_frame_register_clear(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t reg)
{
	if(NULL == frame || reg >= frame->size)
	{
		LOG_WARNING("bad instruction, invalid register reference");
		return -1;
	}
	if(cesk_set_size(frame->regs[reg]) == 0)
	{
		/* the register is empty ? */
		return 0;
	}
	if(_cesk_frame_free_reg(frame,reg) < 0)
	{
		LOG_ERROR("can not free the old value of register %d", reg);
		return -1;
	}

	frame->regs[reg] = cesk_set_empty_set();
	if(frame->regs[reg] == NULL)
	{
		LOG_ERROR("can not create an empty set for register %d", reg);
		return -1;
	}
	
	return 0;
}
int cesk_frame_register_push(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t reg, uint32_t addr)
{
	if(NULL == frame || reg >= frame->size)
	{
		LOG_WARNING("bad instruction, invalid register number");
		return -1;
	}
	
	if(cesk_set_contain(frame->regs[reg], addr) == 0)
	{
		cesk_store_incref(frame->store, addr);
		return 0;
	}

	if(cesk_set_push(frame->regs[reg], addr) < 0)
	{
		LOG_ERROR("can not push value @ %x to register %d", addr, reg);
		return -1;
	}


	return 0;
}
int cesk_frame_store_object_get(cesk_frame_t* frame,dalvik_instruction_t* inst,  uint32_t dst_reg, uint32_t src_addr, const char* classpath, const char* field)
{
	if(NULL == frame || NULL == classpath || NULL == field || dst_reg >= frame->size || src_addr == CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	const cesk_value_t* value = cesk_store_get_ro(frame->store, src_addr);
	if(NULL == value)
	{
		LOG_ERROR("can not find object @%x", src_addr);
		return -1;
	}
	if(value->type != CESK_TYPE_OBJECT)
	{
		LOG_ERROR("try to get a member from a non-object address %x", src_addr);
		return -1;
	}

	cesk_object_t* object = *(cesk_object_t **)(value->data);

	uint32_t* paddr = cesk_object_get(object, classpath, field);
	if(NULL == paddr)
	{
		LOG_ERROR("can not find the field %s/%s", classpath, field);
		return -1;
	}
	/* load the value */

	//return cesk_frame_register_load(frame, inst ,dst_reg, *paddr);
	return cesk_frame_register_load_from_store(frame, inst, dst_reg, *paddr);
}
int cesk_frame_store_object_put(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_addr, const char* classpath, const char* field, uint32_t src_reg)
{
	if(NULL == frame || NULL == classpath || NULL == field || src_reg >= frame->size || dst_addr == CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	
	cesk_value_t* value = cesk_store_get_rw(frame->store, dst_addr);
	
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
	
	cesk_object_t* object = *(cesk_object_t **)(value->data);

	uint32_t* paddr = cesk_object_get(object, classpath, field);

	if(NULL == paddr)
	{
		LOG_ERROR("can not get field %s/%s", classpath, field);
		return -1;
	}

	cesk_value_t* value_set = NULL;

	if(*paddr == CESK_STORE_ADDR_NULL)
	{
		LOG_DEBUG("this is a new field, create an empty set for it");
		/* if no set associated with the field */
		*paddr = cesk_store_allocate(&frame->store, inst, dst_addr, field);
		if(*paddr == CESK_STORE_ADDR_NULL)
		{
			LOG_ERROR("can not allocate new store address for the value set");
			return -1;
		}
		value_set = cesk_value_empty_set();
		if(NULL == value_set)
		{
			LOG_ERROR("can not create an empty set for field %s/%s", classpath, field);
			return -1;
		}
		if(cesk_store_attach(frame->store, *paddr, value_set) < 0)
		{
			LOG_ERROR("can not attach empty set to address %x", *paddr);
			return -1;
		}
		/* increase the refcount to the new address */
		if(cesk_store_incref(frame->store, *paddr) < 0)
		{
			LOG_ERROR("can not incref to addr@%x", *paddr);
			return -1;
		}
	}
	else if(cesk_store_is_reuse(frame->store, dst_addr) == 1)
	{
		/* this address is used by mutliple objects, so we can not dicard old value */
		/* get the address of the value set */
		LOG_DEBUG("this is a reused object, just keep the old value");
		value_set = cesk_store_get_rw(frame->store, *paddr);
		if(NULL == value_set)
		{
			LOG_ERROR("can not find the value set for field %s/%s", classpath, field);
			return -1;
		}
	}
	else
	{
		/* this address is used by single object, so we will lose the old value after we write the field */
		LOG_DEBUG("this address is used by signle object, drop the old value");
		value_set = cesk_value_empty_set();
		if(NULL == value_set)
		{
			LOG_ERROR("can not create an empty set for field %s/%s", classpath, field);
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
	cesk_set_t* set = *(cesk_set_t**)value_set->data;

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
		if(cesk_store_incref(frame->store, tmp_addr) < 0)
		{
			LOG_WARNING("can not incref at address @%x", tmp_addr);
		}
	}

	/* okay, append the value of registers to the set */
	if(cesk_set_join(set, frame->regs[src_reg]) < 0)
	{
		LOG_ERROR("can not merge set");
		return -1;
	}
	/* release the write pointer */
	cesk_store_release_rw(frame->store, *paddr);
	cesk_store_release_rw(frame->store, dst_addr);
	return 0;
}
uint32_t cesk_frame_store_new_object(cesk_frame_t* frame, const dalvik_instruction_t* inst, const char* classpath)
{
	if(NULL == frame || NULL == inst || NULL == classpath)
	{
		LOG_ERROR("invalid arguments");
		return CESK_STORE_ADDR_NULL;
	}
	LOG_DEBUG("creat new object from class %s", classpath);
	/* allocate address */
	uint32_t addr = cesk_store_allocate(&frame->store, inst, CESK_STORE_ADDR_NULL, NULL); 

	if(CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("can not allocate store address for object");
		return CESK_STORE_ADDR_NULL;
	}
	
	const cesk_value_t* value;

	if((value = cesk_store_get_ro(frame->store, addr)) != NULL)
	{
		/* check validity of the address */
		if(value->type != CESK_TYPE_OBJECT)
		{
			LOG_ERROR("can not attach an object to a non-object address");
			return CESK_STORE_ADDR_NULL;
		}
		cesk_object_t* object = *(cesk_object_t**)value->data;
		if(NULL == object)
		{
			LOG_ERROR("invalid value");
			return CESK_STORE_ADDR_NULL; 
		}
		if(cesk_object_classpath(object) != classpath)
		{
			LOG_ERROR("address %x is for class %s but the new object is a instance of %s", 
					  addr, 
					  cesk_object_classpath(object), 
					  classpath);
			return CESK_STORE_ADDR_NULL;
		}
		/* reuse */
		if(cesk_store_set_reuse(frame->store, addr) < 0)
		{
			LOG_ERROR("can not reuse the address @ %x", addr);
			return CESK_STORE_ADDR_NULL;
		}

	}
	else
	{
		/* a fresh address, use it */
		cesk_value_t* new_val = cesk_value_from_classpath(classpath);
		if(NULL == new_val)
		{
			LOG_ERROR("can not create new object from class %s", classpath);
			return CESK_STORE_ADDR_NULL;
		}
		if(cesk_store_attach(frame->store, addr, new_val) < 0)
		{
			LOG_ERROR("failed to attach new object to address %x", addr);
			return CESK_STORE_ADDR_NULL;
		}
		/* because the attach function auqire a writable pointer automaticly, 
		 * Therefore, you should release the address first
		 */
		cesk_store_release_rw(frame->store, addr);
		
	}
	return addr;
}
int cesk_frame_store_array_get(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_addr, uint32_t index, uint32_t src_reg)
{
	LOG_TRACE("fixme : array support, get an element from the array");
	return 0;
}
int cesk_frame_store_array_put(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t index, uint32_t dst_reg, uint32_t src_reg)
{
	LOG_TRACE("fixme : array support, put an emement to the array");
	return 0;
}
uint32_t cesk_frame_store_new_array(cesk_frame_t* frame, const dalvik_instruction_t* inst)
{
	LOG_TRACE("fixme : array support");
	return CESK_STORE_ADDR_NULL;
}
