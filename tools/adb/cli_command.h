#ifndef __CLI_COMMAND_H__
#include <adam.h>

#define FUNCTION ((const char*)0x1)
#define CLASSPATH ((const char*)0x2)
#define STRING    ((const char*)0x3)
#define SEXPRESSION ((const char*)0x4)
#define FILENAME ((const char*)0x5)
#define NUMBER ((const char*)0x6)
#define VALUELIST ((const char*)0x7)

/**
 * @brief defination of a command
 **/
typedef struct cli_command_t{
	uint32_t index; /*!< the index of the command */
	const char* pattern[64]; /*!< the comamnd pattern */
	const char* description; /*!< the description, used to print help */
	union{
		struct{
			const char* class; 
			const char* method;
			const dalvik_type_t * signature[128];
			const dalvik_type_t* return_type;
		} function;   /*!< function argument */
		const char* class; /*!< class argument */
		const char* string; /*!< string argument */
		const char* literal; /*!< literal */
		sexpression_t* sexp; /*!< sexpression */
		uint32_t numeral; /*!< the numeral value */
		struct {
			uint32_t values[128];
			uint32_t size;
		} list;
	} args[64];
	int (*action)(struct cli_command_t* arg);
} cli_command_t;

#define Commands cli_command_t cli_commands[] = {
#define EndCommands {.index = 0xffffffffu}};
#define Command(id) {.index=id, .pattern = 
#define EndCommand },
#define Desc(d) ,.description = d
#define Method(name) ,.action = name
int cli_command_init();
int cli_command_match(sexpression_t* sexp);
int cli_command_get_help_text(sexpression_t* what, void* buf, uint32_t nlines, uint32_t nchar); 
#endif
