#include <string.h>

#include <log.h>

#include <cesk/cesk_store.h>

#define HASH_INC(addr,val) (addr * MH_MULTIPLY + cesk_value_hashcode(val))
#define HASH_CMP(addr,val) (addr * MH_MULTIPLY + cesk_value_compute_hashcode(val))
/* make a copy of a store block */
static inline cesk_store_block_t* _cesk_store_block_fork(cesk_store_block_t* block)
{
    /* copy the store block */
    cesk_store_block_t* new_block = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
    if(NULL == new_block) 
    {
        LOG_ERROR("can not allocate memory for new block");
        return NULL;
    }
    memcpy(new_block, block, CESK_STORE_BLOCK_SIZE);
    new_block->refcnt = 0;
    /* increase the reference counter of the vlaues in the block */
    int i;
    for(i = 0; i < CESK_STORE_BLOCK_NSLOTS; i ++)
        if(new_block->slots[i].value != NULL)
            cesk_value_incref(new_block->slots[i].value);
    return new_block;
}
/* decref all members of the set before free it */
static inline int _cesk_store_free_set(cesk_store_t* store, cesk_set_t* set)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire iterator for the set");
		return -1;
	}
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		/* decref for all its references */
		if(cesk_store_decref(store, addr) < 0)
		{
			LOG_WARNING("can not decref at address %x", addr);
		}
	}
	return 0;
}
/* decref all members of the object before free it */
static inline int _cesk_store_free_object(cesk_store_t* store, cesk_object_t* object)
{
	if(NULL == object)
	{
		LOG_ERROR("can not dispose a NULL object");
		return -1;
	}
	cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		for(j = 0; j < this->num_members; j ++)
		{
			if(cesk_store_decref(store, this->valuelist[j]) < 0)
			{
				LOG_WARNING("can not decref at address %x", this->valuelist[j]);
			}
		}
		this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
	}
	return 0;
}
/* make an address empty, but do not affect the refcnt */
static inline int _cesk_store_swipe(cesk_store_t* store, cesk_store_block_t* block, uint32_t addr)
{
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_value_t* value = block->slots[ofs].value;
	/* release the address */
	block->slots[ofs].value = NULL; 
	
	/* update the hashcode */
	store->hashcode ^= HASH_INC(addr, value);
	
	/* decref of it's refernces */
	int rc = -1;
	switch(value->type)
	{
		case CESK_TYPE_OBJECT:
			rc = _cesk_store_free_object(store, value->pointer.object);
			break;
		case CESK_TYPE_SET:
			rc = _cesk_store_free_set(store, value->pointer.set);
			break;
		case CESK_TYPE_ARRAY:
			LOG_TRACE("fixme: array support: decref of its reference here");
	}
	if(rc < 0)
	{
		LOG_WARNING("can not dispose the object");
	}
	/* unreference from this frame */
	cesk_value_decref(value);
	return 0;
}
/* the function for the addressing hash code */
static inline hashval_t _cesk_store_address_hashcode(const dalvik_instruction_t* inst, uint32_t parent, const char* field)
{
    uint32_t idx;
	
	if(NULL == inst)
	{
		LOG_ERROR("invalid instruction address");
		return 0;
	}
	
	/* read the instruction annotation for index */
	//dalvik_instruction_read_annotation(inst, &idx, sizeof(idx));
	idx = dalvik_instruction_get_index(inst);

	return (idx * idx * MH_MULTIPLY + parent * 100007 * MH_MULTIPLY + ((uintptr_t)field&(hashval_t)~0));

}
/* get a block in a store and prepare to write */
static inline cesk_store_block_t* _cesk_store_getblock_rw(cesk_store_t* store, uint32_t addr)
{
    if(NULL == store || CESK_STORE_ADDR_NULL == addr) 
    {
        LOG_ERROR("invailid argument");
        return NULL;
    }
    uint32_t b_idx = addr / CESK_STORE_BLOCK_NSLOTS;
    if(b_idx >= store->nblocks)
    {
        LOG_ERROR("out of memory");
        return NULL;
    }
    cesk_store_block_t* block = store->blocks[b_idx];
    if(block->refcnt > 1)
    {
        LOG_DEBUG("more than one store are using this block, fork it");
        cesk_store_block_t* newblock = _cesk_store_block_fork(block);
        if(NULL == newblock)
        {
            LOG_ERROR("can not fork the block");
            return NULL;
        }
        newblock->refcnt ++;
        block->refcnt --;
        store->blocks[b_idx] = newblock;
        block = newblock;
    }
    return block;
}
cesk_store_t* cesk_store_empty_store()
{
   cesk_store_t* ret = (cesk_store_t*)malloc(sizeof(cesk_store_t));
   if(NULL == ret)
   {
       LOG_ERROR("can not allocate memory for empty store");
       return NULL;
   }
   ret->nblocks = 0;
   ret->num_ent = 0;
   ret->hashcode = CESK_STORE_EMPTY_HASH;
   return ret;
}

