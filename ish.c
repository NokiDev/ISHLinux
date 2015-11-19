/* ish.c : Ingesup Shell */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tools.h"

#define VERSION 0.40 /* A mettre a jour a chaque evolution */

#define LBUF 255
#define TRUE 1
#define FALSE 0

extern char ** environ;

int RUN=1; /* RUN doit être mis a zero pour stopper le shell */
char ** env;
char buf[LBUF];
char test[LBUF];
static int redir[3];

char** name, ** value;
int lire_params_env(char** cmd, int * n) 
{
	int i =0;
	name = (char **)malloc(sizeof(char*));
	value = (char **)malloc(sizeof(char*));
	while(cmd[*n]!=NULL)
	{
        char * d;
	    d=cmd[*n];
		name[i] = d;

		while(*d != '\0'){
			if(*d == '=')break;
			d++;
		}
		if(*d == '\0')
        {
            return 1;
        }
		*d = '\0';
		d++;
        if(*d == '\0')
        {
            free(name);
            return -1;
        }
		value[i] = d;
		
		i++;
        (*n)++;
		name = (char**)realloc((void*)name, sizeof(char*)* i+1);
		value = (char**)realloc((void*)value, sizeof(char*)*i+1);
	}
	name[i] = NULL;
	value[i] = NULL;
    return 0;
}


int commande_externe(char ** cmd, int* n, int res, int rss, int isDaemon)
{
    int pid;
    int i;
	pid = fork();
	if(pid == -1)
    {
        perror("fork"); 
		return -1;
    }
    if(pid == 0)
	{
        for(i =0; i<3; i++)
        {
            if(redir[i] != 0)
            {
                close(i); dup(redir[i]);
            }   
        }
        if(res) {dup2(res,0); close(res);}
        if(rss) {dup2(rss,1); close(rss);}
		execvpe(cmd[0], cmd, environ);
		perror(cmd[0]);
		exit(3);
	}
    if(rss == 0)
    {
        while(wait(&i) != pid);
    }
    if(rss) close(rss);
    if(res) close(rss);
    return 1;
}

/* cette fonction regarde si la commande c est interne et l'execute en 
   retournant VRAI sinon elle retourne FAUX */
