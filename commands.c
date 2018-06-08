//
// Created by b1u3dr4g0nf1y on 6/8/18.
//

#include <string.h>
#include "commands.h"

char **name, **value;
typedef struct EnvVar {
    char* name;
    char* value;
}EnvVar;

EnvVar* envVars;


int readEnvironmentParameters(char **cmd, int *n) {
    int i = 0;

    envVars = (EnvVar*) malloc(sizeof(char*) + sizeof(char*));

    name = (char **) malloc(sizeof(char *));
    value = (char **) malloc(sizeof(char *));
    while (cmd[*n] != NULL) {
        char *d;
        d = cmd[*n];
        name[i] = d;
        envVars[i].name = d;

        while (*d != '\0') {
            if (*d == '=')
                break;
            d++;
        }
        if (*d == '\0') {
            return 1;
        }
        *d = '\0';
        d++;
        if (*d == '\0') {
            free(name);
            return -1;
        }
        value[i] = d;
        envVars[i].value = d;

        i++;
        (*n)++;

        envVars = (EnvVar*) realloc((void*) envVars, (sizeof(char*) + sizeof(char*)) * i + 1);

        name = (char **) realloc((void *) name, sizeof(char *) * i + 1);
        value = (char **) realloc((void *) value, sizeof(char *) * i + 1);
    }
    //envVars[i] = NULL;
    name[i] = NULL;
    value[i] = NULL;
    return 0;
}

int environmentAssignment(char** cmd, int*n) {
    if (readEnvironmentParameters(cmd, n)) {
        fprintf(stderr, "Error it's [NAME=VALUE]\n");
    } else {
        int j;
        for (j = 0; name[j] != NULL; j++) {
            setenv(name[j], value[j], 1);
        }
        free((void *) name);
        free((void *) value);
    }
}

char test[LBUF];

BOOL env_cmd(char** cmd, int* n)
{
    (*n)++;
    int i = 0;
    BOOL returnVal = TRUE;
    while (env[i] != NULL) {
        i++;
    }
    char **tmpEnv = (char **) malloc(sizeof(char *) * i + 1);
    for (i = 0; env[i] != NULL; i++) {
        tmpEnv[i] = strdup(env[i]);
    }
    tmpEnv[i] = NULL;
    if (cmd[*n] != NULL) {
        int err = readEnvironmentParameters(cmd, n);
        if (err == -1) {
            fprintf(stderr, "Error, use of env is : $env [NAME=VALUE] command\n");
            returnVal = err;
        } else {
            int j;
            for (j = 0; name[j] != NULL; j++) {
                fprintf(stderr, "%s=%s", name[j], value[j]);
                int t = 0;
                for (i = 0; tmpEnv[i] != NULL; i++) {
                    if (strncmp(tmpEnv[i], name[j], strlen(name[j])) == 0) {
                        snprintf(test, sizeof test, "%s=%s", name[j], value[j]);
                        free(tmpEnv[i]);
                        tmpEnv[i] = strdup(test);
                        t = 1;
                    }
                }
                if (!t) {
                    snprintf(test, sizeof test, "%s=%s", name[j], value[j]);
                    tmpEnv[i] = strdup(test);
                    tmpEnv = (char **) realloc((void *) tmpEnv, sizeof(char *) * (i + 2));
                    tmpEnv[i + 1] = NULL;
                }
            }
            free(name);
            free(value);
            char **tmpPtr = env;
            env = tmpEnv;
            if (err == 1) {
                returnVal = run_internal_command(cmd, n);
            }
            env = tmpPtr;
        }
    }
    if (returnVal == 1) {
        for (i = 0; tmpEnv[i] != NULL; i++) {
            fprintf(stdout, "%s\n", tmpEnv[i]);
        }
    }
    for (i = 0; tmpEnv[i] != NULL; i++) {
        free(tmpEnv[i]);
    }
    free(tmpEnv);
    return returnVal;
}

BOOL cd_cmd(char** cmd, int* n)
{
    char *rep;

    (*n)++;
    if (cmd[*n] == NULL) {  // cd sans parametre
        rep = getenv("HOME"); // je recupere le contenu de la variable HOME
    } else {                // je recupere le 1er parametre
        rep = cmd[*n];
    }
    if (chdir(rep) < 0)
        perror(rep);

    free(rep);

    return TRUE;
}
BOOL exit_cmd(char** cmd, int* n)
{
    (*n)++;
    RUN = 0;
    return TRUE;
}

BOOL version_cmd(char** cmd, int* n)
{
    (*n)++;
    fprintf(stdout, "version %s\n", YnovShell_VERSION);
    return TRUE;
}

BOOL pwd_cmd(char** cmd, int* n)
{
    char *rep;
    (*n)++;
    rep = getcwd(NULL, 0);
    fprintf(stdout, "%s\n", rep);
    free(rep);
    return TRUE;
}


BOOL run_internal_command(char** cmd, int * n) {

    if (strstr(cmd[*n], "=") != NULL) {
        environmentAssignment(cmd, n);
        return TRUE;
    }

    for(int i =0; InternalCommands[i].name != NULL; i++)
    {
        if(strncmp(cmd[*n], InternalCommands[i].name, LNAME) == 0) {//internal command check.
            return InternalCommands[i].f(cmd, n);
        }
    }

    return FALSE;
}
