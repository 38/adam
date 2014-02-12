#include <string.h>

#include <log.h>

#include <dalvik/dalvik_block.h>

#include <vector.h>

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

#if 0
    uint32_t inst;
    uint32_t begin;
    dalvik_instruction_t * current_inst = NULL;
    
    size_t nbranches = 0;

    for(begin = inst = method->entry; DALVIK_INSTRUCTION_INVALID != inst; inst = current_inst->next)
    {
        current_inst = dalvik_instruction_get(inst);
        //TODO: parse it
    }
#endif
    return NULL;
}
