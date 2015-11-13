/* boite a outil contenant notre librairie privee */
#include <unistd.h>

#include "tools.h"

int lire_ligne(int fd, char* b, int t)
{
char * d, *f;
    d=b;
    f=b+t;
    while (d < f) {
       if (read(fd, d, 1) != 1) break;
       if (*d == '\n') break;
       d++;
    }
    if (d==f) { /* buffer trop petit */
       while (*b != '\n') read(fd,b,1);
       return -1;
    }
    *d = '\0';
    return (d - b);
}

