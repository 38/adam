#include <cesk/cesk_set.h>

#include <bci/bci_interface.h>
/**
 * @brief array type
 **/
typedef struct {
	uint8_t init_flag;      /*!< Initialization Flags */
	cesk_set_t* set;   /*!< Possible Values */
	cesk_set_iter_t last_iter; /*!< The last iterator */
	uint32_t last_addr;        /*!< The last address */
} _array_t;
int java_lang_Array_init(void* this, const void* param)
{
	_array_t* self = (_array_t*)this;
	if(NULL == param)
	{
		if(NULL == self->set) self->set = cesk_set_empty_set();
	}
	else
	{
		if(NULL == self->set) self->set = cesk_set_fork((const cesk_set_t*)param);

	}
	if(NULL == self->set) 
	{
		LOG_ERROR("can not initialize value set");
		return -1;
	}
	self->init_flag = 1;   /* 0 uninitialized, 1 intialized but last_iter uninitialized, 2 last_iter initilized */
	return 0;
}
int java_lang_Array_fina(void* this)
{
	_array_t* self = (_array_t*)this;
	if(0 == self->init_flag) return 0;
	if(NULL != self->set) cesk_set_free(self->set);
	return 0;
}
int java_lang_Array_dup(const void* this, void* that)
{
	const _array_t* me = (const _array_t*)this;
	_array_t* you = (_array_t*) that;
	you->init_flag = 1;
	you->set = cesk_set_fork(me->set);
	return 0;
}
int java_lang_Array_alist(const void* this, uint32_t offset, uint32_t* buf,size_t sz)
{
	_array_t* self = (_array_t*)this;
	if(self->init_flag == 1 || self->last_addr > offset)
	{
		if(cesk_set_iter(self->set, &self->last_iter) == NULL)
		{
			LOG_ERROR("Can not init iterator for the value set");
			return -1;
		}
		self->last_addr = 0;
	}
	uint32_t ret, addr;
	while(self->last_addr < offset && CESK_STORE_ADDR_NULL != cesk_set_iter_next(&self->last_iter))
		self->last_addr ++;
	if(self->last_addr != offset) return 0;
	for(ret = 0; ret < sz && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&self->last_iter)); ret ++, self->last_addr ++)
		buf[ret] = addr;
	return ret;
}
bci_class_t java_lang_Array_metadata = {
	.size = sizeof(_array_t),
	.initialization = java_lang_Array_init,
	.finalization = java_lang_Array_fina,
	.duplicate = java_lang_Array_dup,
	.get_addr_list = java_lang_Array_alist,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/Array",
		NULL
	}
};
