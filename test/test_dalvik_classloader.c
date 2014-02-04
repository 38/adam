#include <anadroid.h>
#include <dalvik/dalvik_loader.h>
#include <stdio.h>
int main(int argv, char** argc)
{
    FILE* fp;
    char buf[102400];
    anadroid_init();
    fp = fopen(argc[1], "r");
    int nbytes = fread(buf, 1, sizeof(buf), fp);
    buf[nbytes] = 0;
    sexpression_t *sexp;
    sexp_parse(buf, &sexp);
    dalvik_class_from_sexp(sexp);
    sexp_free(sexp);
    anadroid_finalize();
    return 0;
}
