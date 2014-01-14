#include <errorcode.h>
#include <stdio.h>

errorcode_t* _errorcode_last_errcode;
const char* _errorcode_last_errfile;
int _errorcode_last_errline;

errorcode_t global_errorcodes[] = {
    { 0, "success"}
    { 1, "unknown error"}
};


retstatus_t errorcode_raise(const errorcode_t* code, const char* file, int line)
{
    _errorcode_last_errcode = code;
    _errorcode_last_errline = line;
    _errorcode_last_errfile = file;
    return -_errorcode_last_errcode->status;
}
const char* errorcode_strerr(const char* prefix)
{
    static char* buffer[1024];
    if(_errorcode_last_errcode == NULL) 
        snprintf(buffer, sizeof(buffer), "%s: %s", prefix?prefix:"", _errorcode_last_errcode->desc);
    return buffer;
}
void errorcode_clear(void)
{
    errorcode_raise(code, "", 0);
}
retstatus_t errorcode_last(void)
{
    return -_errorcode_last_errcode->status;
}
