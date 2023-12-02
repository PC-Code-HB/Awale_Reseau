#ifndef PARTIE_H
#define PARTIE_H


#include <joueur.h>

#define NB_PARTIES 10

#define NB_COUPS_MAX 1000

typedef struct {
  Joueur j1;
  Joueur j2;

  int nbCoups;
  int coups[NB_COUPS_MAX];
} Partie;


void to_string_partie(const Partie *p, char *string);
void to_object_partie(Partie *p, const char *string);


bool equals_partie(const Partie *p1, const Partie *p2);


#endif
