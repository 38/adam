#include <bci/bci_interface.h>

bci_class_t java_lang_Class_metadata = {
	.size = sizeof(const char*),
	.super = "java/lang/Object",
	.provides = {
		"java/lang/Class",
		NULL
	}
};
