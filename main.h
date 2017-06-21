#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#define VALEUR_MAX 50

typedef enum {FALSE, TRUE} boolean;

typedef struct Source_t{
  char* nom;
  char* nomObjet;
  char* path;
  time_t modif;
  pthread_t thread;
  boolean compile;
  struct stat stats;
  struct Source_t* suivant;
} Source;

void verifierParam(int, char**);
boolean verifierExtension(char*);
void lireRep();
void surveillerPipe();
void initialiserMutex();
Source* creerMaillon(struct dirent*);
void creerThread(Source*);
void compile(void*);
void ecrirePipe(char*);
boolean verifierNouveau(char*);
void verifierSuppression();
void tuerThread(Source*);
void finProg();