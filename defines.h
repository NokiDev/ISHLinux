#ifndef INCLUDE_DEFINES_H
#define INCLUDE_DEFINES_H

#define BOOL short // 8bits.
#define TRUE 1 //
#define FALSE 0 //
#define LBUF 4096 // Maximum buffer allowed for a command.

extern char **environ;
extern int execvpe(const char *, char **const, char **const);

BOOL RUN = TRUE;
char **env;
#endif
