#include <anadroid.h>
void anadroid_init(void)
{
    stringpool_init(STRING_POOL_SIZE);
    dalvik_instruction_init();
    dalvik_tokens_init();
    dalvik_label_init();
    dalvik_type_init();
    dalvik_memberdict_init();
}
void anadroid_finalize(void)
{
    dalvik_instruction_finalize();
    dalvik_label_finalize();
    stringpool_fianlize();
    dalvik_type_finalize();
    dalvik_memberdict_finalize();
}
