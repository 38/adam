#include <dalvik/dalvik_method.h>
#include <adam.h>
int main(int argc, char** argv)
{
    adam_init();
	if(argc < 2)
	{
		fprintf(stderr, "load a sxddx file, usage %s sxddx_file", argc[0]);
		return 1;
	}
    FILE* fp = fopen(argv[1], "r");
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp) + 1;
    char *buf = (char*)malloc(len);
    fseek(fp, 0, SEEK_SET);
    int nbytes;
    if((nbytes = fread(buf, 1, len, fp)) < 0)
    {
        LOG_ERROR("Can't read file");
        return 0;
    }
    buf[nbytes] = 0;

    fclose(fp);
    const char *ptr;
    for(ptr = buf; ptr != NULL && ptr[0] != 0;)
    {
        sexpression_t* sexp;
        if(NULL == (ptr = sexp_parse(ptr, &sexp)))
        {
            LOG_ERROR("Can't parse S-Expression");
            return 0;
        }
        if(SEXP_NIL == sexp) continue;
        if(NULL == dalvik_class_from_sexp(sexp))
        {
            LOG_ERROR("Can't parse class definitions");
            return 0;
        }
        sexp_free(sexp);
    }
    free(buf);
    adam_finalize();
    return 0;
}
