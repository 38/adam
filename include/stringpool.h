#ifndef __STRINGPOOL_H__

#include <stdint.h>
#include <stdio.h>

#define STRINGPOOL_MURMUR_C1 0xcc9e2d51ul
#define STRINGPOOL_MURMUR_C2 0x1b873593ul
#define STRINGPOOL_MURMUR_N  0xe6546b64ul
#define STRINGPOOL_MURMUR_R1 15
#define STRINGPOOL_MURMUR_R2 13
#define STRINGPOOL_MURMUR_M  5
#define STRINGPOOL_MURMUR_SEED 0x23f53423l

#define STRINGPOOL_MULTIPLY_SEED 0x23f4537l
#define STRINGPOOL_MULTIPLY_M    0xee6b27ebul

#define STRINGPOOL_SUM_M  0xfffe7985ul
#define STRINGPOOL_SUM_R1 17
#define STRINGPOOL_SUM_R2 15

#define STRINGPOOL_ELF_MSK 0xf0000000ul





/* 
 * query a string in the pool, if the string is alread in the pool, return the memory in pool direcly , 
 * if not, the allocate a new item in the pool
 */
const char* stringpool_query(const char* str);

int stringpool_init(int poolsize);

void stringpool_fianlize(void);

/* 
 * There's another interface for stringpool. 
 * The user provide the char in the string one by one,
 * rather than provide an array of char .
 * This is more effctive way, when the program is scanning
 * a string 
 */
typedef struct {
    int         count; /* how many chars recieved before */
    uint32_t    h[4];  /* current hash functions */
    uint32_t    last;
    const char* begin;
} stringpool_accumulator_t;

/* initialize an accumulator
 * buf:   The memory for this accumulator
 * begin: Where the string we are going to scan begin
 */
void stringpool_accumulator_init(stringpool_accumulator_t* buf, const char* begin);
/* put a char to the accumulator
 * acc: the accumulator
 * c  : the char
 */
static inline void stringpool_accumulator_next(stringpool_accumulator_t* acc, char c)
{
    uint8_t data = (uint8_t)c;
    acc->last = (acc->last >> 8) | (data<<24);   /* assume big endian */
    /* TODO: Little endian */
    if(acc->count%4 == 0 && acc->count != 0)
    {
        uint32_t k = acc->last;
        printf("-%u\n", k);
        k *= STRINGPOOL_MURMUR_C1;
        k  = (k << STRINGPOOL_MURMUR_R1) | (k >> (32 - STRINGPOOL_MURMUR_R1));
        k *= STRINGPOOL_MURMUR_C2;

        acc->h[0] ^= k;
        acc->h[0] *= (acc->h[0] << STRINGPOOL_MURMUR_R2) | 
                     (acc->h[0] >> (32 - STRINGPOOL_MURMUR_R2)); 
        acc->h[0] ^= acc->h[0] * STRINGPOOL_MURMUR_M + STRINGPOOL_MURMUR_N;
    }
    
    acc->h[1] = (acc->h[1] + data + (data << STRINGPOOL_SUM_R1) + (data >> STRINGPOOL_SUM_R2))%STRINGPOOL_SUM_M;
    acc->h[2] = (data * acc->h[2])%STRINGPOOL_MULTIPLY_M;
    acc->h[3] = (acc->h[3] << 4) + data;
    uint32_t g = acc->h[3] & STRINGPOOL_ELF_MSK;
    if(g) acc->h[3] ^= g >> 24;
    acc->h[3] &= ~g;
    acc->count ++;
}
/* query current string */
const char* stringpool_accumulator_query(stringpool_accumulator_t* acc);
#endif /*__STRINGPOOL_H__*/
