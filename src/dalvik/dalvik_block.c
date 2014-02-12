#include <string.h>
#include <assert.h>

#include <log.h>
#include <vector.h>

#include <dalvik/dalvik_block.h>

typedef struct _dalvik_block_cache_node_t{
    const char*     methodname;
    const char*     classpath;
    dalvik_block_t* block;
    struct _dalvik_block_cache_node_t * next;
} dalvik_block_cache_node_t;

static dalvik_block_cache_node_t* _dalvik_block_cache[DALVIK_BLOCK_CACHE_SIZE];

/* allocate a node refer to the block */
static inline dalvik_block_cache_node_t* _dalvik_block_cache_node_alloc(const char* class, const char* method, dalvik_block_t* block)
{
    if(NULL == block ||
       NULL == class ||
       NULL == block )
    {
        return NULL;
    }
    dalvik_block_cache_node_t* ret = (dalvik_block_cache_node_t*) malloc(sizeof(dalvik_block_cache_node_t));
    if(NULL == ret) 
    {
        LOG_ERROR("can not allocate memory");
        return NULL;
    }
    ret->block = block;
    ret->next = NULL;
    return ret;
}
/* tranverse the graph */
static inline void _dalvik_block_tranverse_graph(dalvik_block_t* node, vector_t* result)
{
    if(NULL == node ||
       NULL == result ||
       DALVIK_BLOCK_INDEX_INVALID == node->index) 
        return;
    vector_pushback(result, &node); 
    node->index = DALVIK_BLOCK_INDEX_INVALID;  /* mark the node visited */
    int i;
    for(i = 0; i < node->fanout; i ++)
        _dalvik_block_tranverse_graph(node->branch[i]->block, result);
}
/* from the entry point, free the block graph */
static inline void _dalvik_block_graph_free(dalvik_block_t* entry)
{
    
    vector_t* vec = vector_new(sizeof(dalvik_block_t*));
    _dalvik_block_tranverse_graph(entry, vec);
    int i;
    for(i = 0; i < vector_size(vec); i ++)
    {
        dalvik_block_cache_node_t* node = *(dalvik_block_cache_node_t**)vector_get(vec, i);
        //TODO: free node->block
        LOG_INFO("fixme: free node->block");
        free(node);
    }
    vector_free(vec);
}
static inline hashval_t _dalvik_block_hash(const char* class, const char* method)
{
    return 
            (((uintptr_t)class & 0xffffffffull) * MH_MULTIPLY) ^ 
            (((uintptr_t)method & 0xffff) * MH_MULTIPLY) ^ 
            ((uintptr_t)method >> 16) ^ 
            ~((uintptr_t)class>>((sizeof(uintptr_t)/2)));
}
void dalvik_block_init()
{
    memset(_dalvik_block_cache, 0, sizeof(_dalvik_block_cache));
}
void dalvik_block_finalize()
{
    int i;
    for(i = 0; i < DALVIK_BLOCK_CACHE_SIZE; i ++)
    {
        dalvik_block_cache_node_t* p;
        dalvik_block_t* graph_entry;
        for(p = _dalvik_block_cache[i]; NULL != p;)
        {
            dalvik_block_cache_node_t* tmp = p;
            p = p->next;
            graph_entry = tmp->block;
            _dalvik_block_graph_free(graph_entry);
            free(tmp);
        }
    }
}
/* Sort the instruction by the offset. this order is equavalient to the 
 * executing order without branch. Because parser always parse the instruction
 * one by one 
 */
static int _dalvik_block_key_comp(const void* this, const void* that)
{
    return (*(int*)this) - (*(int*)that);
}
/* Tranverse all instructions in this method, figure out which is key instruction that we really take care
 * and return the offset of the address in the array.
 */
