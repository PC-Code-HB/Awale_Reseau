# Application client/serveur pour le jeu Awale
Ce projet contient le code nécessaire pour avoir une application client/serveur permettant à des clients de jouer une partie de awale contre un autre client connecté au serveur, ou de visualiser une partie en cours entre 2 autres clients.

## Compiler et Executer

1. Utiliser le makefile pour compiler le projet
   - ```make``` pour compiler en _mode final_
   - ```make debug``` pour compiler en _mode debug_
   - ```make clean``` pour supprimer tous les fichiers objets et les executables créés
2. Sur la machine serveur lancer le programme **_serveur_**
3. Une fois le serveur lancé vous pouvez créer autant de clients que vous le souhaitez (limite de 100) et utiliser toutes les fonctionnalités de l'application. Pour cela, il vous faut lancer le programme **_client_** avec comme premier paramètre **l'adresse ip de la machine serveur** et en second, **votre pseudo**. Exemple : ```./client 127.0.0.1 toto``` si mon serveur est sur la même machine et que je veux m'appeler "toto".

## Utiliser l'application

L'application est intéractif et est guidé par un menu. A chaque étape, on vous demande d'entrer le numéro de l'option que vous voulez.

### Subtilités
- Il est possible qu'à certains moments le traitement et l'application de messages reçus par le client du serveur soient bloqués tant que vous n'avez pas répondu ou fait un choix à la question posée. C'est une pourquoi une fois répondu plusieurs actions se produisent à la suite. Cela ne gêne en rien le bon fonctionnement de l'application mais peut être un point négatif pour l'expérience utilisateur.
- Lorsque l'on observe une partie, on peut quitter avant la fin en saisissant la commande "/quit". Un fois entré, l'effet n'est pas immédiat, il faut d'abord que la partie est évolué encore une fois (coup joué, fin, etc), pour être pris en compte.
- Lorsque vous jouez une partie, votre camp est la ligne du bas du plateau, le sens de rotation est celui anti-horaire et les coups possibles vont de 1 à 6 (1 à gauche, 6 à droite).

## Fonctionnalités implémantés

- Voir les joueurs en ligne c'est-à-dire connecté sur le même serveur que vous.
- Défier un joueur disponible (= qui n'est pas en train de faire une partie ou d'en regarder une). Vous pouvez défier qu'un joueur à la fois et attendre que celui-ci vous réponde.
- En cas d'acceptation d'un défi, vous pouvez jouer une partie de awale en effectuant à tour de rôles des coups. Vous pouvez à tout moment décider d'abandonner et votre adversaire sera prévenu. En cas de deconnection brutale d'un des 2 joueurs, l'autre sera averti comme si vous veniez d'abandonner.
- Le serveur peut gérer plusieurs parties en parallèles (limite hardcodé à 10 mais pourrait être augmenté).
- Voir la liste des parties en cours avec les joueurs qui s'affrontent.
- Observer une partie en cours jusqu'à la fin ou retourner au menu principal avant.


## Bugs possibles et/ou constatés

- La fonction pour vider le buffer avant appel des scanf des menus côté client n'est pas totalement au point.
- En cas de limite atteinte du nombre de parties en parallèle, le serveur n'envoie pas de message au client qui sont alors "perdus".
- En cas de déconnection brutale d'un client dans la phase d'attente de réponse de défi ou en train de répondre à un défi, l'autre client est bloqué.




