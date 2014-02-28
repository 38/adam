#include <string.h>

#include <log.h>

#include <cesk/cesk_store.h>

#define HASH_INC(addr,val) (addr * MH_MULTIPLY + cesk_value_hashcode(val))
#define HASH_CMP(addr,val) (addr * MH_MULTIPLY + cesk_value_compute_hashcode(val))
/** @brief make a copy of a store block, but do not set the refcnt */
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
/** @brief set refcnt befofe actual deletion , decref all members of the set before free it */
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
/** @brief decref all members of the object before free it */
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
	
	/* decref of its refernces */
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
/** @brief the function for the addressing hash code */
static inline hashval_t _cesk_store_address_hashcode(const dalvik_instruction_t* inst, uint32_t parent, uint32_t field_ofs)
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

	return (idx * idx * MH_MULTIPLY + parent * 100007 * MH_MULTIPLY + (field_ofs * MH_MULTIPLY * MH_MULTIPLY));

}
/** @brief get a block in a store and prepare to write */
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
	/* if the block is used by more than one store, the block is not writable, so make a copy of 
	 * the block and replace the old block with new  copy */
    if(block->refcnt > 1)
    {
        LOG_DEBUG("more than one store are using this block, fork it");
        cesk_store_block_t* newblock = _cesk_store_block_fork(block);
        if(NULL == newblock)
        {
            LOG_ERROR("can not fork the block");
            return NULL;
        }
        newblock->refcnt = 1;
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
cesk_value_const_t* cesk_store_get_ro(const cesk_store_t* store, uint32_t addr)
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
    return (cesk_value_const_t*)block->slots[offset].value;
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
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
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
    uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	cesk_value_t* val = block->slots[offset].value;
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
    cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
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
uint32_t cesk_store_allocate(cesk_store_t** p_store, const dalvik_instruction_t* inst, uint32_t parent, uint32_t field_ofs)
{
    cesk_store_t* store = *p_store;
    uint32_t idx;
	idx = dalvik_instruction_get_index(inst);
    uint32_t  init_slot = _cesk_store_address_hashcode(inst, parent, field_ofs)  % CESK_STORE_BLOCK_NSLOTS;
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
			   store->blocks[block]->slots[slot].field == field_ofs)
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
	/* if there's no equal entry */
    if(equal_offset == -1)
    {
        if(empty_offset != -1)
        {
            LOG_DEBUG("allocate %lu (block=%d, offset = %d) for instruction %d", 
                       empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset, empty_block, empty_offset, idx);
            store->blocks[empty_block]->slots[empty_offset].idx = idx;
			store->blocks[empty_block]->slots[empty_offset].parent = parent;
			store->blocks[empty_block]->slots[empty_offset].field = field_ofs;
			store->blocks[empty_block]->slots[empty_offset].reuse = 0;
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
		_cesk_store_swipe(store, block_rw, addr);
    if(value)
    {
        /* reference to new value */
        cesk_value_incref(value);
		value->write_status = 1;
		/* we do not update new hash code here, that means we should use cesk_store_release_rw function
		 * After we finish modifiying the store */
    }
	block_rw->slots[offset].value = value;
	block_rw->slots[offset].reuse = 0;  /* attach to a object, all previous object is lost */
    return 0;
}
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
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
    cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
    if(NULL == block)
	{
		LOG_ERROR("can not aquire writable pointer to block");
        return -1;
	}

    if(block->slots[ofs].value == NULL)
    {
		LOG_DEBUG("the value is empty");
        return 0;
    }

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
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
	{
		LOG_ERROR("can not get a writable pointer to the block");
		return -1;
	}
	block->slots[ofs].refcnt = 0;
	return 0;
}
/** @brief adjust the structure of the destination , because the slot number of
 *         destination set should not be less than the source set
 *
 *         If some block is missing the destination store use the source block 
 *         directly
 *  @return the adjusted destination store (address may different from the input address)
 */
static inline cesk_store_t* _cesk_store_merge_adjust(cesk_store_t* dest, const cesk_store_t* sour)
{
	if(NULL == dest || NULL == sour)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	/* if source is contains more blocks, add those blocks to the destination first */
	if(sour->nblocks > dest->nblocks)
	{
		LOG_DEBUG("address space of source store is larger than that of destination store, so create new blocks in destination store");
		int prev_nblocks = dest->nblocks;
		dest->nblocks = sour->nblocks;
		dest = (cesk_store_t*)realloc(dest, sizeof(cesk_store_t) + sour->nblocks * sizeof(cesk_store_block_t*));
		if(NULL == dest)
		{
			LOG_ERROR("can not realloc");
			return NULL;
		}
		int i;
		/* add those block to the desination store first */
		for(i = prev_nblocks; i < sour->nblocks; i ++)
		{
			dest->blocks[i] = sour->blocks[i];
			/* of course, the refcnt increased */
			sour->blocks[i]->refcnt ++;
		}
		LOG_DEBUG("destination store has been resized from %d blocks to %d blocks", prev_nblocks, dest->nblocks);
	}
	return dest;
}
/** @brief merge two object, in the given store. In this function, we assume
 *         that all object which should be add to the destination has been 
 *         added alread. Therefore, the only resonsibility is to intialize the
 *         value set of the object. 
 *         Because all object has been placed now, so there's no possibility of
 *         a newly created set conflict to an object.
 *  @param sour the source store
 *  @param dest the destination store
 *  @param sour_addr the source address
 *  @param dest_addr the destination address
 *  @param reloc the relocation table 
 *  @return -1 if error
 */
static inline int _cesk_store_merge_object(
		cesk_store_t* dest,
		uint32_t dest_addr,
		const cesk_store_t* sour,
		uint32_t sour_addr,
		cesk_reloc_table_t* reloc)
{
	if(NULL == dest || NULL == sour ||
	   CESK_STORE_ADDR_NULL == dest_addr ||
	   CESK_STORE_ADDR_NULL == sour_addr)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	/* first get the pointer to the value */
	cesk_value_t*       dest_val = cesk_store_get_rw(dest, dest_addr);
	cesk_value_const_t* sour_val = cesk_store_get_ro(sour, sour_addr);
	if(NULL == dest_val || NULL == sour_val)
	{
		LOG_ERROR("can not aquire pointer to destination adress or source address");
		goto ERROR;
	}
	/* check if it's an object value */
	if(dest_val->type != CESK_TYPE_OBJECT || sour_val->type != CESK_TYPE_OBJECT)
	{
		LOG_ERROR("can not merge two address that is not object");
		goto ERROR;
	}
	/* okay, then get the object */
	cesk_object_t* dest_obj = dest_val->pointer.object;
	const cesk_object_t* sour_obj = sour_val->pointer.object;
	if(NULL == dest_obj || NULL == sour_obj)
	{
		LOG_ERROR("one of the object is NULL?");
		goto ERROR;
	}

	/* We should check the compatibility of two object. Because after the relocation, different instruction would occupy 
	 * different store address. So that, the actual class path of two object should be same */
	if(cesk_object_classpath(sour_obj) != cesk_object_classpath(dest_obj))
	{
		LOG_ERROR("can not merge two object with different class path");
		LOG_DEBUG("class path of source object: %s", cesk_object_classpath(sour_obj));
		LOG_DEBUG("class path of destination object: %s", cesk_object_classpath(dest_obj));
		goto ERROR;
	}

	/* merge two object */
	cesk_object_struct_t* dest_struct = dest_obj->members;
	const cesk_object_struct_t* sour_struct = sour_obj->members;
	
	/* because we need to allocate set during the merge, therefore 
	 * we should know which instruction creates the object 
	 */
	const dalvik_instruction_t* inst = NULL;
	uint32_t block_idx = dest_addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset    = dest_addr % CESK_STORE_BLOCK_NSLOTS;
	inst = dalvik_instruction_get(dest->blocks[block_idx]->slots[offset].idx);

	/* tanverse the member list */
	int i;
	for(i = 0; i < sour_obj->depth; i ++)
	{
		int j;
		/* for each object structure */
		for(j = 0; j < dest_obj->depth; j ++)
		{
			/* here we loop for each field, get the value set first */
			uint32_t sour_set_addr = sour_struct->valuelist[j];
			uint32_t dest_set_addr = dest_struct->valuelist[j];
			cesk_value_t* setval = NULL;
			/* if the dest set has not been set up yet. In fact there's one possible value: null pointer */
			if(CESK_STORE_ADDR_NULL == dest_set_addr)
			{
				/* allocate a new address for the value set */
				dest_set_addr = cesk_store_allocate(&dest, inst, dest_addr, CESK_OBJECT_FIELD_OFS(dest_obj, dest_struct->valuelist + j));
				if(CESK_STORE_ADDR_NULL == dest_set_addr)
				{
					LOG_ERROR("can not allocate memory for new field");
					goto ERROR;
				}
				/* put default ZERO */
				setval = cesk_value_empty_set();
				if(NULL == setval)
				{
					LOG_ERROR("can not create an empty set for the new field");
					goto ERROR;
				}
				if(cesk_set_push(setval->pointer.set, CESK_STORE_ADDR_ZERO) < 0)
				{
					LOG_ERROR("can not push default to the field");
					goto ERROR;
				}
				/* fianlly attach it */
				if(cesk_store_attach(dest, dest_set_addr, setval) < 0)
				{
					LOG_ERROR("can not attach the value set the value address");
					goto ERROR;
				}
			}
			/* otehrwise, there's an value set for this field, aquire it directly */
			else
			{
				setval = cesk_store_get_rw(dest, dest_set_addr);
				if(NULL == setval)
				{
					LOG_ERROR("can not aquire writable pointer to the destination set");
					goto ERROR;
				}
			}
			if(CESK_TYPE_SET != setval->type)
			{
				LOG_ERROR("the field value should be a set, but it's not");
				cesk_store_release_rw(dest, dest_set_addr);
				goto ERROR;
			}
			/* now we have a valid and writable set value for merge */
			cesk_set_t* set = setval->pointer.set;

			/* if the source set is empty, just put a zero in the desintation set */
			if(CESK_STORE_ADDR_NULL == sour_set_addr)
			{
				if(cesk_set_push(set, CESK_STORE_ADDR_ZERO) < 0)
				{
					LOG_ERROR("can not push value to the destination set");
					cesk_store_release_rw(dest, dest_set_addr);
					goto ERROR;
				}
			}
			/* otherwise, there are some value in the destination set, use the set merge function */
			else
			{
				cesk_value_const_t* tmp = cesk_store_get_ro(sour, sour_set_addr);
				if(NULL == tmp)
				{
					LOG_ERROR("can not get the value set in the sour store");
					cesk_store_release_rw(dest, dest_set_addr);
					goto ERROR;
				}
				const cesk_set_t* src_set = tmp->pointer.set;
				if(NULL == src_set)
				{
					LOG_ERROR("the set value should not be NULL pointer");
					cesk_store_release_rw(dest, dest_set_addr);
					goto ERROR;
				}
				if(cesk_set_merge_reloc(set, src_set, reloc) < 0)
				{
					LOG_ERROR("can not merge the field set");
					cesk_store_release_rw(dest, dest_set_addr);
					goto ERROR;
				}
			}

			/* After everthing is done, we simply release the pointer */
			cesk_store_release_rw(dest, dest_set_addr);
		}
		CESK_OBJECT_STRUCT_ADVANCE(sour_struct);
		CESK_OBJECT_STRUCT_ADVANCE(dest_struct);
	}
	return 0;
ERROR:
	LOG_ERROR("can not merge two object");
	if(dest_addr != 0) cesk_store_release_rw(dest, dest_addr);
	return -1;
}

