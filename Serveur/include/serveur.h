#ifndef SERVER_H
#define SERVER_H



#include <joueur.h>
#include <awale.h>
#include <partie.h>

#include <socket_serveur.h>




static void app(void);

static void connect_client(fd_set *rdfs, int *max_rdfs);
static void deconnect_client(int i);
static void get_list_joueur(int i);
static void get_list_parties(int i);


static void transmettre_defi();
static void update_etat_joueur(int i);
static void accept_defi();
static void decline_defi();

static void start_game();
static void* game (void *arg);
static void to_string_plateau(const int *plateau, char *string);
static void end_game(void *arg);


#endif /* guard */