cesk_store_t* cesk_store_fork(const cesk_store_t* store)
{
    int i;
    size_t size = sizeof(cesk_store_t) + sizeof(cesk_store_block_t*) * store->nblocks;
    cesk_store_t* ret = (cesk_store_t*)malloc(size);
    if(NULL == ret)
    {
        LOG_ERROR("can not allocate memory for new store");
        return NULL;
    }
    memcpy(ret, store, size);
    /* increase refrence counter of all blocks */
    for(i = 0; i < ret->nblocks; i ++)
        ret->blocks[i]->refcnt++;
    LOG_DEBUG("a store of %d entities is being forked, %zu bytes copied", ret->num_ent, size);
    return ret;
}
const cesk_value_t* cesk_store_get_ro(const cesk_store_t* store, uint32_t addr)
{
    uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
    if(block_idx >= store->nblocks) 
    {
        LOG_ERROR("invalid address out of space");
        return NULL;
    }
    const cesk_store_block_t* block = store->blocks[block_idx];
    if(NULL == block)
    {
        LOG_ERROR("opps, it should not happen");
        return NULL;
    }
    return block->slots[offset].value;
}
int cesk_store_is_reuse(const cesk_store_t* store, uint32_t addr)
{
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	const cesk_store_block_t* block = store->blocks[block_idx];
	if(NULL == block)
	{
		LOG_ERROR("ooops, this should not happen");
		return -1;
	}
	return block->slots[offset].reuse;
}
int cesk_store_set_reuse(cesk_store_t* store, uint32_t addr)
{
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset = addr / CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	cesk_store_block_t* block = store->blocks[block_idx];
	if(NULL == block)
	{
		LOG_ERROR("what's wrong?");
		return -1;
	}
	block->slots[offset].reuse = 1;
	return 0;
}
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr)
{
    uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
    if(block_idx >= store->nblocks) 
    {
        LOG_ERROR("invalid address out of space");
        return NULL;
    }
    cesk_store_block_t* block = store->blocks[block_idx];
    if(NULL == block)
    {
        LOG_ERROR("opps, it should not happen");
        return NULL;
    }
    cesk_value_t* val = block->slots[offset].value;
    if(NULL == val)
    {
        LOG_TRACE("is this an empty slot?");
        return NULL;
    }
    /*
     * Because the if the block is refered by more than one store,
     * It's always nesscesary to copy the object 
     * Because the object is also refered by more than one store
     * In addition the block itself is to be modified, so
     * We have to fork the block first in this situation
     */
    if(block->refcnt > 1) 
    {
        LOG_DEBUG("this block is refered by other frame, so fork it before modify");
        cesk_store_block_t* newblock = _cesk_store_block_fork(block);
        if(NULL == newblock)
        {
            LOG_ERROR("cannot fork the frame, aborting");
            return NULL;
        }
        store->blocks[block_idx] = newblock;
        block->refcnt --;
        newblock->refcnt ++;
        block = newblock;
    }
    if(val->refcnt > 1)
    {
        LOG_DEBUG("this value is refered by other frame block, so fork it first");
        cesk_value_t* newval = cesk_value_fork(val);
        if(NULL == newval)
        {
            LOG_ERROR("error during fork the value, aborting copy");
            return NULL;
        }

        block->slots[offset].value = newval;

        cesk_value_decref(val);
        cesk_value_incref(newval);

        val = newval;
    }
    /* when a rw pointer is auquired, the hashcode is ready to update.
     * After finish updating, you should call the function release the 
     * value and update the hashcode 
     */
    store->hashcode ^= HASH_INC(addr, val);
    val->write_status = 1;
    return val;
}
void cesk_store_release_rw(cesk_store_t* store, uint32_t addr)
{
    uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
    if(block_idx >= store->nblocks) 
    {
        LOG_ERROR("invalid address out of space");
        return;
    }
    cesk_store_block_t* block = store->blocks[block_idx];
    if(NULL == block)
    {
        LOG_ERROR("opps, it should not happen");
        return;
    }
    cesk_value_t* val = block->slots[offset].value;
    if(val->write_status == 0)
    {
        LOG_WARNING("there's no writable pointer accociated to this status");
        return;
    }
    val->write_status = 0;
    /* update the hashcode */
    store->hashcode ^= HASH_INC(addr, val);
}
/* just for debug purpose */
hashval_t cesk_store_compute_hashcode(const cesk_store_t* store)
{
	uint32_t ret = CESK_STORE_EMPTY_HASH;
	int i,j;
	for(i = 0; i < store->nblocks; i ++)
	{
		for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
		{
			if(NULL != store->blocks[i]->slots[j].value && store->blocks[i]->slots[j].value->write_status == 0)
			{
				uint32_t addr = i * CESK_STORE_BLOCK_NSLOTS + j;
				ret ^= HASH_CMP(addr, store->blocks[i]->slots[j].value);
			}
		}
	}
	return ret;
}
uint32_t cesk_store_allocate(cesk_store_t** p_store, const dalvik_instruction_t* inst, uint32_t parent, const char* field)
{
    cesk_store_t* store = *p_store;
    uint32_t idx;
	idx = dalvik_instruction_get_index(inst);
    uint32_t  init_slot = _cesk_store_address_hashcode(inst, parent, field)  % CESK_STORE_BLOCK_NSLOTS;
    uint32_t  slot = init_slot;
    /* here we perform a quadratic probing inside each block
     * But we do not jump more than 5 times in one block
     * After tried 5 jumps, the allocator should allocate a 
     * new store block anyway.
     */
    int empty_block = -1;
    int empty_offset = -1;
    int equal_block = -1;
    int equal_offset = -1;
    int attempt;
    for(attempt = 0; attempt < CESK_STORE_ALLOC_ATTEMPT; attempt ++)
    {
        int block;
        LOG_DEBUG("attempt #%d @%d for instruction %d", attempt, slot, idx);
        for(block = 0; block < store->nblocks; block ++)
        {
            if(store->blocks[block]->slots[slot].value == NULL && empty_offset == -1)
            {
                LOG_DEBUG("find an empty slot @ (block = %d, offset = %d)", block, slot);
                empty_block = block;
                empty_offset = slot;
            }
            if(store->blocks[block]->slots[slot].value != NULL && 
               store->blocks[block]->slots[slot].idx == idx && 
			   store->blocks[block]->slots[slot].parent == parent &&
			   store->blocks[block]->slots[slot].field == field)
            {
                LOG_DEBUG("find the equal slot @ (block = %d, offset = %d)", block, slot);
                equal_block = block;
                equal_offset = slot;
            }
            if(equal_offset != -1) break;
        }
        if(equal_offset != -1) break;
        slot = (slot * slot + 100007 * slot + 634567) % CESK_STORE_BLOCK_NSLOTS;
    }
    /* After look for empty slot / equal slot */
    if(empty_offset == -1 && equal_offset == -1)
    {
        /* no empty slot, no equal slot, allocate a new page */
        LOG_DEBUG("can not allocate a store entry for this object, allocate a new block. current_size = %d, num_ent = %d", 
                   store->nblocks, store->num_ent);
        store = realloc(store, sizeof(cesk_store_t) + sizeof(cesk_store_block_t*) * (store->nblocks + 1));
        if(NULL == store)
        {
            LOG_ERROR("can not increase the size of store");
            return CESK_STORE_ADDR_NULL;
        }
        (*p_store) = store;
        store->blocks[store->nblocks] = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
        if(NULL ==  store->blocks[store->nblocks])
        {
            LOG_ERROR("can not allocate a new page for the block");
            return CESK_STORE_ADDR_NULL;
        }
        memset(store->blocks[store->nblocks], 0, CESK_STORE_BLOCK_SIZE);
        store->blocks[store->nblocks]->refcnt ++;
        empty_block = store->nblocks ++;
        empty_offset = init_slot;   /* use the init_slot, so that we can locate it faster */
    }
    if(equal_offset == -1)
    {
        /* if there's no equal entry */
        if(empty_offset != -1)
        {
            LOG_DEBUG("allocate %lu (block=%d, offset = %d) for instruction %d", 
                       empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset, empty_block, empty_offset, idx);
            store->blocks[empty_block]->slots[empty_offset].idx = idx;
			store->blocks[empty_block]->slots[empty_offset].parent = parent;
			store->blocks[empty_block]->slots[empty_offset].field = field;
            return empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset;
        }
        else
        {
            LOG_ERROR("neither empty slot or equal entry has been found in the store, allocation failed");
            return CESK_STORE_ADDR_NULL;
        }
    }
    else
    {
        /* some equal entry */
        LOG_DEBUG("reuse %lu (block = %d, offset = %d for instruction %d",
                   equal_block * CESK_STORE_BLOCK_NSLOTS + equal_offset, equal_block, equal_offset, idx);
        store->blocks[equal_block]->slots[equal_offset].reuse = 1;   
        return equal_block * CESK_STORE_BLOCK_NSLOTS  + equal_offset;
    }
}
int cesk_store_attach(cesk_store_t* store, uint32_t addr, cesk_value_t* value)
{
    if(NULL == store || CESK_STORE_ADDR_NULL == addr)
    {
        LOG_ERROR("invalid arguments");
        return -1;
    }
    uint32_t block = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t offset = addr % CESK_STORE_BLOCK_NSLOTS;
    if(block >= store->nblocks)
    {
        LOG_ERROR("out of memory");
        return -1;
    }
    if(value == store->blocks[block]->slots[offset].value)
    {
        LOG_TRACE("value is already attached to this address");
        return 0;
    }
	/* just aquire a writable pointer of this block */
	cesk_store_block_t* block_rw = _cesk_store_getblock_rw(store, addr);
	/*
    if(store->blocks[block]->refcnt > 1)
    {
        cesk_store_block_t* newblock = _cesk_store_block_fork(store->blocks[block]);
        if(NULL == newblock)
        {
            LOG_ERROR("can't fork store");
            return -1;
        }
        store->blocks[block]->refcnt --;
        newblock->refcnt ++;
        store->blocks[block] = newblock;
    }*/
    if(block_rw->slots[offset].value == NULL && value != NULL) 
    {
        block_rw->num_ent ++;
        store->num_ent ++;
    }
    else if(block_rw->slots[offset].value != NULL && value == NULL)
    {
        block_rw->num_ent --;
        store->num_ent --;
    }
    if(NULL != block_rw->slots[offset].value)
    {
		/* update the hashcode */ 
		//store->hashcode ^= addr * MH_MULTIPLY + cesk_value_hashcode(store->blocks[block]->slots[offset].value);
        //store->hashcode ^= HASH_INC(addr, store->blocks[block]->slots[offset].value);
		/* Because cesk_store_decref also update the value, so we do not update the value here */
		/* clean the old object */
		/*block_rw->slots[offset].refcnt = 1;
		cesk_store_decref(store, addr);*/   /* dirty hack, using this way to trigger the clean procedure */
		/* we can not reduce the refcnt here, because the address is actually not defered from others
		 * So we should swipe the old value out, but do not affect the refcnt here
		 */
		_cesk_store_swipe(store, block_rw, addr);
		/* dereference to previous object */
		/* it seems that the cesk_store_decref will call cesk_value_decref */
		/* cesk_value_decref(store->blocks[block]->slots[offset].value);*/

    }
    if(value)
    {
        /* reference to new value */
        cesk_value_incref(value);
		value->write_status = 1;
		/*store->hashcode ^= (addr * MH_MULTIPLY + cesk_value_hashcode(value));*/
		/* we do not update new hash code here, that means we should use cesk_store_release_rw function
		 * After we finish modifiying the store */
    }
	block_rw->slots[offset].value = value;
	block_rw->slots[offset].reuse = 0;  /* attach to a object, all previous object is lost */
    return 0;
}
/* TODO: Question, once an adress is set to reusing, is there any time for the address to clear this bit ? */
void cesk_store_free(cesk_store_t* store)
{
    int i;
    if(NULL == store) return;
    for(i = 0; i < store->nblocks; i ++)
    {
        if(store->blocks[i]->refcnt > 0)
            store->blocks[i]->refcnt --;
        if(store->blocks[i]->refcnt == 0)
        {
            int j;
            for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
                if(store->blocks[i]->slots[j].value != NULL)
                    cesk_value_decref(store->blocks[i]->slots[j].value);
            free(store->blocks[i]);
        }
    }
    free(store);
}

