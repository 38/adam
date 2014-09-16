#ifndef __ADDRESS_CONTAINER_H__
#define __ADDRESS_CONTAINER_H__
#include <bci/bci_interface.h>
cesk_set_t* addressContainer_dereference(uint32_t addr, const uint32_t* index, bci_method_env_t* env);
#endif
