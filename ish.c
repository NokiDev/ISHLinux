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

#include "tools.h"
#include "commands.h"

char buf[LBUF];

static int redirectionOperators[3];

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

/*! \brief Check if the command is present internally in the program
 *  \param[in] cmd, line typed by the user. contains cmd, and args splitted by '\0'
 *  \param[in] n, current pointer on the cmd.
 *  \return BOOL, False if the command is not found or an error has been encountered, True otherwise
 */
BOOL internalCommand(char **cmd, int *n) {
    clearRedirectionOperators();

    return run_internal_command(cmd, n);
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
