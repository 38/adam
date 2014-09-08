#include <dalvik/dalvik.h>
#include <cesk/cesk.h>
#include <bci/bci.h>
const cesk_set_t* bci_interface_read_arg(const bci_method_env_t* env, uint32_t k, uint32_t N)
{
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
	return env->frame->regs[regid];
}
