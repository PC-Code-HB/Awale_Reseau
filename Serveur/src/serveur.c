
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <serveur.h>


/**
 * Paramètres passés au lancement d'un thread de jeu
 */
typedef struct {
  SOCKET j1;
  SOCKET j2;
  int id; //index du thread dans le tableau
} ArgThreadGame;


/**
 * Message pour un thread jeu
 */
typedef struct {
  char buffer[BUF_SIZE];
  SOCKET sock;
} MessageGame;
  


/**
 * Permet de lire dans un thread jeu quand on reçoit un message d'une partie
 * verifie que la socket correspond bien à celle du joueur passés en value
 */
#define read_game(value) {				\
    sem_wait(semParties + argThread->id);			\
    while (messagesParties[argThread->id].sock != tabSock[value-1]) {	\
      sem_wait(semParties + argThread->id);			\
    }						\
    strcpy(bufferGame, messagesParties[argThread->id].buffer);	\
  }


//Tableau et nombre de clients connectés
Client clients[MAX_CLIENTS]; 
int nbClients = 0;

//Socket serveur (sur laquelle les clients communique avec lui)
SOCKET sock;

//Buffer dans lequel on met les messages lu par sock au fur et à mesure
char buffer[BUF_SIZE];

//Gestion des parties
Partie parties[NB_PARTIES];
int clientsInGame[MAX_CLIENTS]; //=id du thread dans lequel le serveur déroule la partie du joueur i ou -1 si ne joue pas

//Gestion des observateurs d'une partie
bool observateurs[NB_PARTIES][MAX_CLIENTS];


//Threads de parties
pthread_t threads_partie[NB_PARTIES];
sem_t semParties[NB_PARTIES];
MessageGame messagesParties[NB_PARTIES];
bool threadsLibres[NB_PARTIES]; //=true si thread i inutilisé


/**
 * Accepte la connection d'un client à la socket serveur
 * Enregistre ce nouveau client en créant le joueur associé avec le pseudo reçu
 *
 * En cas de pseudo déjà pris: annule la connexion
 */
static void connect_client(fd_set *rdfs, int *max_rdfs)
{
  
  SOCKADDR_IN csin = { 0 };
  int sinsize = sizeof csin;

  //On l'accepte
  int csock = accept(sock, (SOCKADDR *)&csin, (socklen_t*)&sinsize);
  if(csock == SOCKET_ERROR)
    {
      perror("accept()");
      return;
    }

  //on lit son profil qu'il nous a envoyé
  if(read_client(csock, buffer) == -1)
    {
      //Erreur donc on le deconnecte en ne l'enregistrant pas
      return;
    }

  Joueur joueurTemp;
  joueurTemp.pseudo = NULL;
  to_object_joueur(&(joueurTemp), buffer);

  //Recherche si pseudo existe deja
  for (int i = 0; i < nbClients; i++) {
    if (equals_joueur(&joueurTemp, &(clients[i].joueur))) {
      write_client(csock, "ACK/not_connected");
      return;
    }
  }
  

  //On recherche la valeur du plus grand descripteur
  *max_rdfs = csock > *max_rdfs ? csock : *max_rdfs;

  FD_SET(csock, rdfs);

  //On l'ajoute au tableau des clients
  Client c = { csock };
  c.joueur = joueurTemp;
  clients[nbClients] = c;
  nbClients++;

	  
  strncpy(buffer, "", BUF_SIZE-1);
  strncat(buffer, "SERV_INFO/new_joueur:", BUF_SIZE-1);

  char temp[BUF_SIZE];
  to_string_joueur(&(c.joueur), temp);
  strncat(buffer, temp, BUF_SIZE - 1 - strlen(buffer)-1);

  //On partage cette ajout à tout le monde
  send_message_to_all_clients(clients, c, nbClients, buffer, 1);

  
  //On envoie la confirmation de connexion
  write_client(c.sock, "ACK/connect");

  printf("[CLIENT] : new : %s\n", c.joueur.pseudo);

}

