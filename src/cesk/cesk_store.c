#include <string.h>

#include <log.h>

#include <cesk/cesk_store.h>
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
   return ret;
}

cesk_store_t* cesk_store_fork(cesk_store_t* store)
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
    LOG_DEBUG("a store of %d entities is being forked, %d bytes copied", ret->num_ent, size);
    return ret;
}

const cesk_value_t* cesk_store_get_ro(cesk_store_t* store, uint32_t addr)
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
    return block->slots[offset].value;
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
    return val;
}
uint32_t cesk_store_allocate(cesk_store_t** p_store, dalvik_instruction_t* inst)
{
    cesk_store_t* store = *p_store;
    dalvik_instruction_newidx_t idx;
    dalvik_instruction_read_annotation(inst, &idx, sizeof(idx));
    uint32_t  init_slot = (idx * 2654435761ul)  % CESK_STORE_BLOCK_NSLOTS;
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
    for(attempt = 0; attempt < 5; attempt ++)
    {
        int block;
        LOG_DEBUG("attempt #%d @%d for instruction %d", attempt, slot, idx);
        for(block = 0; block < store->nblocks; block ++)
        {
            if(store->blocks[block]->slots[slot].value == NULL && empty_offset == -1)
            {
                LOG_DEBUG("find a empty slot @ (block = %d, offset = %d)", block, slot);
                empty_block = block;
                empty_offset = slot;
            }
            if(store->blocks[block]->slots[slot].value && 
               store->blocks[block]->slots[slot].idx == idx)
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
            LOG_DEBUG("allocate %d (block=%d, offset = %d) for instruction %d", 
                       empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset, empty_block, empty_offset);
            store->blocks[empty_block]->slots[empty_offset].idx = idx;
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
        LOG_DEBUG("reuse %d (block = %d, offset = %d for instruction %d",
                   equal_block * CESK_STORE_BLOCK_NSLOTS + equal_offset, equal_block, equal_offset);
        store->blocks[equal_block]->slots[equal_offset].reuse = 1;   
        return equal_block * CESK_STORE_BLOCK_NSLOTS  + equal_offset;
    }
}
#if 0
/* this allocator use allocte a fresh address, so it's deprecated */
uint32_t cesk_store_allocate(cesk_store_t** p_store)
{
    cesk_store_t* store = *p_store;
    size_t capacity = store->nblocks * CESK_STORE_BLOCK_NSLOTS;
    uint32_t block = 0;
    uint32_t offset = 0;
    if(capacity <= store->num_ent)
    {
        LOG_DEBUG("store is full, allocate a new block for it");
        store = realloc(store, sizeof(cesk_store_t) + sizeof(cesk_store_block_t*) * (store->nblocks + 1));
        if(NULL == store)
        {
            LOG_ERROR("can not increase the size of store");
            return CESK_STORE_ADDR_NULL;
        }
        *p_store = store;
        store->blocks[store->nblocks] = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
        if(NULL == store->blocks[store->nblocks]) 
        {
            LOG_ERROR("can not allocate memory for new store block");
            return CESK_STORE_ADDR_NULL;
        }
        memset(store->blocks[store->nblocks], 0, CESK_STORE_BLOCK_SIZE);
        store->blocks[store->nblocks]->refcnt ++;
        block = store->nblocks ++;
        offset = 0;
    }
    else
    {
        /* store is not full, find a empty slot */
        for(block = 0; block < store->nblocks; block ++)
        {
            if(store->blocks[block]->num_ent < CESK_STORE_BLOCK_NSLOTS)
            {
                /* find a block that has unused slots */
                for(offset = 0; offset < CESK_STORE_BLOCK_NSLOTS; offset ++)
                {
                    if(NULL == store->blocks[block]->slots[offset].value) 
                    {
                        goto FOUND;
                    }
                }
            }
        }
        LOG_ERROR("unreachable code");
        return CESK_STORE_ADDR_NULL;
    }
FOUND:
    return block * CESK_STORE_BLOCK_NSLOTS + offset;
}
#endif
int cesk_store_attach(cesk_store_t* store, uint32_t addr,cesk_value_t* value)
{
    if(NULL == store || CESK_STORE_ADDR_NULL == addr || NULL == value)
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
        LOG_TRACE("value is already attached to this addr");
        return 0;
    }
    if(store->blocks[block]->refcnt > 1)
    {
        /* copy this block */
        cesk_store_block_t* newblock = _cesk_store_block_fork(store->blocks[block]);
        if(NULL == newblock)
        {
            LOG_ERROR("can't fork store");
            return -1;
        }
        store->blocks[block]->refcnt --;
        newblock->refcnt ++;
        store->blocks[block] = newblock;
    }
    if(store->blocks[block]->slots[offset].value == NULL && value != NULL) 
    {
        store->blocks[block]->num_ent ++;
        store->num_ent ++;
    }
    else if(store->blocks[block]->slots[offset].value != NULL && value == NULL)
    {
        store->blocks[block]->num_ent --;
        store->num_ent --;
    }
    if(NULL != store->blocks[block]->slots[offset].value)
    {
        /* dereference to previous object */
        cesk_value_decref(store->blocks[block]->slots[offset].value);
    }
    if(value)
    {
        /* reference to new value */
        cesk_value_incref(value);
    }
    store->blocks[block]->slots[offset].value = value;
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
/* get a block in a store and prepare to write */
static inline cesk_store_block_t* _cesk_store_getblock_rw(cesk_store_t* store, uint32_t addr)
{
    if(NULL == store || CESK_STORE_ADDR_NULL) 
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

int cesk_store_incref(cesk_store_t* store, uint32_t addr)
{
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
    uint32_t idx = addr / CESK_STORE_BLOCK_NSLOTS;
    uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
    if(idx >= store->nblocks)
    {
        LOG_ERROR("out of memory");
        return -1;
    }
    if( store->blocks[idx]->slots[ofs].value == NULL)
    {
        LOG_ERROR("can not decref on a dead object");
        return -1;
    }
    cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
    if(NULL == block)
        return -1;

    /* decrease the counter */
    if(block->slots[ofs].refcnt > 0) 
        block->slots[ofs].refcnt --;

    if(0 == block->slots[ofs].refcnt)
    {
        LOG_TRACE("value %xS is dead, swipe it out", addr);
        /* unreference from this frame */
        cesk_value_decref(block->slots[ofs].value);
        /* release the address */
        block->slots[ofs].value = NULL; 
        block->num_ent --;
        store->num_ent --;
    }
    return block->slots[ofs].refcnt;
}

//TODO: cycle reference detection & collection
