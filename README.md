# COMMUNICAT'IF
## Noms des membres du groupe (4IF4) : LARIBI Fatma, AZAR Gloria et FAUCHON Marilou

### Fichiers sources contenus dans l'archive 
Côté Serveur : 
    1. server2.c
    2. server2.h
    3. client2.h
    4. Makefile
Côté Client : 
    1. client2.c
    2. client2.h
    3. Makefile
    4. test.txt (fichier pour la fonctionnalité "Envoi de fichier")
    5. Dossier "received" pour les fichiers reçus

### Compilation et exécution des programmes
__Côté Serveur__ : 
    Se placer dans le répertoire "TP_Reseau_LARIBI_AZAR_FAUCHON/Serveur".
    Taper : "make server".
    Puis : "./server".
__Côté Client__ : 
    Se placer dans le répertoire "TP_Reseau_LARIBI_AZAR_FAUCHON/Client"
    Taper : "make client".
    Puis : "./client <ip machine serveur> login:mdp".

### Fonctionnalités implémentées

#### Authentification des clients avec cryptage des mots de passe
Pour qu'un client se connecte il faut qu'il respecte la partie "Compilation et exécution des programmes" du "Côté Client".
Le client doit taper la ligne de commande "./client <ip machine serveur> login:mdp" après avoir compiler les programmes côté client pour pouvoir se connecter au serveur à l'adresse <ip machine serveur>. Ex : ./client 127.0.0.1 bob:leponge.
__Il est conseillé de n'utiliser que des lettres (minuscules ou majuscules) pour le mot de passe, car seules les lettres sont cryptées.__
Le cryptage de César est utilisé pour le cryptage des mots de passe.
Dans le cas où il s'agit de la première connexion du client sur ce serveur de chat, le mot de passe sera enregistré et associé à ce client (mot de passe associé au login et enregistré dans un fichier). Sinon, une vérification du mot de passe est réalisée en le comparant au mot de passe enregistré dans un fichier lors de la première connexion du client. Dans ce cas-là, si jamais le mot de passe entré par le client est erroné, un message indiquant l'erreur est affiché et le client est automatique déconnecté du serveur. Si le mot de passe entré est correct, le client est effectivement connecté et le menu du serveur est affiché sur son terminal. 
_Méthodes associées_ : 
    - static int authentification(char * name, char * password);
    - static void cryptage(char *p);

#### Création de différents types d'historiques
Lorsqu'un client se connecte au serveur (si l'authentification réussie), en plus de l'affichage d'un menu qui présente toutes les fonctionnalités auxquelles il a accès, il y a aussi l'affichage de deux types d'historiques : 
1. Un historique "HISTORIQUE DE TOUS LES MESSAGES PUBLIQUES" qui regroupe tous les messages publics (connections, déconnections, messages publics (all), création d'un groupe, intégration d'un groupe) envoyés sur le serveur depuis son démarrage. Cet historique est "stocké" dans un fichier "logs.txt" ouvert au démarrage du serveur et complété tant que le serveur n'est pas déconnecté.
2. Un historique "HISTORIQUE DES MESSAGES PERSONNELS NON LUS" qui regroupe tous les messages personnels (messages groupés, messages d'un de ses groupes publics, messages personnels) non lus par le client qui vient de se connecter. Cet historique est "stocké" dans un fichier personnel du type "login.txt" et complété lorsque le client est déconnecté et que d'autres clients lui envoie des messages personnels. Le contenu du fichier est écrasé à chaque fois que le client se déconnecte afin qu'il ne contienne que les messages __non lus__.
Il existe aussi un historique pour les clients qui rejoignent un groupe public et pour qui s'affichent tous les messages envoyés sur ce groupe avant qu'il n'intègre ce dernier (plus de précision dans la partie **Rejoindre un groupe public de discussion**).

#### Envoi de messages entre deux clients ou entre un client et l'ensemble des clients connectés au serveur
Cette fonctionnalité correspond à l'option 1 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
1. Il faut d'abord taper 1 puis ENTRER dans le terminal.
2. Pour envoyer un message à l'ensemble des utilisateurs connectés, taper all:message puis ENTRER.
   Les utilisateurs connectés recevront ce message sous forme d'un message public.
   Par exemple, pour envoyer "hello" à tout le monde, il faudra écrire "all:hello".
   Attention, si vous souhaitez re-envoyer un message à tous les usagers connecté, il faut d'abord taper 1 puis ENTRER avant de re-taper all:message puis ENTRER.
3. Pour envoyer un message à un utilisateur en particulier, après avoir taper 1 puis ENTRER, il faut enter le login du destinataire suivi de ":" et du message, sous cette forme: "destinataire:message". 
   Par exemple, pour envoyer "hello" à bob, il faudra écrire "bob:hello".
_Méthode associée_ : 
    - static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server, char * fileName);
    - static void send_message_to_client(Client * clients,Client sender,int actual,char * destinataire, char *message_a_envoyer, char from_server);

