#ifndef __STRINGPOOL_H__
#define __STRINGPOOL_H__
/**
 * @file stringpool.h
 * @brief the string pool utils
 *
 * @details 
 * string pool manages a group of string. User can query the string pool,
 * and get an address of the pooled string which is equal to the query string.
 * For two euqal strings, the query function always returns the same address.
 * In this way, we can use pointer comparasion to compare the equality of the string.
 *
 * In this project, only string read from file are not pooled. In this way, we always
 * compare two string by comparing thier address
 *
 */
#include <constants.h>
#include <stdint.h>
#include <stdio.h>

/** 
 * @brief query a string in the pool, if the string is alread in the pool, return the memory in pool direcly , 
 * if not, the allocate a new item in the pool
 * @param str the unpooled string
 * @return pooled string, NULL means error
 */
const char* stringpool_query(const char* str);

/** @brief initliaze string pool 
 * 	@param poolsize the initial poolsize 
 * 	@return >= 0 means success, < 0 indicates error occurred */
int stringpool_init(int poolsize);
/** @brief finalize the string pool 
 *  @return nothing */
void stringpool_fianlize(void);

/** 
 * @brief string accumulator
 *
 * @details
 * There's another interface for stringpool. 
 * The user provide the char in the string one by one,
 * rather than provide an array of char .
 * This is more effctive way, when the program is scanning
 * a string
 */
typedef struct {
    int         count; /*!<how many chars recieved before */
    uint32_t    h[4];  /*!<current hash functions */
    uint32_t    last;  /*!<the last 4 bytes */
    const char* begin; /*!<begin of the string */
} stringpool_accumulator_t;

/** @brief initialize an accumulator
 * @param buf   The memory for this accumulator
 * @param begin Where the string we are going to scan begin
 * @return nothing
 */
void stringpool_accumulator_init(stringpool_accumulator_t* buf, const char* begin);
/** @brief put a char to the accumulator
 * @param acc the accumulator
 * @param c   the char
 * @return nothing
 */
static inline void stringpool_accumulator_next(stringpool_accumulator_t* acc, char c)
{
    uint8_t data = (uint8_t)c;
    /* TODO: Little endian */
    if(acc->count%4 == 0 && acc->count != 0)
    {
        uint32_t k = acc->last;
        k *= STRINGPOOL_MURMUR_C1;
        k  = (k << STRINGPOOL_MURMUR_R1) | (k >> (32 - STRINGPOOL_MURMUR_R1));
        k *= STRINGPOOL_MURMUR_C2;

        acc->h[0] ^= k;
        acc->h[0] *= (acc->h[0] << STRINGPOOL_MURMUR_R2) | 
                     (acc->h[0] >> (32 - STRINGPOOL_MURMUR_R2)); 
        acc->h[0] ^= acc->h[0] * STRINGPOOL_MURMUR_M + STRINGPOOL_MURMUR_N;
    }
    
    acc->last = (acc->last >> 8) | (data<<24);   /* assume big endian */
    
    acc->h[1] = (acc->h[1] + data + (data << STRINGPOOL_SUM_R1) + (data >> STRINGPOOL_SUM_R2))%STRINGPOOL_SUM_M;
    acc->h[2] = (acc->h[2] + data * data);
    acc->h[3] = (acc->h[3] << 4) + data;
    uint32_t g = acc->h[3] & STRINGPOOL_ELF_MSK;
    if(g) acc->h[3] ^= g >> 24;
    acc->h[3] &= ~g;
    acc->count ++;
}
/**@brief query current string
 * @param acc the accumulator to be queried
 * @return the pooled string, NULL means error
 */
const char* stringpool_accumulator_query(stringpool_accumulator_t* acc);
#endif /*__STRINGPOOL_H__*/
