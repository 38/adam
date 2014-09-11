/**
 * @brief this built class emulates the input class that actually does nothing but keep some reference of other address
 * @file addressHolder.c
 **/
#include <bci/bci_interface.h>
typedef struct {
	uint8_t init_state;
	size_t N;
	cesk_set_t** args;
} this_t;
int addressHolder_init(void* this_ptr, const void* init_param, tag_set_t** p_tags)
{
	this_t* this = (this_t*)this_ptr;
	if(this->init_state == 0) this->init_state = 1;
	else if(this->init_state == 1)
	{
		this->init_state = 2;
		return 0;
	}
	this->N = 0;
	this->args = NULL;
	return 0;
}
int addressHolder_delete(void* this_ptr)
{
	this_t* this = (this_t*)this_ptr;
	if(NULL == this->args) return 0;
	size_t i;
	for(i = 0; i < this->N; i ++)
		cesk_set_free(this->args[i]);
	free(this->args);
	return 0;
}
int addressHolder_dup(const void* this_ptr, void* that_ptr)
{
	size_t i = 0, j;
	const this_t* this = (const this_t*)this_ptr;
	this_t* that = (this_t*) that_ptr;
	that->init_state = this->init_state;
	that->N = this->N;
	if(this->N > 0)
	{
		that->args = (cesk_set_t**)malloc(sizeof(cesk_set_t *) * this->N);
		if(NULL == that->args)
		{
			LOG_ERROR("can not allocate memory for the address container");
			return -1;
		}
		for(i = 0; i < that->N; i ++)
		{
			that->args[i] = cesk_set_fork(this->args[i]);
			if(NULL == that->args[i]) 
			{
				LOG_ERROR("can not duplicate the address set #%zu", i);
				goto ERR;
			}
		}
	}
	else that->args = NULL;
	return 0;
ERR:
	for(j = 0; j < i; j ++)
		cesk_set_free(that->args[j]);
	return -1;
}
//TODO finish this
bci_class_t addressHolder_metadata = {
	.initialization = addressHolder_init,
	.finalization = addressHolder_delete,
	.duplicate = addressHolder_dup,
	.provides = {
		"java/io/InputStream",
		"java/io/OutputStream",
		"java/io/FileInputStream",
		"java/io/FileOutputStream",
		"java/io/Reader",
		"java/io/InputStreamReader",
		"java/io/Writer",
		"java/io/OutputStreamWriter",
		NULL
	}
};
