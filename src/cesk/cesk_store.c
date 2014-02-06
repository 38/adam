#include <string.h>

#include <cesk/cesk_store.h>
/* make a copy of a store block */
static inline cesk_store_block_t* _cesk_store_block_fork(cesk_store_block_t* block)
{
    cesk_store_block_t* new_block = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
    if(NULL == new_block) 
    {
        LOG_ERROR("can not allocate memory for new block");
        return NULL;
    }
    memcpy(new_block, block, CESK_STORE_BLOCK_SIZE);
    new_block->refcnt = 0;
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
}

cesk_store_t* cesk_store_fork(cesk_store_t* store)
{
    int i;
    size_t size = sizeof(cesk_store_t) + sizeof(cesk_store_block_t) * store->nblocks;
    cesk_store_t* ret = (cesk_store_t*)malloc(size);
    if(NULL == ret)
    {
        LOG_ERROR("can not allocate memory for new store");
        return NULL;
    }
    memcpy(ret, store, size);
    /* increase refrence counter of all blocks */
    for(i = 0; i < ret->nblocks; i ++)
        ret->blocks[i] ++;
    LOG_DEBUG("a store of %d entities is being forked, %d bytes copied", ret->num_ent, size);
    return ret;
}

const cesk_value_t* cesk_store_get_ro(cesk_store_t* store, uint32_t addr)
{
    uint32_t block_idx = addr / CESK_STORE_BLOCK_SIZE;
    uint32_t offset    = addr % CESK_STORE_BLOCK_SIZE;
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
    return block->slots[offset];
}
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr)
{
    uint32_t block_idx = addr / CESK_STORE_BLOCK_SIZE;
    uint32_t offset    = addr % CESK_STORE_BLOCK_SIZE;
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
    cesk_value_t* val = block->slots[offset];
    if(NULL == val)
    {
        LOG_ERROR("ooops, this should not happen");
        return NULL;
    }
    if(val->refcnt > 1)
    {
        LOG_DEBUG("this value is referenced by other frame, so fork it first");
        cesk_value_t* newval = cesk_value_fork(val);
        if(NULL == newval)
        {
            LOG_ERROR("error during fork the value, aborting copy");
            return NULL;
        }
        if(block->refcnt > 1)
        {
            LOG_DEBUG("this block is referenced by other frame, so fork it before modify");
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
        block->slots[offset] = newval;

        cesk_value_decref(val);
        cesk_value_incref(newval);

        val = newval;
    }
    return val;
}
