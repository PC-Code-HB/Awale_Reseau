#ifndef AWALE_H
#define AWALE_H

#define NB_CASES 12


void initPartie(int *plateau, int *score_joueur1, int *score_joueur2, int *sens_rotation);

void afficherPlateau(int* plateau, int joueur);

int testFinPartie(int *plateau, int joueur, int sens_rotation, int score_joueur1, int score_joueur2);

bool obligerNourrir(int joueur, int *plateau, int sens_rotation, int *coup, int *indexCoup);

void jouerCoup(int case_jeu, int joueur, int *score_joueur, int sens_rotation, int *plateau, int *prises, int *nbPrises);




#endif
