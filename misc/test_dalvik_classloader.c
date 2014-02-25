#include <adam.h>
#include <dalvik/dalvik_loader.h>
#include <stdio.h>
char buf[1024000];
int main(int argv, char** argc)
{
    FILE* fp;
    adam_init();
	if(argv == 1)
	{
		fprintf(stderr, "load a class defination file, usage: %s sxddx_file", argc[0]);
		return 1;
	}
    fp = fopen(argc[1], "r");
    int nbytes = fread(buf, 1, sizeof(buf), fp);
    buf[nbytes] = 0;
    sexpression_t *sexp;
    sexp_parse(buf, &sexp);
    dalvik_class_from_sexp(sexp);
    sexp_free(sexp);
    adam_finalize();
    return 0;
}
