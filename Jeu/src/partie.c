#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <partie.h>

#define TAILLE_MAX 1024

void to_string_partie(const Partie *p, char *string) {

  //j1-j2-nbCoups-coup0;coup1;coup2
  
  char temp[TAILLE_MAX];
  to_string_joueur(&(p->j1), temp);

  strcpy(string, temp);
  strcat(string, "-");

  to_string_joueur(&(p->j2), temp);
  strcat(string, temp);

  sprintf(string, "%s-%d-", string, p->nbCoups);

  if (p->nbCoups > 0) {
    sprintf(string,"%s%d", string, p->coups[0]);
  }
  for (int i = 1; i < p->nbCoups; i++) {
    sprintf(string, "%s;%d", string, p->coups[i]);
  }

}


void to_object_partie(Partie *p, const char *string) {

  char temp1[TAILLE_MAX];
  char temp2[TAILLE_MAX];
  char temp3[TAILLE_MAX];

  
  int nbCoups;
  sscanf(string, "%[^-]-%[^-]-%d-%s", temp1, temp2, &nbCoups, temp3);

  
  p->j1.pseudo = NULL;
  p->j2.pseudo = NULL;
  to_object_joueur(&(p->j1), temp1);
  to_object_joueur(&(p->j2), temp2);

  

  p->nbCoups = nbCoups;

  if (nbCoups > 0) {
    
    char *saveptr;
    char *split = strtok_r(temp3, ";", &saveptr);

    int i = 0;
    while (split != NULL && i < nbCoups) {
      p->coups[i] = atoi(split);

      split = strtok_r(NULL, ";", &saveptr);
      i++;
    }
  }
}

