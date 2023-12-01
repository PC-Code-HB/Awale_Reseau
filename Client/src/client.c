#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <pthread.h>
#include <semaphore.h>

#include <fcntl.h>

#include <client.h>


/* #define viderBufferScanf() {			\ */
/*     fflush(stdout);				\ */
/* } */

#define viderBufferScanf() {}

Joueur joueurs[MAX_JOUEURS];
int indexJoueurs = 0;
sem_t *semJoueurs;

SOCKET sock;
char buffer[BUF_SIZE];
char bufferJeu[BUF_SIZE];
char bufferMenu[BUF_SIZE];
char bufferObserveur[BUF_SIZE];


sem_t *semDefi;
sem_t *semGame;
sem_t *semOKGame;
sem_t *semListParties;
sem_t *semObserveur;


pthread_t *thread_listening;
pthread_t *thread_menu;
pthread_t *thread_jeu;
pthread_t *thread_observeur;



static void init_list_joueur()
{
  const char *sep = "|";

  char *saveptr;
  char *split = strtok_r(buffer, sep, &saveptr);

  while (split != NULL) {

     assert(indexJoueurs < MAX_JOUEURS);
    
     to_object_joueur(joueurs + indexJoueurs, split);
     indexJoueurs++;

     split = strtok_r(NULL, sep, &saveptr);
  }
  
  
}

static void* menu(void *arg) {

  int choix;
  bool fin = false;
  while (!fin) {

    

    printf("Que souhaitez-vous faire ? \n");

    printf("\t0.QUITTER\n");
    printf("\t1.Commencer une partie\n");
    printf("\t2.Actualiser les joueurs connectes\n");
    printf("\t3.Voir la liste des parties en cours\n");
    

    viderBufferScanf();
    scanf("%d", &choix);

    bool auMoinsUn = false;

    switch(choix) {

      //Defier qq un
    case 1:

      
      //Choix du joueur à affronter
     
      sem_wait(semJoueurs);
      for (int i = 1; i < indexJoueurs; i++) {
	if (joueurs[i].etat == 0) {
	  printf("\t%d. %s\n", i, joueurs[i].pseudo);
	  auMoinsUn = true;
	}
      }
      sem_post(semJoueurs);

      if (auMoinsUn) {
	 printf("Choisissez le joueur à affronter parmi ceux disponibles : ");
	 viderBufferScanf();
	 scanf("%d", &choix);
      }
      else {
	printf("Aucun joueur disponible\n");
	break;
      }

      if (choix >= 1 && choix < indexJoueurs && joueurs[choix].etat == 0) {

	
	strcpy(bufferMenu, "DEFI/new:");
	char buffer2[BUF_SIZE];
	joueurs[choix].etat = 1;
	to_string_joueur(joueurs + choix, buffer2);
	strcat(bufferMenu, buffer2);
	strcat(bufferMenu, "|");
	joueurs[0].etat = 1;
	to_string_joueur(joueurs, buffer2);
	strcat(bufferMenu, buffer2);

	/* DEFI/new:user_defie;etat|user_qui_fait_la_demande;etat */
	
	write_server(sock, bufferMenu);
      
	printf("Un defi a ete envoye à %s. On attend sa réponse.\n", joueurs[choix].pseudo);

        sem_wait(semDefi);

      }
      else {
	printf("Choix incorrect, merci de recommencer\n");
      }
      


      break;

    case 0:
      fin = true;
      break;

    case 2:
      printf("Voici la liste des joueurs connectes : \n");
  
      for (int i = 0; i < indexJoueurs; i++) {
	printf("\t%s ; %d\n", joueurs[i].pseudo, joueurs[i].etat);
      }

      break;

    case 3:

      write_server(sock, "GET/list_parties");

      sem_wait(semListParties);

      char *saveptr;
      char *split = strtok_r(bufferMenu, "|", &saveptr);

      char parties[NB_PARTIES][BUF_SIZE];

      int iter = 0;

      if (split != NULL) {
	printf("Voici la liste des parties en cours : \n");
      }
      else {
	printf("Aucune partie en cours\n");
      }
      
      while (split != NULL) {
	iter++;

	printf("\t%d.%s\n", iter, split);

	strcpy(parties[iter-1], split);

	split = strtok_r(NULL, "|", &saveptr);
      }

      while (1) {

	int num = 0;
	printf("Entrer le numero de celle que vous souhaitez regarder ou 0 pour revenir au menu principal : ");
	scanf("%d", &num);


	if (num < 0 || num > iter) {
	  continue;

	}

	if (num == 0) { break; }

	sprintf(bufferMenu, "GET/add_observer:%s", parties[num-1]);
	write_server(sock, bufferMenu);

	sem_wait(semJoueurs);
	joueurs[0].etat = 3;
	sem_post(semJoueurs);

	pthread_create(thread_observeur, NULL, observeur, parties[num-1]);
        pthread_cancel(*thread_menu);

	break;
	
      }
	

      break;
      
    }
    
  }

  write_server(sock, "");
  pthread_cancel(*thread_listening);
  
}