int cesk_store_incref(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_IS_CONST(addr))
	{
		return 0;
	}
    uint32_t b_ofs = addr % CESK_STORE_BLOCK_NSLOTS;
    cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
    if(NULL == block)
        return -1;
    if(block->slots[b_ofs].value != NULL)
    {
        return ++block->slots[b_ofs].refcnt;
    }
    else
    {
        LOG_ERROR("the object @%xS is alread dead", addr);
        return -1;
    }
}
int cesk_store_decref(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_IS_CONST(addr))
	{
		return 0;
	}
    uint32_t idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
    if(idx >= store->nblocks)
    {
        LOG_ERROR("out of memory");
        return -1;
    }
    if( store->blocks[idx]->slots[ofs].value == NULL)
    {
        return 0;
    }
    cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
    if(NULL == block)
        return -1;

    /* decrease the counter */
    if(block->slots[ofs].refcnt > 0) 
        block->slots[ofs].refcnt --;
	else
		LOG_WARNING("found an living object with 0 refcnt at address %x", addr);

    if(0 == block->slots[ofs].refcnt)
    {
        LOG_TRACE("value @0x%x is dead, swipe it out", addr);
		_cesk_store_swipe(store, block, addr);
        block->num_ent --;
        store->num_ent --;
    }
    return block->slots[ofs].refcnt;
}