static void deconnect_client(int i)
{

  
  strcpy(buffer, "");
  strncat(buffer, "SERV_INFO/delete_joueur:", BUF_SIZE-1);

  char temp[BUF_SIZE];
  to_string_joueur(&(clients[i].joueur), temp);
  strncat(buffer, temp, BUF_SIZE - 1 - strlen(buffer)-1);

  //On annonce à tout le monde qu'il s'est déconnecté
  send_message_to_all_clients(clients, clients[i], nbClients, buffer, 1);

  //On ferme sa socket
  closesocket(clients[i].sock);
  //On le supprime du tableau
  remove_client(clients, i, &nbClients);

  //On met à jour les clientsInGame
  memmove(clientsInGame + i, clientsInGame + i + 1, (MAX_CLIENTS - i - 1) * sizeof(int));
  clientsInGame[MAX_CLIENTS-1] = -1;

  //On met à jour les observateurs
  for (int k = 0; k < NB_PARTIES; k++) {
    observateurs[k][i] = false;
    memmove(observateurs[k] + i, observateurs[k] + i + 1, (MAX_CLIENTS - i - 1) * sizeof(bool));
    observateurs[k][MAX_CLIENTS-1] = false;
  }


  printf("[CLIENT] : delete : %s\n", temp);

}

/**
 * Envoie au client i la liste des joueurs connectés au serveur
 * Format : |joueur1|joueur2|
 */
static void get_list_joueur(int i)
{

  strncpy(buffer, "|", BUF_SIZE-1);

  char buffer2[BUF_SIZE];
  for (int k = 0; k < nbClients; k++) {

    if (k == i) {
      continue;
    }
		      
    to_string_joueur(&(clients[k].joueur), buffer2);
    strcat(buffer, buffer2);
    strcat(buffer, "|");
		      

  }
  
  write_client(clients[i].sock, buffer);

  printf("[CLIENT] : get_list_joueur\n");
		    
}

/**
 * Envoie au client i la liste des parties en cours
 * Format : SERV_INFO/list_parties/|partie1|partie2|
 */
static void get_list_parties(int i)
{

  strncpy(buffer, "SERV_INFO/list_parties/|", BUF_SIZE-1);

  char buffer2[BUF_SIZE];
  for (int k = 0; k < NB_PARTIES; k++) {

    if (threadsLibres[k]) {
      continue;
    }
    
    to_string_partie(&(parties[k]), buffer2);

    strcat(buffer, buffer2);
    strcat(buffer, "|");
		    
  }
  
  write_client(clients[i].sock, buffer);

  printf("[CLIENT] : get_list_parties\n");
		    
}

/**
 * Transmet une demande de défi à la cible choisi par le client qui a fait la demande
 * change l'etat du client cible à 1
 */
static void transmettre_defi()
{

  //buffer à envoyer
  char buffer2[BUF_SIZE];
  strcpy(buffer2, "SERV_INFO/");
  strcat(buffer2, buffer);


  //recherche du client cible
  char *saveptr;
  char *split = strtok_r(buffer, ":|", &saveptr);
  split = strtok_r(NULL, ":|", &saveptr);

  
  Joueur temp[1];
  temp->pseudo = NULL;
  to_object_joueur(temp, split);

  for (int i = 0; i < nbClients; i++) {
    if (equals_joueur(temp, &(clients[i].joueur))) {
      
      //Envoie de la demande
      write_client(clients[i].sock, buffer2);

      usleep(5000);

      //Mis à jour de l'etat de la cible
      clients[i].joueur.etat = 1;
      update_etat_joueur(i);
      
      break;

    }
  }

  destroy_joueur(temp);

  printf("[DEFI] : demande transmise\n");

}


/**
 * Annonce à tout le monde que le joueur i a changé d'etat
 * Envoie son nouvel etat
 */
static void update_etat_joueur(int i) {


  char buffer2[BUF_SIZE];

  strcpy(buffer2, "SERV_INFO/update_etat_joueur:");

  char buffer3[BUF_SIZE];
  to_string_joueur(&(clients[i].joueur), buffer3);

  strcat(buffer2, buffer3);

  send_message_to_all_clients(clients, clients[i], nbClients, buffer2, 1);

  printf("[CLIENT] : update_etat_joueur : %s;%d\n", clients[i].joueur.pseudo, clients[i].joueur.etat);
  
}