static void* observeur(void *arg) {

  pthread_join(*thread_menu, NULL);
  
  int plateau[NB_CASES];
  int score_joueur1;
  int score_joueur2;

  int joueur;

  char tempPlateau[BUF_SIZE];
  char temp[BUF_SIZE];

  char *tempPartie = (char*)arg;
  
  Partie partie;
  to_object_partie(&partie, tempPartie);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  printf("Vous pouvez utiliser la commande \"/quit\" pour quitter le mode observeur\n");


  while(1) {


    //S'il y a le clavier qui a changé
    int n = read(STDIN_FILENO, bufferObserveur, BUF_SIZE);

    if (n>0) {
      bufferObserveur[n] = 0;
    }

    if (n > 0 && strcmp(bufferObserveur, "/quit\n") == 0) {
      break;
    }
    else if (n > 0) {
      printf("\n[INFO_MODE_OBSERVEUR] : commande inconnu\n");
    }
	
    
    sem_wait(semObserveur);

    if (strstr(bufferObserveur, "OBS/turn") != NULL) {

      sscanf(bufferObserveur, "%[^:]:%d:%d:%d:%s",temp, &joueur, &score_joueur1, &score_joueur2, tempPlateau);

      to_object_plateau(plateau, tempPlateau);

      afficherPlateau(plateau, joueur);

      printf("%s a %d points\n", partie.j1.pseudo, score_joueur1);
      printf("%s a %d points\n", partie.j2.pseudo, score_joueur2);
      printf("\n");
      
    
    }

    else if (strstr(bufferObserveur, "OBS/terminer") != NULL) {
      printf("La partie est fini\n");

      pthread_create(thread_menu, NULL, menu, NULL);
      break;
    }

  }

  
  fcntl(STDIN_FILENO, F_SETFL, flags);
  pthread_create(thread_menu, NULL, menu, NULL);

}