#### Envoi d'un message à un groupe de clients 
Cette fonctionnalité correspond à l'option 2 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
L'utilisateur commence par taper 2 puis ENTRER.
L'utilisateur tape ensuite les noms des destinataires séparés d'un espace entre eux, puis ":" et enfin le message.
Par exemple, pour envoyer à bob et à alice le message "hello" il faudra taper la commande suivante "bob alice:hello", dans ce cas bob et alice recevront un PRIVATE GROUP MESSAGE.
_Méthode associée_ :
    - static void send_message_to_clients(Client * clientsConnectes, int actual, char ** nameClients, Client sender, int nbMembers, const char *buffer, char from_server);

#### Envoi de fichier 
Cette fonctionnalité correspond à l'option 3 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client. Le fichier sera envoyé à tous les utilisateurs connectés.
L'utilisateur commence par taper 3 puis ENTRER.
Un message lui demandant de taper le nom d'un fichier existant s'affiche à l'écran.
Il faut taper le nom d'un fichier existant puis cliquer sur ENTRER. Un message témoignant de la bonne réception du fichier s'affiche sur l'écran.
_Méthodes associées_ :
    - static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server,char * fileName); (server2.c)
    - void send_file(FILE *fp, int sockfd); (client2.c)
    - void write_file(char *fileName, char * content); (client2.c)
    - void send_file(FILE *fp, int sockfd); (server2.c)

#### Création d'un groupe public de discussion 
Cette fonctionnalité correspond à l'option 4 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
L'utilisateur commence par taper 4 puis ENTRER.
L'utilisateur tape ensuite le nom du groupe puis sur ENTRER. Un message témoignant de la création du groupe s'affiche à l'écran.
L'utilisateur qui a créé le groupe fait alors partie de ce groupe.
__Attention : un client ne peut pas créer un groupe avec un nom de groupe qui existe déjà, s'il essaie il verra s'afficher un message d'erreur__


#### Rejoindre un groupe public de discussion 
Cette fonctionnalité correspond à l'option 5 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
L'utilisateur commence par taper 5 puis ENTRER.
La liste des groupes existant ainsi que leurs membres sont affichés, l'utilisateur entre le nom du groupe qu'il souhaite rejoindre. Si le nom du groupe entré n'existe pas, un message disant que le groupe demandé n'existe pas apparaît, sinon, un message témoignant du succès de l'opération s'affiche.
__Attention : un client ne peut pas rejoindre un groupe auquel il appartient déjà, s'il essaie il verra s'afficher un message d'erreur__

#### Envoyer un message à un groupe public de discussion 
Cette fonctionnalité correspond à l'option 6 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
L'utilisateur commence par taper 6 puis ENTRER.
L'utilisateur envoie son message sous la forme suivante "nomDuGroupe:message".
Par exemple, pour envoyer "hello" au groupe communicatIF, il faut taper la commande suivante: "communicatIF:hello".
Les membres du groupes recevront donc un PUBLIC GROUP MESSAGE.
_Méthode associée_ :
    - static void send_message_to_group(Groupe *groupes, int actualGroupe, Client * clientsConnectes, int actual, char * nameGroup, Client sender, const char *buffer, char from_server);

#### Déconnection "propre" d'un client 
Cette fonctionnalité correspond à l'option 7 parmi les fonctionnalités présentées dans le menu lors de la connexion d'un client.
Lorsque que le client tape le chiffre '7' dans sa console et appuie sur ENTRER, il est déconnecté du serveur.
_Méthode associée_ :
    - static void remove_client(Client *clients, int to_remove, int *actual, int suppressionFichier);

#### Déconnection "propre" du serveur
Pour déconnecter proprement le serveur, il suffit de taper la lettre 'q' dans la console du serveur. 
Cela déconnecte ensuite tous les clients connectés au serveur.