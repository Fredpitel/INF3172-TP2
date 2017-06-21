/* 
 * File:   main.c
 * Author: ee691361
 *
 * Created on 23 novembre 2015, 19:22
 */

#include "main.h"

DIR* rep;
pthread_mutex_t mutexPipe;
Source* racine;
pid_t pidEnfant;
int objetPipe[2];
char nomRep[PATH_MAX];

int main(int argc, char** argv) {

  atexit(finProg);
  verifierParam(argc, argv);
  initialiserMutex();
  pipe(objetPipe);
  realpath(argv[1], nomRep);
  pidEnfant = fork();
  if (pidEnfant == 0) {
    chdir(argv[1]);
    lireRep();
  } else {
    surveillerPipe();
  }

  return 0;
}

void verifierParam(int argc, char** argv) {
  struct dirent* fichier;
  boolean trouve = FALSE;

  if (argc != 2) {
    printf("Ce programme doit recevoir un r�pertoire en param�tre.\n");
    exit(1);
  }

  rep = opendir(argv[1]);

  if (rep == NULL) {
    printf("Erreur lors de l'ouverture du r�pertoire %s.\n", argv[1]);
    exit(1);
  }

  while ((fichier = readdir(rep)) != NULL && trouve == FALSE) {
    if (strlen(fichier->d_name) >= 3 && verifierExtension(fichier->d_name)) {
      trouve = TRUE;
    }
  }

  closedir(rep);

  if (trouve == FALSE) {
    printf("Le r�pertoire ne contient aucun fichier source.\n");
    exit(1);
  }
}

boolean verifierExtension(char* nomFichier) {
  char extension[3];

  strcpy(extension, &nomFichier[strlen(nomFichier) - 2]);
  extension[3] = '\0';
  if (strcmp(extension, ".c") == 0) {
    return TRUE;
  }

  return FALSE;
}

void lireRep() {
  struct dirent* fichier;
  Source* ptrSource;

  while (1) {
    rep = opendir(nomRep);
    if (rep == NULL) {
      printf("Erreur lors de l'ouverture du repertoire %s.\n", nomRep);
    }
    while ((fichier = readdir(rep)) != NULL) {
      if ((strlen(fichier->d_name) >= 3) && (verifierExtension(fichier->d_name))) {
        if (verifierNouveau(fichier->d_name)) {
          ptrSource = creerMaillon(fichier);
          creerThread(ptrSource);
        }
      }
    }
    verifierSuppression();
    if (closedir(rep) != 0) {
      printf("Erreur lors de l'ouverture du repertoire %s.\n", nomRep);
    }
  }

  printf("THIS SHOULDN'T HAPPEN!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
}

void surveillerPipe() {
  char buffer[512];
  int c;

  while (1) {
    while ((c = read(objetPipe[0], buffer, sizeof (buffer)) != 0)) {
      buffer[c] = 0;
      printf("%s", buffer);
    }
  }
}

void initialiserMutex() {
  if (pthread_mutex_init(&mutexPipe, NULL) != 0) {
    printf("Erreur d'initialisation du mutex.\n");
    exit(1);
  }
}

Source* creerMaillon(struct dirent* fichier) {
  Source* ptrSource;
  Source* prochain;

  ptrSource = malloc(sizeof (Source));

  strcpy(ptrSource->nom, fichier->d_name);
  strcpy(ptrSource->nomObjet, ptrSource->nom);
  ptrSource->nomObjet[strlen(ptrSource->nomObjet) - 1] = 'o';
  strcpy(ptrSource->path, nomRep);
  strcat(ptrSource->path, "/");
  strcat(ptrSource->path, fichier->d_name);
  stat(ptrSource->path, &ptrSource->stats);
  ptrSource->compile = FALSE;
  ptrSource->modif = ptrSource->stats.st_mtim.tv_sec;

  if (racine == NULL) {
    racine = ptrSource;
  } else {
    prochain = racine;
    while (prochain->suivant != NULL) {
      prochain = prochain->suivant;
    }
    prochain->suivant = ptrSource;
  }

  return ptrSource;
}

void creerThread(Source* source) {
  int resultat;

  resultat = pthread_create(&source->thread, NULL, (void*) &compile, (void*) source);
  if (resultat != 0) {
    printf("Erreur lors de la creation d'un thread.\n");
    exit(1);
  }
}

void compile(void* ptrSource) {
  Source source = *(Source*) ptrSource;

  while (1) {
    if (access(source.nomObjet, F_OK) == -1 && source.compile == FALSE) {
      ecrirePipe(source.nom);
      source.compile = TRUE;
    } else {
      if (source.stats.st_mtim.tv_sec > source.modif) {
        source.modif = source.stats.st_mtim.tv_sec;
        ecrirePipe(source.nom);
      }
    }
  }
}

void ecrirePipe(char* nomFichier) {
  FILE* entree;
  FILE* stream;
  char commande[300];
  char buffer[512];

  strcat(commande, "gcc -Wall -c ");
  strcat(commande, nomFichier);

  if (!(entree = popen(commande, "r"))) {
    exit(1);
  }

  if (pthread_mutex_lock(&mutexPipe) != 0) {
    printf("Erreur avec le mutex.\n");
    exit(1);
  }

  stream = fdopen(objetPipe[1], "w");
  while (fgets(buffer, sizeof (buffer), entree) != NULL) {
    fputs(buffer, stream);
  }
  fclose(stream);

  if (pthread_mutex_unlock(&mutexPipe) != 0) {
    printf("Erreur avec le mutex.\n");
    exit(1);
  }
}

boolean verifierNouveau(char* nomFichier) {
  Source* ptrSource = racine;

  while (ptrSource != NULL) {
    if (strcmp(ptrSource->nom, nomFichier) == 0) {
      return FALSE;
    }
    ptrSource = ptrSource->suivant;
  }

  return TRUE;
}

void verifierSuppression() {
  Source* ptrSource = racine;

  while (ptrSource) {
    if (access(ptrSource->nom, F_OK) == -1) {
      tuerThread(ptrSource);
    }
    ptrSource = ptrSource->suivant;
  }
}

void tuerThread(Source* source) {
  int resultat;
  Source* ptrPrecedent = racine;

  resultat = pthread_kill(source->thread, SIGKILL);
  if (resultat != 0) {
    printf("Erreur lors de la terminaison d'un thread.\n");
    exit(1);
  }

  if (ptrPrecedent == source) {
    racine = source->suivant;
  } else {
    while (ptrPrecedent->suivant != source) {
      ptrPrecedent = ptrPrecedent->suivant;
    }
    ptrPrecedent->suivant = source->suivant;
  }

  free(source);
}

void finProg() {
  Source* ptrSource = racine;
  Source* ptrSuivant;

  printf("J'ai fait ma job");

  if ((kill(pidEnfant, SIGKILL)) != 0) {
    printf("Erreur lors de la terminaison du processus enfant.\n");
  }

  while (ptrSource) {
    ptrSuivant = ptrSource->suivant;
    free(ptrSource);
    ptrSource = ptrSuivant;
  }
}