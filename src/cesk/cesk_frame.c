#include <log.h>
#include <cesk/cesk_frame.h>
#include <cesk/cesk_store.h>
cesk_frame_t* cesk_frame_new(uint16_t size)
{
    if(0 == size)
    {
        return NULL;
    }
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
        case CESK_TYPE_NUMERIC:
        case CESK_TYPE_BOOLEAN:
            /* atmoic types, no reference */
            break;
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
    uint32_t addr;
    for(addr = 0; addr < nslot; addr ++)
    {
        if(BITAT(fb,addr) == 0) 
            cesk_store_attach(store,addr,NULL);
    }
    return 0;
}
hashval_t cesk_frame_hashcode(cesk_frame_t* frame)
{
    hashval_t ret = 0xa3efab97ul;
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
