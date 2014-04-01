#include <stdio.h>
#include <stringpool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <adam.h>
const int* stringpool_accumulator_hash(stringpool_accumulator_t* acc);
const int* stringpool_hash(const char* str);

int main(int argc, char** argv) 
{
	adam_init();
	puts("Tring basic logic");
	const char *p = stringpool_query("teststring");
	assert(p != NULL);
	assert(strcmp(p, "teststring") == 0);
	char another_string[] = "teststring";
	const char* q = stringpool_query(another_string);
	assert(p == q);
	puts("Tring qurey randmoized strings");
	int i;
	char first[256];
	for(i = 0; i < 10000; i ++)
	{
		char buf[256];
		int len = rand()&0xff;
		int j;
		for(j = 0; j < len; j ++)
			buf[j] = 65 + rand() % 40;
		buf[len] = 0;
		strcpy(first, buf);
		const char* rc;
		assert( (rc = stringpool_query(buf)) != buf);
		assert(strcmp(rc, buf) == 0);

		/* Test the accumulator interface */
		stringpool_accumulator_t accumulator;
		stringpool_accumulator_init(&accumulator, buf);
		for(j = 0; j < len; j ++)
			stringpool_accumulator_next(&accumulator, buf[j]);
		const int* h1;
		const int* h2;
		h1 = stringpool_accumulator_hash(&accumulator);
		h2 = stringpool_hash(buf);
		for(j = 0; j < 4; j ++)
		{
			assert(h1[j] == h2[j]);
		}
		stringpool_accumulator_init(&accumulator, buf);
		for(j = 0; j < len; j ++)
			stringpool_accumulator_next(&accumulator, buf[j]);
		assert(stringpool_accumulator_query(&accumulator) == rc);
	}
	p = stringpool_query(first);
	assert(p != first);
	assert(strcmp(p, first) == 0);
	adam_finalize();
	return 0;
}
