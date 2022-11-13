#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_CLIENT_GROUPE 30
#define MAX_GROUPE 50

//Pour le cryptage de césar
#define DECALAGE 5

#define BUF_SIZE    1024

//Structure pour les groupes publiques de discussion
typedef struct
{
   char nameGroupe[BUF_SIZE];
   char * membresGroupe[MAX_CLIENT_GROUPE];
   int nombreMembresGroupe;
}Groupe;

#include "client2.h"

static void init(void);
static void end(void);
static void app(char * fileName);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server,char * fileName);
static void send_message_to_client(Client * clients,Client sender,int actual, char * destinataire, char *message_a_envoyer, char from_server);
static void send_message_to_clients(Client * clientsConnectes, int actual, char ** nameClients, Client sender, int nbMembers, const char *buffer, char from_server);
static void send_message_to_group(Groupe *groupes, int actualGroupe, Client * clientsConnectes, int actual, char * nameGroup, Client sender, const char *buffer, char from_server);
static Client * find_recipients(char ** name, int nbMembers, int actual,Client *clients,  Client *destinataires_groupes);
static void remove_client(Client *clients, int to_remove, int *actual, int suppressionFichier);
static void clear_groups(Groupe *groupes,int actualGroupe);
static void clear_clients(Client *clients, int actual);
static void cryptage(char *p);
static int authentification(char * name, char * password);
static void cryptage_mdp(char *p);
static void decryptage_mdp(char *p);


#endif /* guard */