/**
 * Le client qui a reçu une demande de défi l'a accepté.
 * On l'annonce donc à celui qui a fait la demande.
 * On change l'etat de ce dernier à 2
 */
static void accept_defi()
{

  //buffer à envoyer
  char buffer2[BUF_SIZE];
  strcpy(buffer2, "SERV_INFO/");
  strcat(buffer2, buffer);


  //recherche du client cible
  char *saveptr;
  char *split = strtok_r(buffer, ":|", &saveptr);
  split = strtok_r(NULL, ":|", &saveptr);
  

  Joueur temp[1];
  temp->pseudo = NULL;
  to_object_joueur(temp, split);

  for (int i = 0; i < nbClients; i++) {
    if (equals_joueur(temp, &(clients[i].joueur))) {

   
      //Envoie de l'acceptation
      write_client(clients[i].sock, buffer2);

      usleep(5000);
      
      //Mis à jour de l'etat de la cible
      clients[i].joueur.etat = 2;
      update_etat_joueur(i);

     
      break;

    }

  }

  destroy_joueur(temp);

  printf("[DEFI] : defi accepte\n");

}

/**
 * Le client qui a reçu une demande de défi l'a refusé.
 * On l'annonce donc à celui qui a fait la demande.
 * On change l'etat de ce dernier à 0.
 */
static void decline_defi()
{

  //buffer à envoyer
  char buffer2[BUF_SIZE];
  strcpy(buffer2, "SERV_INFO/");
  strcat(buffer2, buffer);


  //recherche du client cible
  char *saveptr;
  char *split = strtok_r(buffer, ":|", &saveptr);
  split = strtok_r(NULL, ":|", &saveptr);

  Joueur temp[1];
  temp->pseudo = NULL;
  to_object_joueur(temp, split);

  for (int i = 0; i < nbClients; i++) {
    if (equals_joueur(temp, &(clients[i].joueur))) {

      //Envoie du refus
      write_client(clients[i].sock, buffer2);

      usleep(5000);

      //Mis à jour de l'etat de la cible
      clients[i].joueur.etat = 0;
      update_etat_joueur(i);

      
      
      break;

    }
  }

  destroy_joueur(temp);

  printf("[DEFI] : defi refuse\n");

}


/**
 * Après un acceptation de défi, on lance un thread de jeu pour commencer une partie de awale
 *
 * Recherche les 2 clients concernés par la partie.
 * AJoute leurs sockets dans une structure ArgThreadGame
 * Créer une partie
 * Cherche un thread libre et le demarre avec ArgtThreadGame comme paramètre
 */
static void start_game()
{

  char *saveptr;
  char *split = strtok_r(buffer, ":|", &saveptr);
  split = strtok_r(NULL, ":|", &saveptr);

  Joueur j1[1];
  j1->pseudo = NULL;
  to_object_joueur(j1, split);

  split = strtok_r(NULL, ":|", &saveptr);

  Joueur j2[1];
  j2->pseudo = NULL;
  to_object_joueur(j2, split);


  ArgThreadGame *arg = (ArgThreadGame*)malloc(sizeof(ArgThreadGame));

  int iJ1, iJ2;
  for (int i = 0; i < nbClients; i++) {
    if (equals_joueur(j1, &(clients[i].joueur))) {
      arg->j1 = clients[i].sock;
      iJ1 = i;
    }
    if (equals_joueur(j2, &(clients[i].joueur))) {
      arg->j2 = clients[i].sock;
      iJ2 = i;
    } 
  }

  
  
  for (int i = 0; i < NB_PARTIES; i++) {
    if (threadsLibres[i]) {
      arg->id = i;
      clientsInGame[iJ1] = i;
      clientsInGame[iJ2] = i;

      parties[i].j1 = clients[iJ1].joueur;
      parties[i].j2 = clients[iJ2].joueur;
      parties[i].nbCoups = 0;
      
      pthread_create(threads_partie + i, NULL, game, arg);
      threadsLibres[i] = false;

      printf("[DEFI] : thread parti %d run\n", i);
      
      break;
    }
  }

  destroy_joueur(j1);
  destroy_joueur(j2);
  
}


