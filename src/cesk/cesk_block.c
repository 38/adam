#include <cesk/cesk_frame.h>
#include <cesk/cesk_block.h>
static cesk_block_t* _cesk_block_buf[DALVIK_BLOCK_MAX_KEYS];
static int32_t      _cesk_block_max_idx;
static inline int _cesk_block_graph_new_imp(const dalvik_block_t* entry)
{
    if(NULL != _cesk_block_buf[entry->index]) 
    {
        /* the block has been visited */
        return 0;
    }
    
    if(_cesk_block_max_idx < entry->index) _cesk_block_max_idx = entry->index;

    size_t size = sizeof(cesk_block_t) + sizeof(cesk_block_t*) * entry->nbranches;

    cesk_block_t* ret = (cesk_block_t*)malloc(size);

    if(NULL == ret)
    {
        LOG_ERROR("can not create analAyzer block for code block %d", entry->index);
        return -1;
    }
    
    _cesk_block_buf[entry->index] = ret;
    
    ret->code_block = entry;
    ret->input = cesk_frame_new(entry->nregs);
   
    int i;
    for(i = 0; i < entry->nbranches; i ++)
    {
        if(entry->branches[i].disabled) continue;
        if(-1 == _cesk_block_graph_new_imp(entry->branches[i].block)) 
            return -1;
    }
    return 0;
}
cesk_block_t* cesk_block_graph_new(const dalvik_block_t* entry)
{
    if(NULL == entry)
        return NULL;
    memset(_cesk_block_buf, 0, sizeof(_cesk_block_buf));
    _cesk_block_max_idx = -1;
    
    _cesk_block_graph_new_imp(entry);

    int i;

    for(i = 0; i <= _cesk_block_max_idx; i ++)
        if(_cesk_block_buf[i] != NULL)
        {
            int j;
            for(j = 0; j < _cesk_block_buf[i]->code_block->nbranches; j ++)
            {
                if(_cesk_block_buf[i]->code_block->branches[j].disabled) continue;
                _cesk_block_buf[i]->fanout[j]  = _cesk_block_buf[_cesk_block_buf[i]->code_block->branches[j].block->index]->input;
            }
        }
    return _cesk_block_buf[entry->index];
}
static inline int _cesk_block_interpret(cesk_block_t* blk)
{
    //cesk_frame_t* frame = cesk_frame_fork(blk->input);   /* fork a frame for output */
    return 0;
}
int cesk_block_analyze(cesk_block_t* block)
{
    if(NULL == block)
    {
        LOG_ERROR("invalid argument");
        return -1;
    }
    _cesk_block_interpret(block);
    return 0;
}
