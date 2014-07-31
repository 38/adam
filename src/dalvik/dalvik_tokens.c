#include <sexp.h>
#include <dalvik/dalvik_tokens.h>
#include <stringpool.h>
#include <log.h>
#include <debug.h>

const char* dalvik_keywords[DALVIK_MAX_NUM_KEYWORDS];

const char* _dalvik_token_defs[DALVIK_MAX_NUM_KEYWORDS] = {
	"move",
	"return",
	"const",
	"monitor",
	"check",
	"instance",
	"array",
	"new",
	"filled",
	"16",
	"from16",
	"wide",
	"object",
	"result",
	"exception",
	"void",
	"4",
	"high16",
	"32",
	"class",
	"jumbo",
	"enter",
	"exit",
	"cast",
	"of",
	"length",
	"range",
	"throw",
	"goto",
	"packed",
	"switch",
	"sparse",
	"cmpl",
	"cmpg",
	"cmp",
	"float",
	"double",
	"long",
	"if",
	"eq",
	"ne",
	"le",
	"ge",
	"gt",
	"lt",
	"eqz",
	"nez",
	"lez",
	"gez",
	"gtz",
	"ltz",
	"boolean",
	"byte",
	"char",
	"short",
	"aget",
	"aput",
	"sget",
	"sput",
	"iget",
	"iput",
	"invoke",
	"virtual",
	"super",
	"direct",
	"static",
	"interface",
	"int",
	"to",
	"neg",
	"not",
	"add",
	"sub",
	"mul",
	"div",
	"rem",
	"and",
	"or",
	"xor",
	"shl",
	"shr",
	"ushl",
	"ushr",
	"2addr",
	"lit8",
	"lit16",
	"nop",
	"string",
	"default",
	"rsub",
	"method",
	"attrs",
	"abstract",
	"annotation",
	"final",
	"private",
	"protected",
	"public",
	"synchronized",
	"transient",
	"line",
	"limit",
	"registers",
	"label",
	"field",
	"source",
	"implements",
	"data",
	"catch",
	"catchall",
	"fill",
	"using",
	"from",
	"__DEBUG__",
	"true",
	"false",
	NULL
}; 

int dalvik_tokens_init(void)
{
	int i;
	for(i = 0; _dalvik_token_defs[i]; i ++)
		dalvik_keywords[i] = stringpool_query(_dalvik_token_defs[i]);
	return 0;
}
