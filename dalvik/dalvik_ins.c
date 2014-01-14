#include <dalvik_ins.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

dalvik_instruction_t* dalvik_instruction_pool = NULL;

size_t _dalvik_instruction_pool_capacity = DALVIK_POOL_INIT_SIZE;

size_t _dalvik_instruction_pool_size = 0;

int _dalvik_instruction_pool_resize()
{
	dalvik_instruction_t* old_pool;
	assert(dalvik_instruction_pool != NULL);
	
	old_pool = dalvik_instruction_pool;
	
	dalvik_instruction_pool = realloc(old_pool, _dalvik_instruction_pool_capacity * 2);

	if(NULL == dalvik_instruction_pool) 
	{
		dalvik_instruction_pool = old_pool;
		return -1;
	}
	_dalvik_instruction_pool_capacity *= 2;
	return 0;
}

int dalvik_instruction_init( void )
{
	_dalvik_instruction_pool_size = 0;
	if(NULL == dalvik_instruction_pool) 
		dalvik_instruction_pool = (dalvik_instruction_t*)malloc(sizeof(dalvik_instruction_t) * _dalvik_instruction_pool_capacity);
	if(NULL == dalvik_instruction_pool)
		return -1;
	return 0;
}

int dalvik_instruction_finalize( void )
{
	if(dalvik_instruction_pool != NULL) free(dalvik_instruction_pool);
	return 0;
}

dalvik_instruction_t* dalvik_instruction_new( void )
{
	if(_dalvik_instruction_pool_size >= _dalvik_instruction_pool_capacity)
	{
		if(_dalvik_instruction_pool_resize() < 0) return NULL;
	}
	return dalvik_instruction_pool + (_dalvik_instruction_pool_size ++);
}

