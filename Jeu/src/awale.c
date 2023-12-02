/**
 * Code du jeu Awalé
 * Servira de base pour l'implémentation en réseau
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <awale.h>

/**
 * Initialisation de la partie :
 *  - plateau : 4 grains par case
 *  - sens de rotation aléatoire : 1=horaire / -1 = anti-horaire (cf affichage pour comprendre pourquoi)
 *  - scores des joueurs à 0
 */
void initPartie(int *plateau, int *score_joueur1, int *score_joueur2, int *sens_rotation) {

  //plateau
  for (int i=0; i<NB_CASES; ++i) {
    plateau[i] = 4;
    //plateau[i] = 10; //pour débug : ligne du dessus pour le vrai jeu
  }

  *score_joueur1 = 0;
  *score_joueur2 = 0;
  // *sens_rotation = (rand()/RAND_MAX < 0.5) ? (-1) : (1);
  *sens_rotation = -1; //pour débug : ligne du dessus pour le vrai jeu
}

/**
 * Parcours des cases et affichage ASCII
 * Affichage du jeu adapté en fonction du joueur
 * 
 * Joueur 1 :  
 * 6 7 8 9 10 11
 * 5 4 3 2 1  0
 * 
 * Joueur 2 :
 * 0  1  2 3 4 5
 * 11 10 9 8 7 6
 */
void afficherPlateau(int* plateau, int joueur) {
    
  int idx_row1, idx_row2;

  //indices des cases par lesquelles commencent les lignes à afficher
  idx_row1 = NB_CASES/2 * (2 - joueur);
  idx_row2 = NB_CASES/2 * joueur - 1;
    
  //affichage ligne par ligne
  printf("Point de vue du joueur %d\n", joueur);
  printf("Plateau de jeu :\n\n");
  for (int i=idx_row1; i<idx_row1+6; ++i){
    printf(" | %d", plateau[i]);
  }
  printf(" |\n");
  for (int i=0; i<NB_CASES/2; ++i){
    printf("-----");
  }
  printf("\n");
  for (int i=idx_row2; i>idx_row2-6; --i){
    printf(" | %d", plateau[i]);
  }
  printf(" |\n\n");
}

/**
 * Réalise une copie de plateau dans tmpPlateau
 */
static void copiePlateau(int *plateau1, int *plateau2) {
  for (int i=0; i<NB_CASES; ++i) {
    plateau2[i] = plateau1[i];
  }
}

/**
 * Renvoie 1 si la case est dans le camp du joueur
 * Renvoie 0 sinon
 */
static bool estDansCampJoueur(int joueur, int case_parcourue) {
  return (NB_CASES/2*joueur > case_parcourue) && (NB_CASES/2*(joueur-1) <= case_parcourue);
}

/**
 * Renvoie 1 si le joueur a au moins 1 pion dans son camp
 * Renvoie 0 sinon
 */
static bool aDesPionsDansSonCamp(int joueur, int *plateau) {

  int case_parcourue = NB_CASES/2*(joueur-1);
  while (plateau[case_parcourue] == 0
         && case_parcourue < NB_CASES/2*joueur)
    {
      case_parcourue++;
    }
  return (case_parcourue == NB_CASES/2*joueur) ? false : true;
}

/**
 * Renvoie le nombre de cases entre la case passée en paramètre et la fin du camp du joueur (cela dépend
 * du sens de rotation de la partie)
 */
static int nbCasesDansLeCamp(int case_selec, int joueur, int *plateau, int sens_rotation) {
  if (sens_rotation == 1) {
    return (NB_CASES/2*(joueur) - case_selec);
  } else {
    return case_selec + 1 - 6*(joueur-1);
  }
}

/**
 * Appelée après un coup pour savoir si la partie est terminée, et quel joueur l'emporte
 * Valeurs de retour : 
 *  - 0 : la partie continue
 *  - 1 : le joueur 1 gagne
 *  - 2 : le joueur 2 gagne
 *  - 3 : aucun des joueurs ne gagne
 */
