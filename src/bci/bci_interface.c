#include <dalvik/dalvik.h>
#include <cesk/cesk.h>
#include <bci/bci.h>
const cesk_set_t* bci_interface_read_arg(const bci_method_env_t* env, uint32_t k, uint32_t N)
{
	if(NULL == env || N + 2 > env->frame->size || k >= N)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	return env->frame->regs[env->frame->size - N + k];
}
uint32_t bci_interface_new_object(bci_method_env_t* env, const char* path, const void* init_param)
{
	if(NULL == env || NULL == path)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	cesk_alloc_param_t param = CESK_ALLOC_PARAM(CESK_ALLOC_NA, CESK_ALLOC_NA);
	return cesk_frame_store_new_object(env->frame, env->rtable, env->instruction, &param, path, init_param, env->D, NULL);
}
int bci_interface_return_set(bci_method_env_t* env, const cesk_set_t* set)
{
	if(NULL == env || NULL == set)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	return cesk_diff_buffer_append(env->D, CESK_DIFF_REG, CESK_FRAME_RESULT_REG, set); 
}
int bci_interface_return_single_address(bci_method_env_t* env, uint32_t addr)
{
	if(NULL == env || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	return cesk_frame_register_load(env->frame, CESK_FRAME_RESULT_REG, addr, NULL, env->D, NULL);
}
const cesk_set_t* bci_interface_read_register(const bci_method_env_t* env, uint32_t regid)
{
	if(NULL == env || regid >= env->frame->size) 
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	return env->frame->regs[regid];
}
void* bci_interface_get_rw_by_classdef(bci_method_env_t* env, uint32_t addr, const bci_class_t* class)
{
	if(NULL == env || NULL == class || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_value_t* value = cesk_store_get_rw(env->frame->store, addr, 0);
	if(NULL == value)
	{
		LOG_ERROR("can not get a writable pointer to address "PRSAddr" at store %p", addr, env->frame->store);
		return NULL;
	}
	if(CESK_TYPE_SET == value->type)
	{
		LOG_ERROR("a value set rather than an object instance has been allocated to address"PRSAddr". Something went wrong", addr);
		return NULL;
	}
	cesk_object_t* object = value->pointer.object;
	cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->built_in && this->class.bci->class == class)
			return this->bcidata;
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return NULL;
}
void* bci_interface_get_rw(bci_method_env_t* env, uint32_t addr, const char* classpath)
{
	if(NULL == env || NULL == classpath || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_value_t* value = cesk_store_get_rw(env->frame->store, addr, 0);
	if(NULL == value)
	{
		LOG_ERROR("can not get a writable pointer to address "PRSAddr" at store %p", addr, env->frame->store);
		return NULL;
	}
	if(CESK_TYPE_SET == value->type)
	{
		LOG_ERROR("a value set rather than an object instance has been allocated to address"PRSAddr". Something went wrong", addr);
		return NULL;
	}
	cesk_object_t* object = value->pointer.object;
	cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->built_in && this->class.path->value == classpath)
			return this->bcidata;
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return NULL;
}
int bci_interface_append_tag_set(bci_method_env_t* env, uint32_t addr, const tag_set_t* tags)
{
	if(NULL == env || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	cesk_value_t* value = cesk_store_get_rw(env->frame->store, addr, 0);
	if(NULL == value)
	{
		LOG_ERROR("can not get a writable pointer to address "PRSAddr" at store %p", addr, env->frame->store);
		return -1;
	}
	if(CESK_TYPE_SET == value->type)
	{
		const tag_set_t* old_tags = cesk_set_get_tags(value->pointer.set);
		tag_set_t* new_tags = tag_set_merge(old_tags, tags);
		cesk_set_assign_tags(value->pointer.set, new_tags);
	}
	else
	{
		tag_set_t* old_tags = value->pointer.object->tags;
		tag_set_t* new_tags = tag_set_merge(old_tags, tags);
		if(NULL != old_tags) tag_set_free(old_tags);
		value->pointer.object->tags = new_tags;
	}
	return 0;
}
void bci_interface_release_rw(bci_method_env_t* env, uint32_t addr)
{
	cesk_store_release_rw(env->frame->store, addr);
}
const void* bci_interface_get_ro(bci_method_env_t* env, uint32_t addr, const char* classpath)
{
	if(NULL == env || NULL == classpath || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_value_const_t* value = cesk_store_get_ro(env->frame->store, addr);
	if(NULL == value)
	{
		LOG_ERROR("can not get a writable pointer to address "PRSAddr" at store %p", addr, env->frame->store);
		return NULL;
	}
	if(CESK_TYPE_SET == value->type)
	{
		LOG_ERROR("a value set rather than an object instance has been allocated to address"PRSAddr". Something went wrong", addr);
		return NULL;
	}
	const cesk_object_t* object = value->pointer.object;
	const cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->built_in && this->class.path->value == classpath)
			return this->bcidata;
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return NULL;
}
const void* bci_interface_get_ro_by_classdef(bci_method_env_t* env, uint32_t addr, const bci_class_t* class)
{
	if(NULL == env || NULL == class || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	cesk_value_const_t* value = cesk_store_get_ro(env->frame->store, addr);
	if(NULL == value)
	{
		LOG_ERROR("can not get a writable pointer to address "PRSAddr" at store %p", addr, env->frame->store);
		return NULL;
	}
	if(CESK_TYPE_SET == value->type)
	{
		LOG_ERROR("a value set rather than an object instance has been allocated to address"PRSAddr". Something went wrong", addr);
		return NULL;
	}
	const cesk_object_t* object = value->pointer.object;
	const cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->built_in && this->class.bci->class == class)
			return this->bcidata;
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return NULL;
}
int bci_interface_return_object(bci_method_env_t* env, uint32_t addr)
{
	if(NULL == env || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	cesk_value_const_t* value = cesk_store_get_ro(env->frame->store, addr);
	if(NULL == value)
	{
		LOG_ERROR("can not get a readonly pointer to address "PRSAddr, addr);
		return -1;
	}
	return cesk_diff_buffer_append(env->D, CESK_DIFF_STORE, addr, value);
}
