#ifndef __STRINGPOOL_H__

#define STRINGPOOL_MURMUR_MAGIC 0xc6a4a793
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
} stringpool_accumulator_t;

/* initialize an accumulator
 * buf:   The memory for this accumulator
 * begin: Where the string we are going to scan begin
 */
void stringpool_accumulator_init(stringpool_accumulator_t* buf, const char* begin);
/* put a char to the accumulator */
static inline void stringpool_accumulator_next(stringpool_accumulator_t* acc, char c)
{
    acc->last = (acc->last >> 8) | (((unsigned char)c)<<24);   /* assume big endian */
    /* TODO: Little endian */
    if(acc->count%4 == 0 && acc->count != 0)
    {
        //TODO
    }
    acc->count ++;
}
/* query current string */
const char* stringpool_accumulator_query(stringpool_accumulator_t* acc);
#endif /*__STRINGPOOL_H__*/
