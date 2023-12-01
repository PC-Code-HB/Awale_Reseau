#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#include <socket_client.h>

//Pour les sockets sous Windows
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
 * Initialise la socket
 */
int init_connection(const char *address)
{

  //Création de la socket
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = { 0 };
  struct hostent *hostinfo;

  if(sock == INVALID_SOCKET)
    {
      perror("socket()");
      exit(errno);
    }

  //On recupère les informations de l'hôte auquel on veut se connecter
  hostinfo = gethostbyname(address);
  if (hostinfo == NULL)
    {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
    }

  //On définit les propriétés de l'interface
  sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  
  //On fait une demande de connection
  if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
      perror("connect()");
      exit(errno);
    }


  return sock;
}

/**
 * Ferme la socket
 */
void end_connection(int sock)
{
  closesocket(sock);
}

/**
 * Lit un message reçu
 */
int read_server(SOCKET sock, char *buffer)
{
  int n = 0;

  if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
    {
      perror("recv()");
      exit(errno);
    }

  buffer[n] = 0;

  return n;
}

/**
 * Envoie un message
 */
void write_server(SOCKET sock, const char *buffer)
{
  if(send(sock, buffer, strlen(buffer), 0) < 0)
    {
      perror("send()");
      exit(errno);
    }
}
