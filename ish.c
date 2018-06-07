/* ish.c : Ingesup Shell */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defines.h"
#include "tools.h"
#include "manifest.h"

extern char **environ;

extern int execvpe(const char *, char **const, char **const);

BOOL RUN = TRUE; /* RUN doit être mis a zero pour stopper le shell */
char **env;
char buf[LBUF];
char test[LBUF];


static int redirectionOperators[3];

char **name, **value;

int readEnvironmentParameters(char **cmd, int *n) {
    int i = 0;

    name = (char **) malloc(sizeof(char *));
    value = (char **) malloc(sizeof(char *));
    while (cmd[*n] != NULL) {
        char *d;
        d = cmd[*n];
        name[i] = d;

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

        i++;
        (*n)++;
        name = (char **) realloc((void *) name, sizeof(char *) * i + 1);
        value = (char **) realloc((void *) value, sizeof(char *) * i + 1);
    }
    name[i] = NULL;
    value[i] = NULL;
    return 0;
}

int externalCommand(char **cmd, int *n, int res, int rss, int isDaemon)
{
    int pid;
    int i;
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        for (i = 0; i < 3; i++) {
            if (redirectionOperators[i] != 0) {
                close(i);
                dup(redirectionOperators[i]);
            }
        }
        if (res != 0) {
            dup2(res, 0);
            close(res);
        }
        if (rss != 0) {
            dup2(rss, 1);
            close(rss);
        }
        execvpe(cmd[0], cmd, environ);
        perror(cmd[0]);
        exit(3);
    }
    if (rss == 0 && !isDaemon) {
        while (wait(&i) != pid);
    }
    if (rss) {
        close(rss);
    }
    if (res) {
        close(res);
    }
    return 1;
}


