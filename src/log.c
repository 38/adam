#include <constants.h>
#include <log.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static char _log_path[8][128] = {};
static FILE* _log_fp[8] = {};
void log_init()
{
    FILE* default_fp = stderr;
    FILE* fp = fopen(CONFIG_PATH "/log.cfg", "r");
    strcpy(_log_path[7], "<stderr>");
    _log_fp[7] = stderr;
    if(NULL != fp)
    {
        static char buf[1024];
        static char type[1024];
        static char path[1024];
        static char mode[1024];
        for(;NULL != fgets(buf, sizeof(buf), fp);)
        {
            char* begin = NULL;
            char* end = NULL;
            char* p;
            for(p = buf; *p; p ++)
            {
                if(*p != '\t' &&
                   *p != ' '  &&
                   NULL == begin)
                    begin = p;
                if((*p == '#'  ||
                    *p == '\n' ||
                    *p == '\r') &&
                   NULL == end)
                    end = p;
            }
            if(NULL == begin) begin = buf;
            if(NULL == end)   end = p;
            *end = 0;
            int rc = sscanf(begin, "%s%s%s", type, path, mode);
            if(rc < 2) continue;
            if(rc == 2) 
            {
                mode[0] = 'w';
                mode[1] = '0';
            }
            int level;
#define     _STR_TO_ID(name) else if(strcmp(type, #name) == 0) level = name
            if(0);
            _STR_TO_ID(DEBUG);
            _STR_TO_ID(TRACE);
            _STR_TO_ID(INFO);
            _STR_TO_ID(NOTICE);
            _STR_TO_ID(WARNING);
            _STR_TO_ID(ERROR);
            _STR_TO_ID(FATAL);
            else if(strcmp(type, "default") == 0)
                level = 7;
            else continue;
#undef      _STR_TO_ID

            FILE* outfile = NULL; 
            if(strcmp(path, "<stdin>") == 0) outfile = stdin;
            else if(strcmp(path, "<stdout>") == 0) outfile = stdout;
            else if(strcmp(path, "<stderr>") == 0) outfile = stderr;
            else
            {
                int i;
                for(i = 0; i < 8 ; i ++)
                    if(_log_fp[i] != NULL && strcmp(path, _log_path[i]) == 0)
                    {
                        outfile = _log_fp[i];
                        break;
                    }
                if(NULL == outfile)
                {
                    outfile = fopen(path, mode);
                }
            }
            if(_log_fp[level] != NULL)
            {
                int i;
                /* more than one log file, override */ 
                FILE* unused = _log_fp[level];
                for(i = 0; i < 8; i ++)
                    if(_log_fp[i] == unused && i != level)
                        break;
                if(i == 8 && unused != stdin && unused != stdout && unused != stderr)
                    fclose(unused);
                _log_fp[level] = NULL;
            }
            _log_fp[level] = outfile;
            strcpy(_log_path[level], path);
        }
        if(_log_fp[7] != NULL) default_fp = _log_fp[7];
    }
    int i;
    for(i = 0; i < 7; i ++)
        if(_log_fp[i] == NULL)
            _log_fp[i] = default_fp;
}
void log_finalize()
{
    int i, j;
    for(i = 0; i < 8; i ++)
        if(_log_fp[i] != NULL &&
           _log_fp[i] != stdin &&
           _log_fp[i] != stdout &&
           _log_fp[i] != stderr)
        {
            FILE* unused = _log_fp[i];
            for(j = 0; j < 8; j ++)
                if(unused == _log_fp[j])
                    _log_fp[j] = NULL;
            fclose(unused);
        }
}
void log_write(int level, const char* file, const char* function, int line, const char* fmt, ...)
{
    static const char LevelChar[] = "FEWNITD";
    FILE* fp = _log_fp[level];
    va_list ap;
    fprintf(fp,"%c[%s@%s:%3d] ",LevelChar[level],function,file,line);
    va_start(ap,fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fprintf(fp, "\n");
}