/**
 * Procedure executé quand un thread jeu se termine
 * 
 * Libère la mémoire alloué
 * Met à jour les états des joueurs à 0
 * Annonce aux observateurs que la partie est terminé
 */
static void end_game(void *arg) {
  ArgThreadGame *argThread = (ArgThreadGame*)arg;

  threadsLibres[argThread->id] = true;

  
  for (int i = 0; i < nbClients; i++) {
    if (observateurs[argThread->id][i]) {
      write_client(clients[i].sock, "OBS/terminer");
      observateurs[argThread->id][i] = false;
    }
    if (clients[i].sock == argThread->j1 || clients[i].sock == argThread->j2) {
      clients[i].joueur.etat = 0;
      clientsInGame[i] = -1;
      update_etat_joueur(i);
      usleep(5000);
    }
  }

  printf("[DEFI] : thread parti %d termine\n", argThread->id);

  free(argThread);

}

/**
 * Procedure executé par le thread jeu
 *
 * Deroule une partie de awale en envoyant les bons messages aux bons joueurs au fur et à mesure
 * et en recevant les choix des joueurs au bon moment.
 * 
 * Dès qu'un coup a été joué, envoie le plateau et les 2 scores à tous les observateurs.
 *
 */
static void* game( void *arg) {

  //Installer la routine de nettoyage
  pthread_cleanup_push(end_game, arg);

  ArgThreadGame *argThread = (ArgThreadGame*)arg;

  SOCKET tabSock[2] = {argThread->j1, argThread->j2};

  int plateau[NB_CASES];
  int prises[NB_CASES];
  int nbPrises;
  
  int score_joueur1, score_joueur2, sens_rotation;

  int joueur = 1;
  int oppose = 2;
  int choix;

  int coup[NB_CASES / 2];
  int iCoup;

  char bufferGame[BUF_SIZE];
  char buf2 [BUF_SIZE];
  char buf3[BUF_SIZE];
  char *saveptr;
  char *split;

  //Attribution numero de joueur
  write_client(tabSock[joueur-1], "PARTIE/num_joueur:1");
  write_client(tabSock[oppose-1], "PARTIE/num_joueur:2");
  
  //initialisation des paramètres de la partie
  initPartie(plateau, &score_joueur1, &score_joueur2, &sens_rotation);

  bool fin = false;
  bool debut = true;
  int iter = 0;
  while (!fin) {

    iter++;
    
    //JOUEUR PAS SON TOUR:
    //envoie du plateau et message d'attente
    strcpy(bufferGame, "PARTIE/not_turn:");
    to_string_plateau(plateau, buf2);
    strcat(bufferGame, buf2);
    write_client(tabSock[oppose-1], bufferGame);

    //JOUEUR SON TOUR
    //Demande de fin
    if (!debut) {

      write_client(tabSock[joueur-1], "PARTIE/demande_fin");

      read_game(joueur);

      if (strstr(bufferGame, "PARTIE/fin") != NULL) {
        split = strtok_r(bufferGame, ":", &saveptr);
        split = strtok_r(NULL, ":", &saveptr);

        fin = atoi(split) == 1 ? true : false;
      }
    }
    if (fin) {

      write_client(tabSock[oppose-1], "PARTIE/abandon");
      usleep(5000);
      
      break;
    }

    if (iter == 2) {
      debut = false;
    }

    //Envoie plateau
    strcpy(bufferGame, "PARTIE/plateau:");
    to_string_plateau(plateau, buf2);
    strcat(bufferGame, buf2);
    write_client(tabSock[joueur-1], bufferGame);


    usleep(5000);

    //Obliger de nourrir ?
    if (obligerNourrir(joueur, plateau, sens_rotation, coup, &iCoup))
      {

        sprintf(bufferGame, "PARTIE/famine:%d|", iCoup);
	
        for (int i = 0; i < iCoup; i++)
          {
	sprintf(bufferGame, "%s;%d", bufferGame, coup[i]+1);
          }

        write_client(tabSock[joueur-1], bufferGame);

      }

    else
      {
        write_client(tabSock[joueur-1], "PARTIE/tous_coups");
      }

    //Attente reception coup du joueur
    read_game(joueur);

    if (strstr(bufferGame, "PARTIE/coup") != NULL) {
      split = strtok_r(bufferGame, ":", &saveptr);
      split = strtok_r(NULL, ":", &saveptr);
      
      choix = atoi(split);
    }
    else {
      printf("error : pas de coup reçu\n");
    }

    //Jouer coup
    nbPrises = 0;
    jouerCoup(choix, joueur, joueur == 1 ? &score_joueur1 : &score_joueur2, sens_rotation, plateau, prises, &nbPrises);
    parties[argThread->id].coups[ parties[argThread->id].nbCoups ++ ] = choix;

    //Partage avec les observateurs
    sprintf(bufferGame, "OBS/turn:%d:%d:%d:", joueur, score_joueur1, score_joueur2);
    to_string_plateau(plateau, buf2);
    strcat(bufferGame, buf2);
    for (int i = 0; i < nbClients; i++) {
      if (observateurs[argThread->id][i]) {
        write_client(clients[i].sock, bufferGame);
      }
    }

   
    
    
    //Transmettre les resultats du coup
    sprintf(bufferGame, "PARTIE/res_coup:%d:%d", joueur, nbPrises);
    if (nbPrises > 0) {
      sprintf(bufferGame, "%s|%d", bufferGame, prises[0]);
    }
    for (int i = 1; i < nbPrises; i++) {
      sprintf(bufferGame, "%s;%d", bufferGame, prises[i]);
    }

    write_client(tabSock[joueur-1], bufferGame);
    write_client(tabSock[oppose-1], bufferGame);

    usleep(5000);

    //test de victoire
    int res = testFinPartie(plateau, joueur, sens_rotation, score_joueur1, score_joueur2);

    if (res != 0) {
      to_string_plateau(plateau, buf2);
      sprintf(bufferGame, "PARTIE/terminer:%d|%s", res, buf2);
      write_client(tabSock[joueur-1], bufferGame);
      write_client(tabSock[oppose-1], bufferGame);

      fin = true;
    }

    
    //Changement de joueur
    joueur = joueur == 1 ? 2 : 1;
    oppose = oppose == 1 ? 2 : 1;

  }


  //Desinstalle la routine de nettoyage avec execution
  pthread_cleanup_pop(1);
  
}