void clearRedirectionOperators() {
    for (int i = 0; i < 3; i++) {
        if (redirectionOperators[i] != 0) {
            close(i);
            dup(redirectionOperators[i]);
        }
    }
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

/*! \brief Check if the command is present internally in the program
 *  \param[in] cmd, line typed by the user. contains cmd, and args splitted by '\0'
 *  \param[in] n, current pointer on the cmd.
 *  \return BOOL, False if the command is not found or an error has been encountered, True otherwise
 */
BOOL internalCommand(char **cmd, int *n) {
    char *rep;

    clearRedirectionOperators();

    if (strstr(cmd[*n], "=") != NULL) {
        environmentAssignment(cmd, n);
        return TRUE;
    }
    if (strcmp(cmd[*n], "exit") == 0) { // cas le la commande interne exit
        (*n)++;
        RUN = 0;
        return TRUE;
    }
    if (strcmp(cmd[*n], "version") == 0) {
        (*n)++;
        fprintf(stdout, "version %s\n", Shell_VERSION);
        return TRUE;
    }
    if (strcmp(cmd[*n], "pwd") == 0) { // Commande pwd
        (*n)++;
        rep = getcwd(NULL, 0);
        fprintf(stdout, "%s\n", rep);
        free(rep);
        return TRUE;
    }
    if (strcmp(cmd[*n], "env") == 0) {
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
                    returnVal = internalCommand(cmd, n);
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
    if (strcmp(cmd[*n], "cd") == 0) { // commande cd
        (*n)++;
        if (cmd[*n] == NULL) {  // cd sans parametre
            rep = getenv("HOME"); // je recupere le contenu de la variable HOME
        } else {                // je recupere le 1er parametre
            rep = cmd[*n];
        }
        if (chdir(rep) < 0)
            perror(rep);
        return TRUE;
    }
    return FALSE;
}

/* fonction qui determine si un caractere est un separateur */
BOOL is_sepa(char c) {
    if (c == ' ')
        return TRUE;
    if (c == '\t')
        return TRUE;
    if (c == '\n')
        return TRUE;
    return FALSE;
}

void execute(int cmdBegin, int cmdEnd, char **words, int redirectStdin, int redirectStdout, BOOL isDaemon) {
    int NWords = cmdEnd - cmdBegin + 1;
    int it = 0, Red = 0, ired, flag, i;
    char **cmd;
    int n = 0;

    clearRedirectionOperators();

    // Preparsing the command line to catch redirection signs.
    cmd = (char **) malloc(sizeof(char *) * NWords);
    for (i = cmdBegin; i < cmdEnd; i++) {
        if (Red) { /* traitement de la redirection */
            switch (Red) {
                case 1: /* cas du > */
                    ired = 1;
                    flag = O_WRONLY | O_CREAT;
                    break;
                case 2: /* cas du >> */
                    ired = 1;
                    flag = O_WRONLY | O_CREAT | O_APPEND;
                    break;
                case 3: /* cas du < */
                    ired = 0;
                    flag = O_RDONLY;
                    break;
                case 4:
                    ired = 2;
                    flag = O_WRONLY | O_CREAT;
                case 5:
                    ired = 2;
                    flag = O_WRONLY | O_CREAT | O_APPEND;
                default:
                    fprintf(stderr, "Erreur code Red = %d !\n", Red);
            }
            if ((redirectionOperators[ired] = open(words[i], flag, 0644)) == -1) {
                perror(words[i]);
            }
            Red = 0;
        } else { /* remplissage de tab */
            if (strcmp(words[i], ">") == 0) {
                Red = 1;
                continue;
            }
            if (strcmp(words[i], ">>") == 0) {
                Red = 2;
                continue;
            }
            if (strcmp(words[i], "<") == 0) {
                Red = 3;
                continue;
            }
            if (strcmp(words[i], "2>") == 0) {
                Red = 4;
                continue;
            }
            if (strcmp(words[i], "2>>") == 0) {
                Red = 5;
                continue;
            }
            cmd[it++] = words[i];
        }
    }
    cmd[it] = NULL;
    if (!internalCommand(cmd, &n)) {
        externalCommand(cmd, &n, redirectStdin, redirectStdout, isDaemon);
    }
    free((void *) cmd);
}


/* Adds null byte as a replacement for a separator present in the separator character set.
 * \param[in] Buffer to split
 * \param[out] Words, computed from the buffer passed in
 * return number of words.
 *
 */
int bufferToWords(char *b, char ***words) {
    int i = 0;
    char *d = b;
    char *tmpP[LBUF];
    while (TRUE) {
        /* on recherche le debut du premier mot */
        while (*d != '\0') {
            if (!is_sepa(*d)) {
                break;
            }
            d++;
        }
        if (*d == '\0')
            break; /* pas de premier mot */
        tmpP[i++] = d;

        /* on recherche la fin du mot */
        while (*d != '\0') {
            if (is_sepa(*d))
                break;
            d++;
        }
        /* si c'est la fin de la chaine on casse la boucle*/
        if (*d == '\0')
            break;
        *d = '\0';
        d++;
    }
    tmpP[i] = NULL;
    *words = (char **) malloc(sizeof(char *) * (i + 1));
    for (i = 0; tmpP[i] != NULL; i++) {
        *words[i] = tmpP[i];
    }
    words[i] = NULL;
    return i;
}

void handleInput(char *buf) {
    char **words = NULL;
    if (bufferToWords(buf, &words) == 0 || words == NULL) {
        fprintf(stderr, "la commande est vide !\n");
    }
    else {
        int cmdBegin = 0, cmdEnd = 0, i = 0, redirectStdin = 0, redirectStdout = 0, pip[2];
        BOOL isPiped = FALSE;
        BOOL isDaemon = FALSE;

        for (i = 0; words[i] != NULL; i++) {
            if (words[i + 1] == NULL ||
                (strcmp(words[i], ";") == 0) ||
                (strcmp(words[i], "|") == 0) ||
                (strcmp(words[i], "&")) == 0) {

                redirectStdout = 0;
                if (!isPiped)
                    redirectStdin = 0;

                if (strcmp(words[i], "|") == 0) { // Pipe Case
                    cmdEnd = i;
                    if (pipe(pip) != 0) {
                        perror("pipe");
                        exit(3);
                    }
                    redirectStdin = pip[0];
                    redirectStdout = pip[1];
                    isPiped = TRUE;
                    execute(cmdBegin, cmdEnd, words, 0, redirectStdout, isDaemon);
                } else if (strcmp(words[i], "&") == 0) { // Daemon Case
                    cmdEnd = i;
                    isPiped = FALSE;
                    isDaemon = TRUE;
                    execute(cmdBegin, cmdEnd, words, redirectStdin, redirectStdout, isDaemon);
                } else {
                    if (strcmp(words[i], ";") == 0) // separator Case
                        cmdEnd = i;
                    else
                        cmdEnd = i + 1;
                    isPiped = FALSE;
                    execute(cmdBegin, cmdEnd, words, redirectStdin, redirectStdout, isDaemon);
                }
                cmdBegin = i + 1;
            }
        }
        for (i = 0; i < 3; i++) {
            dup2(redirectionOperators[i], i);
        }
    }
    free((void *) words);
}

void printLaunchMessage() {
    fprintf(stdout, "===============================WELCOME============================");
    fprintf(stdout, "\nTry to type a command\n");
}
/* Print the begining of a new line */
void printNewLinePrefix() {
    char hostname[LBUF];
    char *currentDirectory = getcwd(NULL, 0);

    gethostname(hostname, LBUF);
    if (strncmp(getenv("HOME"), currentDirectory, strlen(getenv("HOME"))) == 0) {
        fprintf(stdout, "%s@%s:~%s> ", getenv("LOGNAME"), hostname, currentDirectory);
    } else {
        fprintf(stdout, "%s@%s:%s> ", getenv("LOGNAME"), hostname, currentDirectory);
    }
    free((void *) currentDirectory);
}

int main(int N, char *P[]) {
    /* initialisations diverses */
    signal(SIGINT, SIG_IGN); /* on ignore l'interruption du clavier (Ctrl + C)*/
    signal(SIGTSTP, SIG_IGN);
    system("clear");

    printLaunchMessage();

    while (RUN) {
        env = environ;
        printNewLinePrefix();

        fflush(stdout);
        if (read_line(0, buf, LBUF) < 0)
            fprintf(stderr, "Command is too long, the line is limited to %d chars !\n", LBUF);
        else
            handleInput(buf);
    }
    fprintf(stdout, "-Exit-\n");
    return 0;
}
