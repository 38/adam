/** 
 * @file constants.h
 * @brief this file constains constants
 **/
#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#ifndef LOG_DEFAULT_CONFIG_FILE 
/** @brief default log config file name */
#	define LOG_DEFAULT_CONFIG_FILE "log.cfg"
#endif

#ifndef VECTOR_INIT_CAP
/** @brief initial capacity of a vector */
#   define VECTOR_INIT_CAP 32
#endif /* VECTOR_INIT_CAP */

#ifndef DALVIK_CLASS_MAX_NUM_IMPLEMENTS
/** @brief the max number of interfaces that a class can implement **/
#	define DALVIK_CLASS_MAX_NUM_IMPLEMENTS 128
#endif

#ifndef DAVLIK_LABEL_POOL_SIZE
/** @brief the number of slot of label pool **/
#   define DAVLIK_LABEL_POOL_SIZE 655217
#endif

#ifndef STRING_POOL_SIZE
/** @brief the number of slots in string pool */ 
#   define STRING_POOL_SIZE 100003
#endif

#ifndef DALVIK_POOL_INIT_SIZE
/** @brief the initial size of the instruction pool */
#   define DALVIK_POOL_INIT_SIZE 1024
#endif

#ifndef DALVIK_MAX_CATCH_BLOCK
/** @brief the maximum number of catch blocks a method can have */
#   define DALVIK_MAX_CATCH_BLOCK 1024
#endif

#ifndef DALVIK_MEMBERDICT_SIZE
/** @brief the number of hash slots in the member dictionary */
#   define DALVIK_MEMBERDICT_SIZE 100007
#endif

#ifndef DALVIK_BLOCK_CACHE_SIZE
/** @brief the size of dalvik block graph cache */
#   define DALVIK_BLOCK_CACHE_SIZE 100007
#endif

#ifndef DALVIK_BLOCK_MAX_KEYS
/** @brief the maximum size of a key instruction list */
#   define DALVIK_BLOCK_MAX_KEYS 1024
/** @brief the maximum number of blocks in a method */
#	define CESK_METHOD_MAX_NBLOCKS DALVIK_BLOCK_MAX_KEYS  
#endif

#ifndef CESK_STORE_BLOCK_SIZE
/** @brief the size of one block in cesk store */
#   define CESK_STORE_BLOCK_SIZE 0x4000   /* 16k for each block */
#endif

#ifndef CONFIG_PATH
/** @brief where can I find the config file */
#   define CONFIG_PATH "."
#endif

#ifndef DALVIK_METHOD_LABEL_STACK_SIZE
/** @brief Max number of label a instruction can assign with */
#   define DALVIK_METHOD_LABEL_STACK_SIZE 32
#endif

#ifndef CESK_OBJECT_MAX_USER_DEFINED_CLASSES
/** @brief The limit of the number of  user defined classes are there in the inheritance path to an object **/
#	define CESK_OBJECT_MAX_USER_DEFINED_CLASSES 1024u
#endif

#ifndef CESK_OBJECT_MAX_BUILTIN_CLASSES
/** @brief The limit of the number of built-in classes are there in the inheritance path to an object */
#	define CESK_OBJECT_MAX_BUILTIN_CLASSES 1024u
#endif

#ifndef CESK_SET_HASH_SIZE
/** @brief the number of slots that used for implementation of set */
#   define CESK_SET_HASH_SIZE 100007
#endif

#ifndef CESK_STORE_ALLOC_ATTEMPT
/** @brief the number of attempts before cesk_store allocate a new block */
#	define CESK_STORE_ALLOC_ATTEMPT 5
#endif

#ifndef CESK_STORE_ADDR_CONST_PREFIX
/** @brief the address prefix for constant address */
#	define CESK_STORE_ADDR_CONST_PREFIX 0xffffff00ul
#endif

#ifndef CESK_STORE_ADDR_RELOC_PREFIX
/** @brief the address prefix for reloc address */
#	define CESK_STORE_ADDR_RELOC_PREFIX 0xff000000ul
#endif

#ifndef CESK_FRAME_REG_STATIC_PREFIX
/** @brief the address prefix for a static address */
#	define CESK_FRAME_REG_STATIC_PREFIX 0x80000000ul
#endif

#ifndef CESK_SET_EMPTY_HASH
/** @brief the hash code for empty set */
#	define CESK_SET_EMPTY_HASH 0x9c7cba63ul
#endif

#ifndef CESK_STORE_EMPTY_HASH
/** @brief the hash code for an empty store */
#	define CESK_STORE_EMPTY_HASH 0x547f6a73l
#endif

#ifndef CESK_FRAME_INIT_HASH
/** @brief the initial hash of a frame */
#	define CESK_FRAME_INIT_HASH 0xa3efab97ul
#endif

/** @brief the invalid address in the virtual store */
#define CESK_STORE_ADDR_NULL 0xfffffffful

#ifndef CESK_ALLOC_TABLE_NSLOTS
/** @brief the number of solots in an allocation table */
#	define CESK_ALLOC_TABLE_NSLOTS 100007
#endif

#ifndef CESK_ALLOC_TABLE_MAX_CHAIN_SIZE
/** @brief the maximum length of chain in an allocation table */
#	define CESK_ALLOC_TABLE_MAX_CHAIN_SIZE 5
#endif

#ifndef CESK_METHOD_CAHCE_SIZE
/** @brief the size of method analyzer */
#	define CESK_METHOD_CAHCE_SIZE 100007
#endif

#ifndef CESK_RELOC_HASH_SIZE
/** @brief the number of slots for the relocated hash **/
#	define CESK_RELOC_HASH_SIZE 655217
#endif

#ifndef CESK_BLOCK_MAX_NUM_OF_FUNC
/** @brief the max number of function the invoke function can handle 
 * (because the virtual function call can refer to different function in a single instruction) */
#	define CESK_BLOCK_MAX_NUM_OF_FUNC 128
#endif

#ifndef CESK_BLOCK_METHOD_PARTITION_HEAP_SIZE
/** @brief the max size of the heap for method partition */
#	define CESK_BLOCK_METHOD_PARTITION_HEAP_SIZE 2048
#endif

#ifndef BCI_NAMETAB_SIZE
/** @brief the size of BCI Name Table */
#	define BCI_NAMETAB_SIZE 100007
#endif

#ifndef BCI_CLASS_MAX_PROVIDES
/** @brief how many build-in class names can one built-in class have */
#	define BCI_CLASS_MAX_PROVIDES 128
#endif

/** @brief the magic number used for Knuth Multiplicative Hash */
#define MH_MULTIPLY (2654435761ul)

/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_C1 0xcc9e2d51ul
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_C2 0x1b873593ul
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_N  0xe6546b64ul
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_R1 15
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_R2 13
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_M  5
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MURMUR_SEED 0xf3f53423l

/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MULTIPLY_SEED 0x23f4537l
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_MULTIPLY_M    0xee6b27ebul

/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_SUM_M  0xfffe7985ul
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_SUM_R1 17
/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_SUM_R2 15

/** @brief the constants used for hash functions for string pool */
#define STRINGPOOL_ELF_MSK 0xf0000000ul

/** @brief define the type of a hash function returns */
#define hashval_t uint32_t

/** @brief the formatting string for a store address */
#define PRSAddr "@0x%x"
/* the configure header generated by cmake */
#include <config.h>
#endif
