#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#ifndef DAVLIK_LABEL_POOL_SIZE
#   define DAVLIK_LABEL_POOL_SIZE 655217
#endif

#ifndef STRING_POOL_SIZE
#   define STRING_POOL_SIZE 100003
#endif

#ifndef DALVIK_POOL_INIT_SIZE
#   define DALVIK_POOL_INIT_SIZE 1024
#endif

#ifndef DALVIK_MAX_CATCH_BLOCK
#   define DALVIK_MAX_CATCH_BLOCK 1024
#endif

#ifndef DALVIK_MEMBERDICT_SIZE
#   define DALVIK_MEMBERDICT_SIZE 100007
#endif

#ifndef CESK_STORE_BLOCK_SIZE
#   define CESK_STORE_BLOCK_SIZE 4096
#endif

#ifndef CONFIG_PATH
#   define CONFIG_PATH "."
#endif

#define hashval_t uint32_t
#endif
