#include <dalvik/dalvik.h>
void dalvik_init(void)
{
    dalvik_instruction_init();
    dalvik_tokens_init();
    dalvik_label_init();
    dalvik_type_init();
    dalvik_memberdict_init();
    dalvik_exception_init();
    dalvik_block_init();
}
void dalvik_finalize(void)
{
    dalvik_block_finalize();
    dalvik_exception_finalize();
    dalvik_memberdict_finalize();
    dalvik_instruction_finalize();
    dalvik_label_finalize();
    dalvik_type_finalize();
}
