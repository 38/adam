#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* the size of label pool */
#ifndef DAVLIK_LABEL_POOL_SIZE
#   define DAVLIK_LABEL_POOL_SIZE 655217
#endif

/* the number of slots in hash table for string pool */ 
#ifndef STRING_POOL_SIZE
#   define STRING_POOL_SIZE 100003
#endif

/* the dalvik instruction pool init size */
#ifndef DALVIK_POOL_INIT_SIZE
#   define DALVIK_POOL_INIT_SIZE 1024
#endif

/* how many catch blocks does a method can have */
#ifndef DALVIK_MAX_CATCH_BLOCK
#   define DALVIK_MAX_CATCH_BLOCK 1024
#endif

/* the number of hash slots in the member dictionary */
#ifndef DALVIK_MEMBERDICT_SIZE
#   define DALVIK_MEMBERDICT_SIZE 100007
#endif

/* the size of one block in cesk store */
#ifndef CESK_STORE_BLOCK_SIZE
#   define CESK_STORE_BLOCK_SIZE 0x4000   /* 16k for each block */
#endif

/* where can I find the config file */
#ifndef CONFIG_PATH
#   define CONFIG_PATH "."
#endif

/* the number of slots that used for implementation of set */
#ifndef CESK_SET_HASH_SIZE
#   define CESK_SET_HASH_SIZE 100007
#endif

/* define the type of a hash function returns */
#define hashval_t uint32_t
#endif
