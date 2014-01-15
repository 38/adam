#include <stringpool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
typedef struct _stringpool_hashnode_t{
    uint32_t h[4];
    char* str;
    struct _stringpool_hashnode_t* next;
} stringpool_hashnode_t;
stringpool_hashnode_t **_stringpool_hash;
size_t  _stringpool_size;
static int _stringpool_hash_func(const char* str, uint32_t* h)
{
	const uint32_t m = STRINGPOOL_MURMUR_M;
	const unsigned char* data = (unsigned char*) str;
	h[0] = STRINGPOOL_MURMUR_SEED;//Murmur3
	h[1] = 0;					//sum
	h[2] = STRINGPOOL_MULTIPLY_SEED;   	    //muliply
	h[3] = 0;					//ELF Hash
	int i = 0;
	for(i = 0; str[i]; i ++)
	{
		if((i&3) == 0 && i != 0)
		{
			uint32_t k = *(uint32_t*)(data + i - 4);
            k *= STRINGPOOL_MURMUR_C1;
            k  = (k << STRINGPOOL_MURMUR_R1) | (k >> (32 - STRINGPOOL_MURMUR_R1));
            k *= STRINGPOOL_MURMUR_C2;

			h[0] ^= k;
			h[0] *= (h[0] << STRINGPOOL_MURMUR_R2) | (h[0] >> (32 - STRINGPOOL_MURMUR_R2));
			h[0] ^= h[0] * STRINGPOOL_MURMUR_M + STRINGPOOL_MURMUR_N;
		}
		h[1] = (h[1] + data[i] + (data[i]<<STRINGPOOL_SUM_R1) + (data[i]>>STRINGPOOL_SUM_R2))%STRINGPOOL_SUM_M;
		h[2] = (data[i] * h[2])%STRINGPOOL_MULTIPLY_M;
		h[3] = (h[3] << 4) + data[i];
		uint32_t g = h[3] & STRINGPOOL_ELF_MSK;
		if(g) h[3]^=g>>24;
		h[3] &= ~g;
	}
    /* process the unprocessed input, and swap endian order */
    uint32_t remaining = 0;
	switch(i&3)
	{
        /* following code assume big endian, for little endian, this is not needed */
		case 0:
			remaining |= data[i-4] << 24;
		case 3:
		    remaining |= data[i-3] << 16;
		case 2:
			remaining |= data[i-2] << 8;
		case 1: 
			remaining |= data[i-1];
	}
    remaining *= STRINGPOOL_MURMUR_C1;
    remaining  = (remaining << STRINGPOOL_MURMUR_R1) | (remaining >> STRINGPOOL_MURMUR_R2);
    remaining *= STRINGPOOL_MURMUR_C2;
    h[0] ^= remaining;

    /* we are finishing */
    h[0] ^= i;
    h[0] ^= h[0] >> 16;
	h[0] *= 0x85ebca6b;
	h[0] ^= h[0]>>13;
	h[0] *= 0xc2b2ae35;
	h[0] ^= h[0]>>16;

    /* finally return the length */

    return i;
}
const char* stringpool_query(const char* str)
{
    if(NULL == str) return NULL;

    uint32_t h[4];
    int len;
    len = _stringpool_hash_func(str, h);
    int idx = h[0]%_stringpool_size;
    stringpool_hashnode_t* ptr;
    for(ptr = _stringpool_hash[idx]; NULL != ptr; ptr = ptr->next)
    {
        if(ptr->h[0] == h[0] &&
           ptr->h[1] == h[1] &&
           ptr->h[2] == h[2] &&
           ptr->h[3] == h[3] &&
           strcmp(ptr->str, str) == 0) 
            return ptr->str;
    }
   
    /* If we reach this point, that means we can not find the previous address for this string */

    ptr = (stringpool_hashnode_t*)malloc(sizeof(stringpool_hashnode_t));

    if(NULL == ptr) goto ERR;

    ptr->next = _stringpool_hash[idx];

    ptr->h[0] = h[0];
    ptr->h[1] = h[1];
    ptr->h[2] = h[2];
    ptr->h[3] = h[3];

    ptr->str = (char*)malloc(len + 1);
    
    if(NULL == ptr->str) goto ERR;

    strcpy(ptr->str, str);

    _stringpool_hash[idx] = ptr;

    return ptr->str;

ERR:
    if(NULL != ptr) 
    {
        if(NULL != ptr->str)
            free(ptr->str);
        free(ptr);
    }
    return NULL;

}

int stringpool_init(int poolsize)
{
    _stringpool_size = poolsize;
    _stringpool_hash = (stringpool_hashnode_t**)malloc(sizeof(stringpool_hashnode_t*) * poolsize);
    if(NULL == _stringpool_hash) return -1;
    memset(_stringpool_hash, 0, sizeof(stringpool_hashnode_t*) * _stringpool_size);
    return 0;
}
void stringpool_fianlize(void)
{
    int i;
    for(i = 0; i < _stringpool_size; i ++)
    {
        stringpool_hashnode_t* ptr;
        for(ptr = _stringpool_hash[i]; ptr;)
        {
            stringpool_hashnode_t* cur = ptr;
            ptr = ptr->next;
            if(cur->str)
            {
                free(cur->str);
                free(cur);
            }
        }
    }
    free(_stringpool_hash);
    _stringpool_hash = NULL;
}
void stringpool_accumulator_init(stringpool_accumulator_t* buf, const char* begin)
{
    if(NULL == buf) return;
    buf->begin = begin;
    buf->count = 0;
    //TODO: here
}