static void* listen_info_serveur(void *arg)
{


  fd_set rdfs;
  
  while(1)
    {   

      //Vide l'ensemble
      FD_ZERO(&rdfs);

      //Ajout de la socket serveur
      FD_SET(sock, &rdfs);
      
      
      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
	{
	  perror("select()");
	  exit(errno);
	}

      
      //Si c'est le serveur qui nous a envoyé une info
      if(FD_ISSET(sock, &rdfs))
	{
	  int n = read_server(sock, buffer);

	  //Deconnexion du serveur
	  if(n == 0)
	    {
	      pthread_cancel(*thread_menu);
	      pthread_cancel(*thread_jeu);
	      printf("Server disconnected !\n");
	      
	      break;
	    }

	  //New joueur
	  else if (strstr(buffer, "SERV_INFO/new_joueur") != NULL) {

	    char *saveptr;
	    char *split = strtok_r(buffer, ":", &saveptr);
	    split = strtok_r(NULL, ":", &saveptr);

	    assert(indexJoueurs < MAX_JOUEURS);
	    sem_wait(semJoueurs);
	    to_object_joueur(joueurs + indexJoueurs, split);
	    indexJoueurs++;
	    sem_post(semJoueurs);

	    #ifdef DEBUG
	    printf("\n[SERV_INFO] : Ajout du joueur : %s\n", joueurs[indexJoueurs-1].pseudo);
	    #endif
	    
	  }

	  //Delete joueur
	  else if (strstr(buffer, "SERV_INFO/delete_joueur") != NULL) {


	    char *saveptr;
	    char *split = strtok_r(buffer, ":", &saveptr);
	    split = strtok_r(NULL, ":", &saveptr);

	    Joueur joueur_temp[1];
	    joueur_temp->pseudo = NULL;
	    to_object_joueur(joueur_temp, split);


	    bool finded = false;
	    sem_wait(semJoueurs);
	    for (int i = 0; i < indexJoueurs; i++) {

	      if (equals_joueur(joueur_temp, joueurs + i)) {

		swap_joueur(joueurs + indexJoueurs -1, joueurs+i);
		finded = true;
		break;
	      }
	      
	    }

	    assert(finded == true);

	    destroy_joueur(joueurs + indexJoueurs -1);
	    indexJoueurs--;


	    sem_post(semJoueurs);

	    #ifdef DEBUG
	    printf("\n[SERV_INFO] : Suppression du joueur : %s\n", joueur_temp->pseudo);
	    #endif
	    
	  }

	  //Demande de defi
	  else if (strstr(buffer, "SERV_INFO/DEFI/new") != NULL) {

	    char *saveptr;
	    char *split = strtok_r(buffer, "|", &saveptr);
	    split = strtok_r(NULL, "|", &saveptr);
	

	    printf("\n\nDemande de defi de la part de %s\n", split);

	    pthread_cancel(*thread_menu);

	    printf("Vous accepter le défi ? Oui:1 | Non:2 -> ");
	    int choix;
	    viderBufferScanf();
	    scanf("%d", &choix);

	    if (choix != 1 && choix != 2) {
	      choix = 2;
	    }

	    if (choix == 1) {
	      
	      strcpy(buffer, "DEFI/accept:");
	      strcat(buffer, split);
	      strcat(buffer, "|");

	      joueurs[0].etat = 2;
	      char buffer2[BUF_SIZE];
	      to_string_joueur(joueurs, buffer2);
	      strcat(buffer, buffer2);

	      write_server(sock, buffer);

	      printf("Defi accepte\nLa partie COMMENCE\n\n\n");

	      pthread_create(thread_jeu, NULL, jeu, NULL);

	    }
	    
	    else {

	      strcpy(buffer, "DEFI/decline:");
	      strcat(buffer, split);
	      strcat(buffer, "|");

	      char buffer2[BUF_SIZE];
	      to_string_joueur(joueurs, buffer2);
	      strcat(buffer, buffer2);

	      write_server(sock, buffer);

	      printf("Defi refuse\nRetour au menu\n\n\n");

	      pthread_create(thread_menu, NULL, menu, NULL);

	    }

	  }

	  //Acceptation de defi
	  else if (strstr(buffer, "SERV_INFO/DEFI/accept:") != NULL) {
	    char *saveptr;
	    char *split = strtok_r(buffer, "|", &saveptr);
	    split = strtok_r(NULL, "|", &saveptr);

	    joueurs[0].etat = 2;
	    
	    printf("Defi à %s accepte\nDebut de la partie\n\n\n", split);

	    sem_post(semDefi);

	    pthread_cancel(*thread_menu);
	    pthread_create(thread_jeu, NULL, jeu, NULL);

	  }

	  //Refus de defi
	  else if (strstr(buffer, "SERV_INFO/DEFI/decline:") != NULL) {
	    char *saveptr;
	    char *split = strtok_r(buffer, "|", &saveptr);
	    split = strtok_r(NULL, "|", &saveptr);

	    joueurs[0].etat = 0;

	    printf("Defi à %s refuse\nRetour au menu\n\n\n", split);

	    sem_post(semDefi);

	  }

	  //Update etat joueur
	  else if (strstr(buffer, "SERV_INFO/update_etat_joueur:") != NULL) {


	    char *saveptr;
	    char *split = strtok_r(buffer, ":", &saveptr);
	    split = strtok_r(NULL, ":", &saveptr);


	    Joueur temp[1];
	    temp->pseudo = NULL;
	    to_object_joueur(temp, split);


	    for (int i = 0; i < indexJoueurs; i++) {
	      if (equals_joueur(temp, joueurs +i)) {
		joueurs[i].etat = temp->etat;
		break;
	      }
	    }

	    #ifdef DEBUG
	    printf("\n[SERV_INFO]: Update etat du joueur : %s;%d\n", temp->pseudo, temp->etat);
	    #endif


	  }

	  //PARTIE en cours
	  else if (strstr(buffer, "PARTIE/") != NULL) {

	    sem_wait(semOKGame);

	    strcpy(bufferJeu, buffer);
	    
	    sem_post(semGame);

	  }

	  //OBS en cours
	  else if (strstr(buffer, "OBS/") != NULL) {

	    strcpy(bufferObserveur, buffer);
	    
	    sem_post(semObserveur);

	  }

	  //Reception liste partie
	  else if (strstr(buffer, "SERV_INFO/list_parties")!= NULL) {

	    char *saveptr;
	    char *split = strtok_r(buffer, "/", &saveptr);
	    split = strtok_r(NULL, "/", &saveptr);
	    split = strtok_r(NULL, "/", &saveptr);

	    strcpy(bufferMenu, split);

	    sem_post(semListParties);

	  }
 	  
	  
	}
    }

      

}


