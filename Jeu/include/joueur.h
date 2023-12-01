#ifndef JOUEUR_H
#define JOUEUR_H

#include <stdbool.h>

typedef struct Joueur {
  char *pseudo;
  int etat;

  /**
   * 0 : dans le menu
   * 1 : a envoye un defi et attend la reponse
   * 2 : en partie
   * 3 : en mode observateur
   */
  
} Joueur;


//Construction
void create_joueur(Joueur *j, const char *pseudo, int etat);
void destroy_joueur(Joueur *j);


//Serialize
void to_object_joueur(Joueur *j, char *string);
void to_string_joueur(const Joueur *j, char *string);

//Compare
bool equals_joueur(const Joueur *j1, const Joueur *j2);

void swap_joueur(Joueur *j1, Joueur *j2);




#endif