static inline int _dalvik_block_get_key_instruction_list(uint32_t entry_point, uint32_t* key, size_t size)
{
    uint32_t kcnt = 0;
    uint32_t inst;
    dalvik_instruction_t * current_inst = NULL;
#define __PUSH(value) do{\
    if(kcnt >= size)\
    {\
        LOG_ERROR("excepting %d key instructions, got more than that. Try to adjust constant DALVIK_BLOCK_MAX_KEYS");\
        return 0;\
    }\
    key[kcnt ++] = (value);\
    LOG_DEBUG("new branch %d --> %d", inst, key[kcnt - 1]);\
}while(0)
#define __PUSH_LABEL(label) __PUSH(dalvik_label_jump_table[(label)])
    for(inst = entry_point; DALVIK_INSTRUCTION_INVALID != inst; inst = current_inst->next)
    {
         current_inst = dalvik_instruction_get(inst);
         switch(current_inst->opcode)
         {
             case DVM_GOTO:
                 LOG_DEBUG("key instruction: #%d: opcode=GOTO flag=%x num_operands=%d", 
                            inst, 
                            current_inst->flags, 
                            current_inst->num_operands);
                 /* append the instruction to the key list */
                 __PUSH(inst);
                 __PUSH_LABEL(current_inst->operands[0].payload.labelid);
                 break;
             case DVM_IF:
                 LOG_DEBUG("key instruction: #%d: opcode=IF flag=%x num_operands=%d",
                           inst,
                           current_inst->flags,
                           current_inst->num_operands);
                 __PUSH(inst);
                 /* if it's not the last instruction, this is one possible routine */
                 if(current_inst->next != DALVIK_INSTRUCTION_INVALID)
                     __PUSH(current_inst->next);
                 __PUSH(current_inst->operands[2].payload.labelid);
                 break;
             case DVM_SWITCH:
                 LOG_DEBUG("key instruction: #%d: opcode=SWITCH flag=%x num_operands=%d",
                           inst,
                           current_inst->flags,
                           current_inst->num_operands);
                 __PUSH(inst);
                 if(DVM_FLAG_SWITCH_PACKED == current_inst->flags)
                 {
                     /* packed switch, use a label vector */
                     int i;
                     vector_t* lv = current_inst->operands[2].payload.branches;
                     if(NULL == lv)
                     {
                         LOG_ERROR("invalid operand");
                         return -1;
                     }
                     size_t size = vector_size(lv);
                     
                     for(i = 0; i < size; i ++)
                         __PUSH_LABEL(*(int*)vector_get(lv,i));
                 }
                 else if(DVM_FLAG_SWITCH_SPARSE == current_inst->flags)
                 {
                     /* sparse swith */
                     int i;
                     vector_t* lv = current_inst->operands[2].payload.sparse;
                     if(NULL == lv)
                     {
                         LOG_ERROR("invalid operand");
                         return -1;
                     }
                     size_t size = vector_size(lv);
                     for(i = 0; i < size; i ++)
                     {
                        dalvik_sparse_switch_branch_t* branch;
                        branch = (dalvik_sparse_switch_branch_t*) vector_get(lv, i);
                        __PUSH_LABEL(branch->labelid);
                     }
                 }
                 else
                     LOG_ERROR("invalid instruction switch(flags = %d)", current_inst->flags);
                 break;
             case DVM_INVOKE:
                 LOG_DEBUG("key instruction: #%d: opcode=INVOKE, flags=%x, noperands=%d", 
                           inst,
                           current_inst->flags,
                           current_inst->num_operands);
                 if(current_inst->next != DALVIK_INSTRUCTION_INVALID)
                     __PUSH(current_inst->next);
                 break;
             case DVM_RETURN:
                 LOG_DEBUG("key instruction #%d: opcode = RETURN, flags = %d, noperands = %d",
                            inst,
                            current_inst->flags,
                            current_inst->num_operands);
                 /* there's no next instruction actually */
                 break;
             default:
                 LOG_DEBUG("not a key instruction opcode=%d, flags=%d, noperands=%d",
                           current_inst->opcode,
                           current_inst->flags,
                           current_inst->num_operands);
         }
    }

    if(0 == kcnt)
    {
        LOG_WARNING("we can not find any key instruction here");
        return 0;
    }
    /* sort all key instructions */
    qsort(key, kcnt, sizeof(uint32_t), _dalvik_block_key_comp);
    /* make the instruction offset unique in the array */
    int next_pos = 1;
    int cur_pos = 1;

    for(; cur_pos < kcnt; cur_pos ++)
    {
        if(key[next_pos-1] == key[cur_pos]) continue;  /* just skip this, if it's a duplication of the prevoius one */
        key[next_pos++] = key[cur_pos];
    }
    kcnt = next_pos;
#if LOG_LEVEL >= 6
    LOG_DEBUG("%d key instructions in this function: ", kcnt);
    int i;
    for(i = 0; i < kcnt; i ++)
    {
        LOG_DEBUG("%d", key[i]);
    }
#endif  /* debuge infomation */
    return kcnt;
}
dalvik_block_t* dalvik_block_from_method(const char* classpath, const char* methodname, dalvik_type_t * const * typelist)
{
    if(NULL == classpath || NULL == methodname)
    {
        LOG_ERROR("class path and method name can not be true");
        return NULL;
    }
    LOG_DEBUG("get block graph of method %s/%s", classpath, methodname);
    hashval_t h = _dalvik_block_hash(classpath, methodname) % DALVIK_BLOCK_CACHE_SIZE;
    /* try to find the block graph in the cache */
    dalvik_block_cache_node_t* p;
    for(p = _dalvik_block_cache[h]; NULL != p; p = p->next)
    {
        if(p->methodname == methodname &&
           p->classpath  == classpath)
        {
            LOG_DEBUG("found the block graph in cache!");
            return p->block;
        }
    }
    /* there's no graph for this method in the cache, genterate one */
    dalvik_method_t* method = dalvik_memberdict_get_method(classpath, methodname, typelist);
    if(NULL == method) 
    {
        LOG_ERROR("can not find method %s/%s", classpath, methodname);
        return NULL;
    }
    LOG_DEBUG("find method %s/%s, entry point@%x", classpath, methodname, method->entry);

    uint32_t key[1024];     /* the maximum key instruction we are expecting */
    
    uint32_t kcnt = _dalvik_block_get_key_instruction_list(method->entry, key, sizeof(key)/sizeof(*key));

    uint32_t block_begin = method->entry;

    /* TODO: graph */ 
    return NULL;
}
