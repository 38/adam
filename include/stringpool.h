#ifndef __STRINGPOOL_H__
/* 
 * query a string in the pool, if the string is alread in the pool, return the memory in pool direcly , 
 * if not, the allocate a new item in the pool
 */
const char* stringpool_query(const char* str);

int stringpool_init(int poolsize);

void stringpool_fianlize(void);

#endif /*__STRINGPOOL_H__*/
