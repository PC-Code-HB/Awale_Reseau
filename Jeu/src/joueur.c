#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <joueur.h>


void create_joueur(Joueur *j, const char *pseudo, int etat)
{

  j->etat = etat;

  j->pseudo = (char*)malloc((strlen(pseudo)+1) * sizeof(char));
  strcpy(j->pseudo, pseudo);

}


void destroy_joueur(Joueur *j)
{
  free(j->pseudo);
  j->pseudo = NULL;
}


void to_string_joueur(const Joueur *j, char *buffer)
{

  strcpy(buffer, j->pseudo);
  strcat(buffer, ";");

  char temp[4];
  sprintf(temp, "%d", j->etat);
  strcat(buffer, temp);
}

void to_object_joueur(Joueur *j, char *string)
{
  const char *separators = ";";
  char *saveptr;
  char *strToken = strtok_r(string, separators, &saveptr);

  if (j->pseudo != NULL) {
    free(j->pseudo);
    
  }

  j->pseudo = (char*)malloc((strlen(strToken)+1) * sizeof(char));

  strcpy(j->pseudo, strToken);

  strToken = strtok_r(NULL, separators, &saveptr);

  j->etat = atoi(strToken);

}



bool equals_joueur(const Joueur *j1, const Joueur *j2)
{

  if (strcmp(j1->pseudo, j2->pseudo) == 0) {
    return true;
  }
  else {
    return false;
  }
}


void swap_joueur(Joueur *j1, Joueur *j2) {

  char *pseudo = j2->pseudo;
  int etat = j2->etat;

  j2->pseudo = j1->pseudo;
  j2->etat = j1->etat;

  j1->pseudo = pseudo;
  j1->etat = etat;

}