int testFinPartie(int *plateau, int joueur, int sens_rotation, int score_joueur1, int score_joueur2) {

  int stopJeu; //pour les tests qui ont besoin de condition sur les 2 joueurs

  //vérification du score des joueurs
  if (score_joueur1 >= 25) {
    return 1;
  }

  if (score_joueur2 >= 25) {
    return 2;
  }

  //si un des deux joueurs n'a pas de graines, et que l'autre ne peut pas le nourrir
  if(!aDesPionsDansSonCamp(joueur, plateau) )
    {

      int coup[NB_CASES/2];
      int iCoup = 0;
      if (obligerNourrir(joueur == 1 ? 2 : 1, plateau, sens_rotation, coup, &iCoup)) {
        return 0;
      }
      else {
	
        /* printf("La partie se termine : le joueur %d n'a plus de graines dans son camp et son adversaire ne peut pas le nourrir\n"); */
        return joueur == 1 ? 2 : 1;
      }
      
    }

  //si il n'est plus possible de capturer de graines pour aucun des joueurs
  // if (!possibleCapturerGraines(1, plateau, sens_rotation) && !possibleCapturerGraines(2, plateau, sens_rotation))
  // {
  //     printf("La partie se termine : aucun des joueurs ne peut plus capturer de graines");
  //     return 3;
  // }

  return 0;
}


/**
 * Renvoie true si on est obligé de nourrir l'adversaire avec les coup possiblie pour nourrir
 **/
bool obligerNourrir(int joueur, int *plateau, int sens_rotation, int *coup, int *indexCoup) {

  if (aDesPionsDansSonCamp(joueur == 1 ? 2 : 1, plateau)) {
    return false;
  }
  
  int case_parcourue = NB_CASES/2*(joueur-1);
  *indexCoup = 0;
  while (case_parcourue < NB_CASES/2*(joueur))
    {
      if (plateau[case_parcourue] < nbCasesDansLeCamp(case_parcourue, joueur, plateau, sens_rotation)) {
        coup[*indexCoup] = case_parcourue % (NB_CASES/2);
        (*indexCoup)++;
      }
      case_parcourue++;
    }

  return true;
  
}

/**
 * Joue un coup : on enlève les pions de la case, on les répartit 1 par 1 dans les cases suivantes du
 * plateau (selon le sens de rotation)
 * Attention : la case est jouée depuis le pdv du joueur (case n°1 = celle tout à gauche, case n°6 = celle tout à droite)
 *  pas selon les indices du tableauq ui représente le plateau
 * 
 * prises : tableau listant les prises engendrées par ce coup
 * nbPrises : nb element du tableau (-1 si prises annulés car vide le camp adverse)
 *
 */
void jouerCoup(int case_jeu, int joueur, int *score_joueur, int sens_rotation, int *plateau, int *prises, int *nbPrises) {

  int nb_pions;
  int case_parcourue, case_depart;
  int tmpPlateau[NB_CASES];

  //check validité case
  if (case_jeu > 6 || case_jeu < 1)
    {
      /* printf("Case choisie non valide\n"); */
      return;
    }

  //conversion case en fonction du joueur : cf formule dans afficherPlateau (adaptée ici)
  case_depart = (NB_CASES/2 * joueur) - case_jeu;
  case_parcourue = case_depart;

  //déroulé du tour
  nb_pions = plateau[case_parcourue];
  plateau[case_parcourue] = 0;

  /* printf("case sélec : %d | nb pions = %d\n", case_parcourue, nb_pions); */

  //Egrenage le long du plateau
  //Si le coup vide le camp de l'adversaire, il est joué mais les graines ne sont pas prises
  while (nb_pions > 0)
    {
      int temp = ((case_parcourue + sens_rotation)%NB_CASES);
      case_parcourue = temp >= 0 ? temp : NB_CASES + temp; //la construction/affichage du plateau nous permet de faire ça facile

      //si plus de 11 graines : on saute la case de départ
      if (case_parcourue == case_depart) { continue; }

      /* printf("case sélec : %d | nb pions = %d\n", case_parcourue, plateau[case_parcourue]); */

      plateau[case_parcourue] = plateau[case_parcourue] + 1;
      nb_pions--;
    }

  //sauvegarde temporaire du plateau de jeu : en cas de non-validité on peut y revenir
  //faite ici parce que on a égrené les pions, et que cette opération se fait dans tous les cas
  copiePlateau(plateau, tmpPlateau);

  
  int tmp_sore_joueur = *score_joueur;

  //on vérifie si le joueur remporte des pions
  while (!estDansCampJoueur(joueur, case_parcourue)
         && (plateau[case_parcourue] == 2 || plateau[case_parcourue] == 3))
    {
      /* printf("Le joueur prend les pions : %d\n", plateau[case_parcourue]); */
      prises[*nbPrises] = plateau[case_parcourue];
      (*nbPrises) ++;
      
      *score_joueur += plateau[case_parcourue];
      plateau[case_parcourue] = 0;
      case_parcourue -= sens_rotation;

      //on vérifie que le coup n'a pas vidé le camp de l'adversaire
      //càd on est sur la dernière case du camp adverse et cette case est à 0
        
      if ( ((sens_rotation == 1 && case_parcourue == NB_CASES/2 * (2-joueur))
            || (sens_rotation == 0 && case_parcourue == NB_CASES/2 * (3-joueur) - 1))
           && plateau[case_parcourue] == 0)
        {
          /* printf("Au final aucune prise n'est faite  car le coup vide totalement le camp adverse\n"); */

          *nbPrises = -1;
          /* printf("Votre score actuel est : %d\n", tmp_sore_joueur); */
          copiePlateau(tmpPlateau, plateau);

          *score_joueur = tmp_sore_joueur;
        }
    }
}


