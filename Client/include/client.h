#ifndef CLIENT_H
#define CLIENT_H

#include <joueur.h>
#include <awale.h>
#include <partie.h>

#include <socket_client.h>


#define MAX_JOUEURS 100



static void app(const char *address, const char *name);


static void init_list_joueur();
static void* listen_info_serveur(void *arg);
static void* menu(void *arg);

static void* jeu(void *arg);
static void to_object_plateau(int *plateau, char *string);

static void* observeur(void *arg);





#endif /* guard */
