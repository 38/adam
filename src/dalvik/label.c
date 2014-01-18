#include <dalvik/label.h>
#include <string.h>

dalvik_instruction_t* davlik_label_jump_table[DAVLIK_LABEL_POOL_SIZE];

typedef struct _dalvik_label_map_t {
    const char* label;
    uint32_t    idx;
    struct _dalvik_label_map_t* next;
} dalvik_label_map_t;

dalvik_label_map_t* _dalvik_label_map_table[DAVLIK_LABEL_POOL_SIZE];
int _dalvik_label_count;

void dalvik_label_init(void)
{
    _dalvik_label_count = 0;
    memset(_dalvik_label_map_table, 0, sizeof(_dalvik_label_map_table));
}
void dalvik_label_free(void)
{
    int i;
    for(i = 0; i < DAVLIK_LABEL_POOL_SIZE; i ++)
    {
        dalvik_label_map_t* ptr;
        for(ptr = _dalvik_label_map_table[i]; ptr; )
        {
            dalvik_label_table_map_t* this = ptr;
            ptr = ptr->next;
            free(this);
        }
    }
}
int dalvik_label_get_label_id(const char* label)
{
    int idx = ((uint64_t)label)%DAVLIK_LABEL_POOL_SIZE;
    dalvik_label_map_t* ptr;
    for(ptr = _dalvik_label_map_table[idx]; ptr; ptr = ptr->next)
        if(ptr->label == label) return ptr->idx;
    ptr = (dalvik_label_map_t*)malloc(sizeof(dalvik_label_map_t));
    if(NULL == ptr) return -1;
    if(_dalvik_label_count >= DAVLIK_LABEL_POOL_SIZE) return -1;  /* oops, too many labels */
    ptr->label = label;
    ptr->idx   = _dalvik_label_count ++;
    ptr->next  = _dalvik_label_map_table[idx];
    _dalvik_label_map_table[idx] = ptr;
    return ptr->idx;
}