int commande_interne(char** cmd, int * n)
{
	char *rep;
	if(strstr(cmd[*n], "=") != NULL)
	{
		int err = lire_params_env(cmd,n);
		if(err  == -1)
		{
			printf("Error it's [NAME=VALUE]\n");
		}
		else
		{
			int i;
			int j;
			for(j = 0; name[j] != NULL; j++)
			{
				setenv(name[j], value[j], 1);
			}
		    free((void*)name);
		    free((void*)value);
		}
		return 1;	
	}
	if (strcmp(cmd[*n],"exit") == 0) {// cas le la commande interne exit 
        (*n)++;
		RUN=0;
		return 1;
	}
	if (strcmp(cmd[*n],"vers") == 0) {
        (*n)++;
		printf("ish version %1.2f\n",(float)VERSION);
		return 1;
	}
	if (strcmp(cmd[*n],"pwd") == 0) {// Commande pwd
        (*n)++;
		rep = getcwd(NULL,0);
		printf("%s\n",rep);
		free(rep);
		return 1;
	}
	if (strcmp(cmd[*n],"pid") == 0) {
	(*n)++;
		if (cmd[*n] != NULL && strcmp(cmd[*n], "-a") == 0) {
			printf("PID: %i\tPPID: %i\tUID: %i\tSID: %i\n", getpid(), getppid(), getuid(), getsid(getpid()));
		} else {
	 		printf("%i\n", getpid());
		}
		return 1;
	}
	if (strcmp(cmd[*n],"echo") == 0) {
	(*n)++;
		while (cmd[*n] != NULL) {
			printf("%s ", cmd[*n]);
			(*n)++;
		}
		printf("\n");
		return 1;
	}
	if(strcmp(cmd[*n],"env") == 0){        
        (*n)++;
        int i =0;     
        int returnVal = 1;   
        while(env[i] != NULL){ i++;}
		char** tmpEnv = (char**)malloc(sizeof(char*)* i+1);
        for(i = 0; env[i] != NULL; i++){ tmpEnv[i] = strdup(env[i]);}       
        tmpEnv[i] = NULL;
        if(cmd[*n] != NULL)
        {
            int err = lire_params_env(cmd,n);
            if(err == -1)
            {
				printf("Error, use of env is : $env [NAME=VALUE] command\n");
                returnVal= err;
            }
            else
            {
                int j;
			    for(j = 0; name[j] != NULL; j++)
			    {
					printf("%s=%s", name[j], value[j]);
				    int t=0;
			        for(i = 0; tmpEnv[i] != NULL; i++)
				    {
					    if(strncmp(tmpEnv[i],name[j],strlen(name[j]))==0)
					    {
						    snprintf(test, sizeof test, "%s=%s", name[j], value[j]);
					        free(tmpEnv[i]);
                        	tmpEnv[i] = strdup(test);
					        t=1;
					    }
				    }
				    if(!t)
				    {
					    snprintf(test, sizeof test, "%s=%s", name[j], value[j]);
                        tmpEnv[i] = strdup(test);
                		tmpEnv = (char**)realloc((void*)tmpEnv, sizeof(char*)*(i+2));
                        tmpEnv[i+1] = NULL;
				    }
			    }
                free(name);
	            free(value);
				char ** tmpPtr = env;
				env = tmpEnv;
                if(err == 1){ returnVal = commande_interne(cmd,n);}
				env = tmpPtr;
            } 
		}
        if(returnVal == 1){      
            for(i =0; tmpEnv[i] != NULL; i++)
            {	
                printf("%s\n", tmpEnv[i]);
            }
        }
        for(i=0; tmpEnv[i] != NULL; i++)
        {
            free(tmpEnv[i]);
        }
        free(tmpEnv);
		return returnVal;
    }
	if (strcmp(cmd[*n],"cd") == 0) { // commande cd
		(*n)++;
        if (cmd[*n] == NULL) { //cd sans parametre 
			rep = getenv("HOME");//je recupere le contenu de la variable HOME 
		} else {  //je recupere le 1er parametre 
		    rep = cmd[*n];
		}
		if (chdir(rep) < 0) perror(rep);
		return 1;
	}
	return 0;
}

/* fonction qui determine si un caractere est un separateur */
int is_sepa(char c)
{
	if (c == ' ') return 1;
	if (c == '\t') return 1;
	if (c == '\n') return 1;
	return 0;
}

void execute(int cd, int cf, char**P, int res, int rss)
{
    int N = cf - cd + 1, it=0, Red=0, ired, flag, i, pid;
    char ** cmd;
    int n =0;
	printf("Command Debut : %d\n", cd);
	printf("Command Fin : %d\n", cf);
	for(i = 0; P[i]!= NULL; i++)
	{
		printf("P[%d] : %s\n", i, P[i]);
	}
    for(i = 0; i < 3; i++)
    {
         redir[i] = 0;    
    }
    
    cmd = (char **) malloc(sizeof(char*)* N);
    for(i = cd; i< cf; i++)
    {
        if (Red) 
        { /* traitement de la redirection */
            switch(Red)
            {
                case 1 : /* cas du > */
                    ired = 1;
                    flag = O_WRONLY | O_CREAT;
                    break;
                case 2 : /* cas du >> */
                    ired = 1;
                    flag = O_WRONLY | O_CREAT | O_APPEND;
                    break;
                case 3 : /* cas du < */
                    ired = 0;
                    flag = O_RDONLY;
                    break;
                default :
                    fprintf(stderr,"Erreur code Red = %d !\n", Red);
            }
            if ((redir[ired] = open(P[i],flag, 0644)) == -1) 
            {
               perror(P[i]);
            }
            Red=0;
        }
        else 
        { /* remplissage de tab */
            if (strcmp(P[i],">") == 0) { Red=1; continue; }
            if (strcmp(P[i],">>") == 0) { Red=2; continue; }
            if (strcmp(P[i],"<") == 0) { Red=3; continue; }
            if (strcmp(P[i],"2>") == 0) { Red=4; continue; }
            if (strcmp(P[i],"2>>") == 0) { Red=5; continue; }
            cmd[it++]=P[i];
        }
    }
    cmd[it]= NULL;
    /*printf("it = %d, Contenu de tab : ", it);
    for(i=0;cmd[i] != NULL;i++) {
        if (tab[i] == NULL) break; 
        printf("i :%d %s \n",i, cmd[i]);
    }*/
    printf("\n");
    if(!commande_interne(cmd, &n))commande_externe(cmd, &n, res, rss);
    free((void*)cmd);
}

