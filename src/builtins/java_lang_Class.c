#include <bci/bci_interface.h>

int java_lang_Class_instance_of(const void* this, const char* classpath)
{
	extern bci_class_t java_lang_Class_metadata;
	return classpath ==  java_lang_Class_metadata.provides[0];
}
bci_class_t java_lang_Class_metadata = {
	.size = sizeof(const char*),
	.instance_of = java_lang_Class_instance_of,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/Class",
		NULL
	}
};
