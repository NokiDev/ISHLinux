//
// Created by b1u3dr4g0nf1y on 6/8/18.
//

#ifndef YNOVSHELL_COMMANDS_H
#define YNOVSHELL_COMMANDS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "defines.h"
#include "manifest.h"


BOOL env_cmd(char** argv, int* n);
BOOL cd_cmd(char** argv, int* n);
BOOL exit_cmd(char** argv, int* n);
BOOL env_cmd(char** argv, int* n);
BOOL version_cmd(char** argv, int* n);
BOOL pwd_cmd(char** argv, int* n);

BOOL run_internal_command(char**cmd, int*n);

#define LNAME 10

struct InternalCommand {
    char name[LNAME];
    BOOL (*f)(char** argv, int* n);
} InternalCommands[] = {
    {"pwd", pwd_cmd},
    {"cd", cd_cmd},
    {"exit", exit_cmd},
    {"env", env_cmd},
    {"version", version_cmd}
};

#endif //YNOVSHELL_COMMANDS_H