char ** P;
int analyse_ligne(char *b)
{
    int i =0;
    char * d = b;
    char* tmpP[255];
	while(1)
    {
        /* on recherche le debut du premier mot */
	    while (*d != '\0') {
		    if (!is_sepa(*d)){break;}
		    d++;
	    }
        if (*d == '\0')return 0; /* pas de premier mot */
        tmpP[i++]=d;
        
        /* on recherche la fin du mot */
        while (*d != '\0') {
	        if (is_sepa(*d)) break;
	        d++;
        }
        /* si c'est la fin de la chaine on casse la boucle*/
		if(*d == '\0')break;
		*d = '\0';
		d++;
        
    }
    tmpP[i] = NULL;
    P =(char**)malloc(sizeof(char*)*(i+1));
    for(i =0; tmpP[i] != NULL; i++)
    {
        P[i] = tmpP[i];
    }
    P[i] = NULL;
	return i;
}


void traite_commande(char* buf)
{
    int N=analyse_ligne(buf);
	if (N==0) {
		//printf("in func traite_commande BUF : %s", buf);
		//printf("la commande est vide !\n");
	}
	else
	{
		int cd =0, cf = 0, i, res=0, rss=0, pip[2];
		for(i = 0; P[i] != NULL; i++)
		{
            rss = 0;
            if(i > 0 && strcmp(P[i-1], "|") != 0) res =0;
			if(P[i+1] == NULL || (strcmp(P[i], ";")== 0) || (strcmp(P[i], "|") == 0) || (strcmp(P[i], "&") == 0)
			{   
                if(strcmp(P[i], "|") == 0)
                {
                    cf = i;
                    if(pipe(pip) != 0)
                    {
                        perror("pipe"); exit(3);
                    }
                    res = pip[0];
                    rss = pip[1];
                    execute(cd,cf, P,0, rss);
                }
                else
                {
				    if(strcmp(P[i],";") == 0) cf = i;
				    else cf = i+1;
                    execute(cd, cf, P, res, 0);
				}
                cd = i+1;
			}			
		}
        free((void*)P);
	}
}

int main(int N, char *P[])
{
	/* initialisations diverses */
	//signal(SIGINT, SIG_IGN); /* on ignore l'interruption du clavier (Ctrl + C)*/
	signal(SIGTSTP, SIG_IGN);
	system("clear");
	printf("===============================WELCOME============================");
	printf("\nTry to type a command\n");

	while (RUN) {
		env = environ;
		char host[LBUF];
		char* rep = getcwd(NULL,0);
		gethostname(host, LBUF);
		if (strncmp(getenv("HOME"), rep, strlen(getenv("HOME"))) == 0) {
		 	printf("%s@%s:~%s> ", getenv("LOGNAME"), host, rep);
		} else {
		 	printf("%s@%s:%s> ", getenv("LOGNAME"), host, rep);
		}
		free((void*)rep);
		fflush(stdout);

		if (lire_ligne(0,buf,LBUF) < 0)
		printf("La taille de la ligne est limitee a %d car. !\n",LBUF);
		else traite_commande(buf);
	}
	printf("-Exit-\n");
	return 0;
}