static void to_object_plateau(int *plateau, char *string) {

  char *saveptr;
  char *split = strtok_r(string, ";", &saveptr);

  int i = 0;
  while (split != NULL && i < NB_CASES) {
    plateau[i] = atoi(split);

    split = strtok_r(NULL, ";", &saveptr);
    i++;
  }
  
}

static void* jeu(void *arg) {

  usleep(100000);

  char *saveptr;
  char *split;

  int numJoueur;
  int plateau[NB_CASES];

  bool end = false;
  while(!end) {

    sem_wait(semGame);

    //Attribution numero de joueur
    if (strstr(bufferJeu, "PARTIE/num_joueur:") != NULL) {

      split = strtok_r(bufferJeu, ":", &saveptr);
      split = strtok_r(NULL, ":", &saveptr);

      numJoueur = atoi(split);

      #ifdef DEBUG
      printf("\n[PARTIE] : Attribution du numero de joueur : %d\n", numJoueur);
      #endif
      
    }

    //pas son tour
    else if (strstr(bufferJeu, "PARTIE/not_turn") != NULL) {
      split = strtok_r(bufferJeu, ":", &saveptr);
      split = strtok_r(NULL, ":", &saveptr);

      to_object_plateau(plateau, split);
      afficherPlateau(plateau, numJoueur);

      printf("\nMerci de patienter le temps que votre adversaire joue.\n");
      
    }

      

    //Demande fin
    else if ( strcmp(bufferJeu, "PARTIE/demande_fin") == 0) {

      int fin = -1;
      while (fin != 0 && fin != 1) {
	printf("Voulez-vous continuer ? (Oui:0 , Non:1)");
	viderBufferScanf();
	scanf("%d", &fin);
      }

      if (fin == 1) {
	sem_wait(semJoueurs);
	joueurs[0].etat = 0;
	sem_post(semJoueurs);

	printf("\nVous avez abandonné la partie\n");
	
	end = true;
      }

      sprintf(bufferJeu, "PARTIE/fin:%d", fin);
      write_server(sock, bufferJeu);
      
    }

    //Plateau
    else if (strstr(bufferJeu, "PARTIE/plateau") != NULL) {
      split = strtok_r(bufferJeu, ":", &saveptr);
      split = strtok_r(NULL, ":", &saveptr);

      to_object_plateau(plateau, split);
      afficherPlateau(plateau, numJoueur);
    }

    //Famine : restriction des coups possibles
    else if (strstr(bufferJeu, "PARTIE/famine") != NULL) {

      split = strtok_r(bufferJeu, ":|;", &saveptr);
      split = strtok_r(NULL, ":|;", &saveptr);

      int nbCoup = atoi(split);
      int i = 0;
      int coup[NB_CASES/2];

      split = strtok_r(NULL, ":|;", &saveptr);
      printf("Votre adversaire est en famine, vous devez jouer un de ces coups pour le nourrir : \n");
      while (split != NULL && i < nbCoup) {
	coup[i] = atoi(split);

	printf("\t%d\n", coup[i]);

	split = strtok_r(NULL, ":|;", &saveptr);
	i++;

      }

      bool ok = false;
      int choix;
      while (!ok)
	{
	  printf("Quel coup ? ");
	  viderBufferScanf();
	  scanf("%d", &choix);

	  for (int i = 0; i < nbCoup; i++)
	    {
	      if (coup[i]+1 == choix)
		{
		  ok = true;
		  break;
		}
	    }
	}

      sprintf(bufferJeu, "PARTIE/coup:%d", choix);
      write_server(sock, bufferJeu);
     
      
    }

    //Pas de famine : tous les coups sont possibles
    else if (strstr(bufferJeu, "PARTIE/tous_coups") != NULL) {
      bool ok = false;
      int choix;
      while (!ok)
	{
	  printf("Quel coup ? ");
	  viderBufferScanf();
	  scanf("%d", &choix);

	  if (choix >= 1 && choix <= 6)
	    {
	      ok = true;
	    }

	}

      sprintf(bufferJeu, "PARTIE/coup:%d", choix);
      write_server(sock, bufferJeu);
    }

    //Partie termine
    else if (strstr(bufferJeu, "PARTIE/terminer") != NULL) {

      split = strtok_r(bufferJeu, ":|", &saveptr);
      split = strtok_r(NULL, ":|", &saveptr);

      int res = atoi(split);

      if (res == numJoueur) {
	printf("Vous avez GAGNE !!!\n");
      } else {
	printf("Vous avez PERDU...\n");
      }

      split = strtok_r(NULL, ":|", &saveptr);
      to_object_plateau(plateau, split);

      printf("Voici le plateau à l'état final : \n\n");
      afficherPlateau(plateau, numJoueur);

      end = true;

      sem_wait(semJoueurs);
      joueurs[0].etat = 0;
      sem_post(semJoueurs);
    }

    //Abandon de l'adversaire
    else if (strstr(bufferJeu, "PARTIE/abandon") != NULL) {
      sem_wait(semJoueurs);
      joueurs[0].etat = 0;
      sem_post(semJoueurs);

      printf("Abandon du joueur adverse\n");

      end = true;
    }

    //Resultat coup
    else if (strstr(bufferJeu, "PARTIE/res_coup") != NULL) {

      split = strtok_r(bufferJeu, ":|;", &saveptr);
      split = strtok_r(NULL, ":|;", &saveptr);

      int coupJoueur = atoi(split);

      split = strtok_r(NULL, ":|;", &saveptr);

      int nbPrises = atoi(split);
      int prises[NB_CASES];
      
      if (nbPrises > 0) {
	split = strtok_r(NULL, ":|;", &saveptr);

	int indexPrises = 0;
	while (split != NULL) {
	  prises[indexPrises] = atoi(split);
	  indexPrises++;

	  split = strtok_r(NULL, ":|;", &saveptr);
	}

      }
      
      //coup du joueur
      if (numJoueur == coupJoueur) {
	if (nbPrises == 0) {
	  printf("Vous n'avez fait de prises avec ce coup\n");
	}
	else if (nbPrises == -1) {
	  printf("Au final aucune prise n'est faite  car le coup vide totalement le camp adverse\n");
	}
	else {
	  for (int i = 0; i < nbPrises; i++) {
	    printf("Vous avez pris : %d\n", prises[i]);
	  }
	}
      }
      //coup de l'adversaire
      else {
	if (nbPrises == 0) {
	  printf("Votre adversaire n'a pas fait de prises avec son coup\n");
	}
	else if (nbPrises == -1) {
	  printf("Au final aucune prise n'a été faite par votre adversaire car cela vidait totalement votre camp\n");
	}
	else {
	  for (int i = 0; i < nbPrises; i++) {
	    printf("On vous a pris : %d\n", prises[i]);
	  }
	}
      }
      
    }

    sem_post(semOKGame);

  }

    
  usleep(10000);
  pthread_create(thread_menu, NULL, menu, NULL);

}