/**
 * Coeur du serveur
 *
 * Surveille en permance la socket serveur (boucle principale)
 * En cas de nouveau message se charge d'appeler la bonne fonction ou de communiquer le message au bon thread.
 *
 * Sert de dispatcher en fonction de la nature et le contenu du message reçu
 */
static void app(void)
{
  sock = init_connection();
  
  
  int max = sock;
 
  //ensemble des descripteurs surveillés en lecture
  fd_set rdfs;

  while(1)
    {
      int i = 0;

      //vide l'ensemble
      FD_ZERO(&rdfs); 

      //ajoute le clavier (sert pour l'arret du serveu)
      FD_SET(STDIN_FILENO, &rdfs); 

      //ajoute la socket serveur
      FD_SET(sock, &rdfs); 

      //Ajoute la socket de chaque client
      for(i = 0; i < nbClients; i++)
        {
          FD_SET(clients[i].sock, &rdfs);
        }


      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
          perror("select()");
          exit(errno);
        }

      //S'il y a le clavier qui a changé
      if(FD_ISSET(STDIN_FILENO, &rdfs))
        {
          //on arrete le serveur
          break;
        }

      //Sinon si la socket serveur a changé ( = connection d'un client)
      else if(FD_ISSET(sock, &rdfs))
        {
          connect_client(&rdfs, &max);
	  
        }

      //Sinon c'est la socket d'un client qui a changé
      else
        {
          int i = 0;
          for(i = 0; i < nbClients; i++)
	{
	  //On cherche lequel
	  if(FD_ISSET(clients[i].sock, &rdfs))
	    {
	      Client client = clients[i];

	      //on lit son message
	      int c = read_client(clients[i].sock, buffer);

	      //S'il se deconnecte et qu'il jouait
	      if(c == 0 && clientsInGame[i] != -1)
	        {

	          int numT = clientsInGame[i];
	          for (int j = 0; j < MAX_CLIENTS; j++) {
		if (j != i && clientsInGame[j] == numT) {
		  write_client(clients[j].sock, "PARTIE/abandon");
		  break;
		}
	          }
		      
	          pthread_cancel(threads_partie[numT]);

	          pthread_join(threads_partie[numT], NULL);
	          deconnect_client(i);
	        }

	      //S'il se deconnecte sans jouer
	      else if (c == 0) {
	        deconnect_client(i);
	      }

	      //S'il est en train de jouer on partage son messsage au thread.
	      else if (clientsInGame[i] != -1) {
	        int numT = clientsInGame[i];
	        strcpy(messagesParties[numT].buffer, buffer);
	        messagesParties[numT].sock = clients[i].sock;

	        sem_post(semParties + numT);
	      }

	      //S'il veut la liste des joueurs connectes
	      else if (strcmp(buffer, "GET/list_joueur") == 0) {
	        get_list_joueur(i);
	      }

	      //S'il veut la liste des parties en cours 
	      else if (strcmp(buffer, "GET/list_parties") == 0) {
	        get_list_parties(i);
	      }

	      //S'il veut demander un joueur en defi
	      else if (strstr(buffer, "DEFI/new:") != NULL) {
	        transmettre_defi();
	        clients[i].joueur.etat = 1;
	        usleep(5000);
	        update_etat_joueur(i);
	      }

	      //S'il accepte le defi
	      else if (strstr(buffer, "DEFI/accept:") != NULL) {

	        char temp_buffer[BUF_SIZE];
	        strcpy(temp_buffer, buffer);
	        accept_defi();
	        clients[i].joueur.etat = 2;
	        usleep(5000);
	        update_etat_joueur(i);

	        strcpy(buffer, temp_buffer);

	        start_game();
	      }

	      //S'il refuse le defi
	      else if (strstr(buffer, "DEFI/decline:") != NULL) {
	        decline_defi();
	        clients[i].joueur.etat = 0;
	        usleep(5000);
	        update_etat_joueur(i);

	      }


	      //S'il demande à observer une partie
	      else if (strstr(buffer, "GET/add_observer") != NULL) {
	        clients[i].joueur.etat = 3;
	        update_etat_joueur(i);

	        char *saveptr;
	        char *split = strtok_r(buffer, ":", &saveptr);
	        split = strtok_r(NULL, ":", &saveptr);

	        char temp[BUF_SIZE];
	        for (int j = 0; j < NB_PARTIES; j++) {
	          to_string_partie(parties + j, temp);
	          if (strcmp(temp, split) == 0) {
		observateurs[j][i] = true;
		break;
	          }
	        }
	      }

	      //S'il arrete d'observer une partie
	      else if (strstr(buffer, "GET/remove_observer") != NULL) {
	        clients[i].joueur.etat = 0;
	        update_etat_joueur(i);

	        char *saveptr;
	        char *split = strtok_r(buffer, ":", &saveptr);
	        split = strtok_r(NULL, ":", &saveptr);

	        Partie p;
	        to_object_partie(&p, split);

	        for (int j = 0; j < NB_PARTIES; j++) {

	          if (threadsLibres[j]) {
		continue;
	          }
		      
	          if (equals_partie(&p, parties+j)) {
		observateurs[j][i] = false;
		break;
	          }
	        }
	      }
		    
	      break;
	    }
	}
        }
    }

  clear_clients(clients, nbClients);
  end_connection(sock);
}

int main(int argc, char **argv)
{

  //Initialisation des tableaux
  for (int i = 0 ; i < NB_PARTIES; i++) {
    threadsLibres[i] = true;
    sem_init(semParties + i, 0, 0);

    for (int j = 0; j < MAX_CLIENTS; j++) {
      observateurs[i][j] = false;
    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    clientsInGame[i] = -1;
  }
  
  
  init();

  app();

  end();

  
 
  return EXIT_SUCCESS;
}
