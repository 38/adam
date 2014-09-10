#include <bci/bci_interface.h>

bci_class_t java_io_InputStream_metadata = {
	.size = 0,
	.super = "java/lang/Object",
	.provides = {
		"java/io/InputStream",
		NULL
	}
};
