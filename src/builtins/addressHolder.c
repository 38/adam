/**
 * @brief this built class emulates the input class that actually does nothing but keep some reference of other address
 * @file addressHolder.c
 **/
#include <bci/bci_interface.h>
typedef struct {
	uint8_t init_state;
	size_t N;
	cesk_set_t* args;
} data_t;
bci_class_t addressHolder_metadata = {
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