/**
 * Coeur d'un client
 */
static void app(const char *address, const char *name)
{
  //Connexion au serveur;
  sock = init_connection(address);

  //Creation du joueur
  create_joueur(joueurs + indexJoueurs, name, 0);
  to_string_joueur(joueurs + indexJoueurs, buffer);
  indexJoueurs++;
  
  //Envoi de son profil de joueur
  write_server(sock, buffer);

  //Reception de la confirmation
  read_server(sock, buffer);

  if (strcmp(buffer, "ACK/connect") != 0) {
    printf("Erreur de connexion: surement le même pseudo que quelqu'un d'autre\n");
    exit(0);
  }

  printf("connect ok to %s\n", address);


  //Demande de la liste des joueurs connectés
  strncpy(buffer, "GET/list_joueur", BUF_SIZE-1);
  write_server(sock, buffer);

  //Reception de la liste
  read_server(sock, buffer);
  init_list_joueur();

  printf("init ok\n");


  //Creation des semaphores
  int r;
  semJoueurs = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semJoueurs, 0, 1);

  semDefi = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semDefi, 0, 0);

  semGame = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semGame, 0, 0);

  semOKGame = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semOKGame, 0, 1);

  semListParties = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semListParties, 0, 0);

  semObserveur = (sem_t*)malloc(sizeof(sem_t));
  r = sem_init(semObserveur, 0, 0);
  
  //Creation du thread d'écoute serveur
  thread_listening = (pthread_t*) malloc(sizeof(pthread_t));
  r = pthread_create(thread_listening, NULL, listen_info_serveur, NULL);

  if (r) {
    perror("creation thread_listening\n");
    exit(0);
  }
  

  //Creation du thread menu
  thread_menu = (pthread_t*) malloc(sizeof(pthread_t));
  r = pthread_create(thread_menu, NULL, menu, NULL);

  if (r) {
    perror("creation thread_menu\n");
    exit(0);
  }

  //Creation du thread jeu
  thread_jeu = (pthread_t*) malloc(sizeof(pthread_t));

  //Creation du thread observeur
  thread_observeur = (pthread_t*) malloc(sizeof(pthread_t));
  

  //FIN
  pthread_join(*thread_listening, NULL);
  
  free(semJoueurs);
  free(semDefi);
  free(semGame);
  free(semObserveur);
  free(semListParties);
  free(semOKGame);
  free(thread_listening);
  free(thread_menu);
  free(thread_jeu);
  free(thread_observeur);


  end_connection(sock);
}


int main(int argc, char **argv)
{
  if(argc < 2)
    {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
    }
  
  init();

  app(argv[1], argv[2]);

  end();

  return EXIT_SUCCESS;
}
