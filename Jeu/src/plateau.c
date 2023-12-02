#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <plateau.h>


void to_string_plateau(const int *plateau, char *string) {
  strcpy(string, "");
  sprintf(string, "%d", plateau[0]);
  for (int i = 1; i < NB_CASES; i++) {
    sprintf(string, "%s;%d", string, plateau[i]);
  }
}

void to_object_plateau(int *plateau, char *string) {

  char *saveptr;
  char *split = strtok_r(string, ";", &saveptr);

  int i = 0;
  while (split != NULL && i < NB_CASES) {
    plateau[i] = atoi(split);

    split = strtok_r(NULL, ";", &saveptr);
    i++;
  }
  
}
