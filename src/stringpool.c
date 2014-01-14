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
	const uint32_t m = 0xc6a4a793;
	const unsigned char* data = (unsigned char*) str;
	h[0] = 0x5f2c2345^(strlen(str)*m);	//Murmur
	h[1] = 0;					//sum
	h[2] = 0x23f4537;   	    //muliply
	h[3] = 0;					//ELF Hash
	int i = 0;
	for(i = 0; str[i]; i ++)
	{
		if((i&3) == 0 && i != 0)
		{
			uint32_t k = *(uint32_t*)(data + i - 4);
			h[0] += k;
			h[0] *= m;
			h[0] ^= h[0]>>16;
		}
		h[1] = (h[1] + data[i] + (data[i]<<17) + (data[i]>>15))%0xfffe7985UL;
		h[2] = (data[i] * h[2])%0xee6b27ebUL;
		h[3] = (h[3] << 4) + data[i];
		uint32_t g = h[3] & 0xf0000000ul;
		if(g) h[3]^=g>>24;
		h[3] &= ~g;
	}
	switch(i&3)
	{
		case 0:
			h[0] += data[i-4] << 24;
		case 3:
			h[0] += data[i-3] << 16;
		case 2:
			h[0] += data[i-2] << 8;
		case 1: 
			h[0] += data[i-1];
			h[0] *= m;
			h[0] ^= h[0] >> 16;
	}
	h[0] *= m;
	h[0] ^= h[0]>>10;
	h[0] *= m;
	h[0] ^= h[0]>>17;
    return i;
}
const char* stringpool_query(const char* str)
{
    uint32_t h[4];
    int len;
    len = _stringpool_hash_func(str, h);
    int idx = h[0]%_stringpool_size;
    stringpool_hashnode_t* ptr;
    for(ptr = _stringpool_hash[idx]; NULL == ptr; ptr = ptr->next)
    {
        if(ptr->h[0] == h[0] &&
           ptr->h[1] == h[1] &&
           ptr->h[2] == h[2] &&
           ptr->h[3] == h[3] &&
           strcmp(ptr->str, str) == 0) 
            return ptr->str;
    }
    
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
            if(ptr->str)
            {
                free(ptr->str);
                free(ptr);
            }
        }
    }
}