/***************************************************************************************************************************/
/* /										   */
/* int main() {									   */
/* 										   */
/*   int plateau[NB_CASES];								   */
/*   int score_joueur1, score_joueur2, sens_rotation;						   */
/* 										   */
/*   int joueur = 1;									   */
/*   int choix;									   */
/* 										   */
/*   //initialisation des paramètres de la partie						   */
/*   initPartie(plateau, &score_joueur1, &score_joueur2, &sens_rotation);				   */
/* 										   */
/*   bool fin = false;									   */
/*   while (!fin) {									   */
/* 										   */
/*     //Demande de fin									   */
/*     printf("Voulez-vous continuer ? (Oui:0 , Non:1)");						   */
/*     scanf("%d", &fin);								   */
/*     if (fin) {									   */
/*       break;									   */
/*     }										   */
/* 										   */
/* 										   */
/*     //Affichage plateau								   */
/*     afficherPlateau(plateau, joueur);							   */
/* 										   */
/*     //Obliger de nourrir ?								   */
/*     int coup[NB_CASES / 2];								   */
/*     int iCoup;									   */
/*     if (obligerNourrir(joueur, plateau, sens_rotation, coup, &iCoup))				   */
/*       {										   */
/* 	printf("Votre adversaire est en famine, vous devez le nourrir, voici les coups possibles : \n");	   */
/* 	for (int i = 0; i < iCoup; i++)							   */
/* 	  {									   */
/* 	    printf("\t%d\n", coup[i]+1);							   */
/* 	  }									   */
/* 										   */
/* 	bool ok = false;								   */
/* 	while (!ok)									   */
/* 	  {									   */
/* 	    printf("Quel coup ? ");							   */
/* 	    scanf("%d", &choix);							   */
/* 										   */
/* 	    for (int i = 0; i < iCoup; i++)							   */
/* 	      {									   */
/* 		if (coup[i]+1 == choix)							   */
/* 		  {								   */
/* 		    ok = true;							   */
/* 		    break;								   */
/* 		  }								   */
/* 	      }									   */
/* 	  }									   */
/* 										   */
/*       }										   */
/* 										   */
/*     else										   */
/*       {										   */
/* 	bool ok = false;								   */
/* 	while (!ok)									   */
/* 	  {									   */
/* 	    printf("Quel coup ? ");							   */
/* 	    scanf("%d", &choix);							   */
/* 										   */
/* 	    if (choix >= 1 && choix <= 6)							   */
/* 	      {									   */
/* 		ok = true;								   */
/* 	      }									   */
/* 										   */
/* 	  }									   */
/*       }										   */
/* 										   */
/*     //Jouer coup									   */
/*     jouerCoup(choix, joueur, joueur == 1 ? &score_joueur1 : &score_joueur2, sens_rotation, plateau);		   */
/* 										   */
/*     //Ré-affichage du plateau								   */
/*     afficherPlateau(plateau, joueur);							   */
/* 										   */
/*     //test de victoire								   */
/*     int res = testFinPartie(plateau, joueur, sens_rotation, score_joueur1, score_joueur2);			   */
/* 										   */
/*     switch(res)									   */
/*       {										   */
/*       case 1:									   */
/* 	printf("Le joueur 1 a gagne\n");							   */
/* 	fin = true;									   */
/* 	break;									   */
/* 										   */
/*       case 2:									   */
/* 	printf("Le joueur 2 a gagne\n");							   */
/* 	fin = true;									   */
/* 	break;									   */
/*       }										   */
/* 										   */
/*     //Changement de joueur								   */
/*     joueur = joueur == 1 ? 2 : 1;							   */
/* 										   */
/*   }										   */
/* 										   */
/* 										   */
/* 										   */
/* 										   */
/* 										   */
/* 										   */
/* 										   */
/* 										   */
/*     exit(0);									   */
/* }										   */
/***************************************************************************************************************************/


