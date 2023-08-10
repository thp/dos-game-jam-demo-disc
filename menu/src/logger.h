#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <stdarg.h>

void init_logger(void);
void cleanup_logger(void);

int add_log_file(const char *fname);
int add_log_stream(FILE *fp);
int add_log_console(const char *devname);
int add_log_callback(void (*cbfunc)(const char*, void*), void *cls);

int errormsg(const char *fmt, ...);
int warnmsg(const char *fmt, ...);
int infomsg(const char *fmt, ...);
int dbgmsg(const char *fmt, ...);

int verrormsg(const char *fmt, va_list ap);
int vwarnmsg(const char *fmt, va_list ap);
int vinfomsg(const char *fmt, va_list ap);
int vdbgmsg(const char *fmt, va_list ap);

#endif	/* LOGGER_H_ */
