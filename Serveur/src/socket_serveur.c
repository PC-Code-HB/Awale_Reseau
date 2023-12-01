#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <socket_serveur.h>


//Pour les sockets sur Windows
void init(void)
{
#ifdef WIN32
  WSADATA wsa;
  int err = WSAStartup(MAKEWORD(2, 2), &wsa);
  if(err < 0)
    {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
    }
#endif
}

void end(void)
{
#ifdef WIN32
  WSACleanup();
#endif
}



/**
 * Ferme toutes les sockets des clients
 */
void clear_clients(Client *clients, int actual)
{
  int i = 0;
  for(i = 0; i < actual; i++)
    {
      closesocket(clients[i].sock);
    }
}

/**
 * Enlève un client du tableau
 */
void remove_client(Client *clients, int to_remove, int *actual)
{

  //Destroy joueur
  destroy_joueur(&(clients[to_remove].joueur));
  
  /* we remove the client in the array */
  memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
  
  /* number client - 1 */
  (*actual)--;
}

/**
 * Partage un message envoyé par un client à tous les autres
 */
void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
  int i = 0;
  char message[BUF_SIZE];
  
  for(i = 0; i < actual; i++)
    {
      message[0] = 0;
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
	{
	  if(from_server == 0)
	    {
	      strncpy(message, sender.joueur.pseudo, BUF_SIZE - 1);
	      strncat(message, " : ", sizeof message - strlen(message) - 1);
	    }
	  strncat(message, buffer, sizeof message - strlen(message) - 1);
	  write_client(clients[i].sock, message);
	}
    }
}

/**
 * Initialise la socket serveur
 */
int init_connection(void)
{

  //Création de la socket serveur utilisant des adresses IPV4 et le protocole TCP
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); 
  SOCKADDR_IN sin = { 0 };

  if(sock == INVALID_SOCKET)
    {
      perror("socket()");
      exit(errno);
    }

  //Définition des propriétés de l'interface
  sin.sin_addr.s_addr = htonl(INADDR_ANY); //on accepte n'importe quel adresse de connexion
  sin.sin_port = htons(PORT); //on écoute le port numéro PORT
  sin.sin_family = AF_INET; //on utilise l'ipv4

  //Liaison de l'interface à la socket
  if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
      perror("bind()");
      exit(errno);
    }

  //On active l'écoute les demandes de connexions (au plus MAX_CLIENTS connectés)
  if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
      perror("listen()");
      exit(errno);
    }

  return sock;
}

/**
 * Ferme la socket sock
 */
void end_connection(int sock)
{
  closesocket(sock);
}

/**
 * Lit un message reçu depuis la socket sock
 */
int read_client(SOCKET sock, char *buffer)
{
  int n = 0;

  if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
    {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
    }

  buffer[n] = 0;

  return n;
}

/**
 * Envoie un message à la socket sock
 */
void write_client(SOCKET sock, const char *buffer)
{
  if(send(sock, buffer, strlen(buffer), 0) < 0)
    {
      perror("send()");
      exit(errno);
    }
}