int cesk_store_equal(const cesk_store_t* first, const cesk_store_t* second)
{
    if(NULL == first || NULL == second) return first != second;
    if(first->nblocks != second->nblocks) return 0;   /* because there must be an occupied address that another store does not have */
    if(cesk_store_hashcode(first) != cesk_store_hashcode(second)) return 0;
    int i;
    for(i = 0; i < first->nblocks; i ++)
    {
        int j;
        for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
            if(cesk_value_equal(first->blocks[i]->slots[j].value,
                                second->blocks[i]->slots[j].value) == 0)
                return 0;
    }
    return 1;
}
uint32_t cesk_store_const_addr_from_operand(const dalvik_operand_t* operand)
{
    if(NULL == operand) return CESK_STORE_ADDR_NULL;
    if(!operand->header.info.is_const)
    {
        LOG_ERROR("can not create a value from a non-constant operand");
        return CESK_STORE_ADDR_NULL;
    }
    int intval = 0;
    switch(operand->header.info.type)
    {
        case DVM_OPERAND_TYPE_LONG:
            intval = operand->payload.int64;
            goto num;
        case DVM_OPERAND_TYPE_SHORT:
            intval = operand->payload.int16;
            goto num;
        case DVM_OPERAND_TYPE_INT:
            intval = operand->payload.int32;
            goto num;
        case DVM_OPERAND_TYPE_BYTE:
            intval = operand->payload.int8;
            goto num;
        case DVM_OPERAND_TYPE_DOUBLE:
            if(operand->payload.real64 < -1e-20)
                intval = -1;
            else if(operand->payload.real64 > 1e-20)
                intval = 1;
            else 
                intval = 0;
            goto num;
        case DVM_OPERAND_TYPE_FLOAT:
            if(operand->payload.real32 < -1e-20)
                intval = -1;
            else if(operand->payload.real32 > 1e-20)
                intval = 1;
            else 
                intval = 0;
            goto num;
        case DVM_OPERAND_TYPE_CHAR:
            intval = operand->payload.int8;
num:
            if(intval > 0)
                return CESK_STORE_ADDR_POS;
            else if(intval < 0)
                return CESK_STORE_ADDR_NEG;
            else 
                return CESK_STORE_ADDR_ZERO;
        case DALVIK_TYPECODE_BOOLEAN:
            if(operand->payload.uint8)
                return CESK_STORE_ADDR_TRUE;
            else
                return CESK_STORE_ADDR_FALSE;
            break;
        default:
            LOG_ERROR("can not create value for the type");
			return CESK_STORE_ADDR_NULL;
    }
	return CESK_STORE_ADDR_NULL;
}
uint32_t cesk_store_get_refcnt(const cesk_store_t* store, uint32_t addr)
{
    uint32_t idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	return store->blocks[idx]->slots[ofs].refcnt;
}
int cesk_store_clear_refcnt(cesk_store_t* store, uint32_t addr)
{
    uint32_t idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	if(store->nblocks <= idx) 
	{
		LOG_ERROR("invalid address @0x%x", addr);
		return -1;
	}
	store->blocks[idx]->slots[ofs].refcnt = 0;
	return 0;
}
