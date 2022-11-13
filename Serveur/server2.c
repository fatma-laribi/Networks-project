#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "server2.h"
#include "client2.h"

static void init(void)
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

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(char * fileName)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   //NEW : Variables pour lire le fichier de logs à chaque connection 
   FILE * logFile; 
   char c; 
   char line[34]; 
   //NEW : Variable pour la lecture des fichiers personnels 
   char nomFichier[BUF_SIZE];
   //NEW : Variable pour l'ajout de membres à un groupe 
   char nomMembre[BUF_SIZE];
   /* the index for the array (groupes and clients) */
   int actual = 0;
   int actualGroupe = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   Groupe groupes[MAX_GROUPE];

   fd_set rdfs;

   printf("To disconnect, write q\n");

   while(1)
   {
      int i = 0;
      
      // NEW : Disconnect server 
      char c;
      scanf("%c",c);
      if(tolower(&c) == 'q')
      {
        clear_clients(clients,actual);
        end_connection(sock);
      }
      // END : Disconnect server
      
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         //NEW:authentification
         char * login = strtok(buffer, ":" );
         char * password = strtok ( NULL,":");

         Client c = { csock };
         strncpy(c.name, login, BUF_SIZE - 1);
         int authent=authentification(login,password);

         if(authent==0) //l'authentification a échouée (mot de passe incorrect)
         {
            write_client(c.sock,"Mot de passe incorrecte, vous allez être déconnecté."); //notification envoyée au client 
            /*On ajoute le client à la liste de clients connectés*/
            clients[actual]=c;
            actual++;
            /*On supprime le client et on ferme la socket associée*/
            closesocket(clients[i].sock);
            remove_client(clients, i, &actual,0);
         }
         else //Authentification réussie
         {
            /*On ajoute le client à la liste des clients connectés*/
            clients[actual] = c;
            actual++;
            //NEW : Informations sur les options de communication/commandes données au client qui vient de se connecter
            write_client(clients[i].sock,"\n\n\t\tBienvenue dans le Communicat'IF\n\n");
            write_client(clients[i].sock,"\n\t1:Envoyer un message à un destinataire unique\n\t\t- Envoyer un message à tous les clients -> 'all://message//'\n\t\t- Envoyer un message à quelqu'un nommé 'name' -> 'name://message//'\n\t2:Envoyer un message à un groupe (privé)\n\t\t-Envoyer un message à name1, name2 & name3 -> 'name1 name2 name3://message//' \n\t3:Envoi de fichier\n\t4.Créer un groupe\n\t5.Rejoindre un groupe\n\t6.Envoyer un message à un groupe (publique)\n\t7.Se déconnecter\n\n");
            //NEW : Historique des messages publiques envoyés (à tout le monde) avant que le client se connecte (fichier : logs.txt)
            logFile = fopen(fileName,"r");
            if(logFile != NULL)
            {
               while (fgets(line, 34, logFile) != NULL) {
                  write_client(clients[i].sock,line);
               }
               fclose(logFile);
            }
            //NEW : Historique des messages personnels non lus (messages de groupes privés ou messages privés) (fichier : clients[i].name.txt)
            write_client(clients[i].sock,"\nHISTORIQUE DES MESSAGES PERSONNELS NON LUS:\n");
            nomFichier[0]=0;
            strncat(nomFichier,clients[i].name,sizeof nomFichier - strlen(nomFichier) - 1);
            strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
            logFile = fopen(nomFichier,"r");
            int historiqueNonVide = 0;
            if(logFile != NULL)
            {
               while (fgets(line, 34, logFile) != NULL)
               {
                  historiqueNonVide = 1;
                  write_client(clients[i].sock,line);
               }
               fclose(logFile);
             }
             if(logFile == NULL || historiqueNonVide == 0)
             {
               write_client(clients[i].sock,"Pas de messages personnels non lus.\n");
             }
            //NEW : Connection log send to all the clients except the sender
            strncpy(buffer, c.name, BUF_SIZE - 1);
            strncat(buffer, " connected !", BUF_SIZE - strlen(buffer) - 1);
            send_message_to_all_clients(clients, c, actual, buffer, 1,fileName);

         }
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
                  /*Récupérer l'option*/
                  int c = read_client(clients[i].sock, buffer);
                  int option= atoi(buffer);
                  Client client = clients[i];
                  /*Le client s'est déconnecté*/
                  if(c == 0)
                  {
                     closesocket(clients[i].sock);
                     remove_client(clients, i, &actual,1);
                     strncpy(buffer, client.name, BUF_SIZE - 1);
                     strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                     send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                  }

                  /*OPTION 1 : ÉCRIRE UN MESSAGE À UN DESTINATAIRE UNIQUE*/
                  if(option==1)
                  {
                     write_client(clients[i].sock,"Pour envoyer un message à une personne -> 'nom:message'.\n");
                     write_client(clients[i].sock,"Pour envoyer un message à tout le monde -> 'all:message'.");
                     int c = read_client(clients[i].sock, buffer); //on récupère ce que le client a écrit dans l'entrée standard
                     /*Décomposition du message en 2 parties : nom du destinataire & message*/
                     char * destinataire = strtok(buffer, ":" );
                     char * message = strtok ( NULL,":");
                     /* client disconnected */
                     if(c == 0)
                     {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual,1);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                     }
                     else
                     {
                        if(message != NULL) //vérification de la validité de la synthaxe du message 
                        {
                           /*Traiter l'envoi du message*/
                           if(strcmp(destinataire,"all")==0) //message destiné à tous le clients 
                           {
                              send_message_to_all_clients(clients, client, actual, message, 0,fileName);
                           }
                           else //message destiné à un unique client
                           {
                              send_message_to_client(clients,clients[i],actual,destinataire,message,0);
                           }
                        }
                        else //la synthaxe du message n'est pas valide -> notification à l'expéditeur
                        {
                           write_client(clients[i].sock,"La synthaxe de votre message n'est pas valide, veuillez écrire un message du type -> 'nomDestinataire:message'");
                        }
                     }
                  }

                  /*OPTION 2 : ÉCRIRE UN MESSAGE À UN GROUPE DE PERSONNE (MESSAGE PRIVÉ)*/
                  else if(option==2)
                  {
                     write_client(clients[i].sock,"Pour envoyer un message à plusieurs personne en privé (ex : 3 personnes) -> 'nom1 nom2 nom3:message'.");
                     int c = read_client(clients[i].sock, buffer); //on récupère ce que le client a écrit dans l'entrée standard
                     
                     /* client disconnected */
                     if(c == 0)
                     {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual,1);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                     }
                     else
                     {
                        int m=0; //nombre de destinataires du message 
                        /*Décomposition du message en 2 parties : nom des destinataires & message*/
                        char * bufferDest = strtok(buffer, ":");
                        char * bufferMsg = strtok(NULL, ":");
                        if(bufferMsg != NULL) //Vérification de la validité de la synthaxe du message 
                        {
                           char ** names=malloc(sizeof(sizeof(char)*BUF_SIZE)*MAX_CLIENT_GROUPE); //tableau pour tous les nom des destinataires
                           /*Allocation de mémoire pour le tableau names*/
                           int k;
                           for(k=0;k<MAX_CLIENT_GROUPE;k++)
                           {
                              names[k]=malloc(sizeof(char)*BUF_SIZE);
                           }
                           /*Décomposition de la première partie du message : on sépare tous les noms des destinataires et on les stocke dans names*/
                           char * nameClient=strtok(bufferDest," ");
                           while( nameClient != NULL ) 
                           {
                              strcpy(names[m],nameClient);
                              m++;
                              nameClient = strtok(NULL, " ");
                           }
                           /*Traitement de l'envoi du message à tous les destinataires*/
                           send_message_to_clients(clients,actual,names, client, m, bufferMsg, 0);
                           /*Libération de la mémoire allouée pour le tableau names*/
                           for(k=0;k<MAX_CLIENT_GROUPE;k++)
                           {
                              free(names[k]);
                           }
                           free(names);
                        }
                        else //la synthaxe du message n'est pas valide -> notification à l'expéditeur
                        {
                           write_client(clients[i].sock,"La synthaxe de votre message n'est pas valide, veuillez écrire un message du type -> 'nom1 nom2 nom3:message'");
                        }
                     }
                  }

                  /*OPTION 3 : ENVOYER UN FICHIER*/
                  else if(option == 3){
                    
                    read_client(clients[i].sock, buffer);
                   
                    /*char toSend[] = "The client ";
                    strcat(toSend, client.name);
                    strcat(toSend, " sent you this file");*/
                    
                    char str1[100] = "";
                    char str2[] = "FILE/";
                    char fileName[] = "test.txt";
                    char slash[] = "/";
                    
                    
                    strcat(str1, str2);
                    strcat(str1, fileName);
                    strcat(str1, slash);
                    strcat(str1, buffer);
                    
                    /*
                    strcat(fileName,"FILE/");
                    strcat(fileName,"/test.txt/");
                    strcat(fileName,  buffer);*/
                   // send_message_to_all_clients(clients, client, actual, toSend, 0,fileName);
                    send_message_to_all_clients(clients, client, actual, str1, 1,fileName);
                 
                  }

                  /* OPTION 4 : CRÉER UN GROUPE */
                  else if(option==4)
                  {
                     /*On récupère le nom du nouveau groupe dans l'entrée standard du client*/
                     write_client(clients[i].sock,"Écrire le nom du groupe que vous voulez créer:");
                     int c = read_client(clients[i].sock, buffer);
                     /* client disconnected */
                     if(c == 0)
                     {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual,1);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                     }
                     else
                     {
                        int k,j;
                        int nouveauGroupeExiste = 0;
                        char nomNouveauGroupe[BUF_SIZE];
                        strcpy(nomNouveauGroupe,buffer);
                        /*Vérification que le nom de groupe n'existe pas déjà*/
                        /*Parcours de tous les groupes qui existent*/
                        for(j=0;j<actualGroupe;j++)
                        {
                           /*comparaison du nom du nouveau groupe avec chaque nom de groupe de la liste des groupes créés*/
                           if(strcmp(nomNouveauGroupe,groupes[j].nameGroupe)==0) 
                           {
                              nouveauGroupeExiste = 1; //le nom de groupe existe déjà
                           }
                        }
                        if(nouveauGroupeExiste==0) //le nom de groupe n'existe pas 
                        {
                           strcpy(groupes[actualGroupe].nameGroupe,nomNouveauGroupe);
                           /*On alloue de la mémoire pour le tableau groupes[actualGroupe].membresGroupe (tableau des noms des membres du groupes)*/
                           for(k=0;k<MAX_CLIENT_GROUPE;k++)
                           {
                              groupes[actualGroupe].membresGroupe[k]=malloc(sizeof(char)*BUF_SIZE);
                           }
                           /*On ajoute le créateur du groupe comme unique membre du groupe au début*/
                           groupes[actualGroupe].nombreMembresGroupe = 0;
                           char * nameClient=malloc(sizeof(char)*BUF_SIZE);
                           strncat(nameClient,clients[i].name,sizeof nameClient - strlen(nameClient) - 1);
                           groupes[actualGroupe].membresGroupe[groupes[actualGroupe].nombreMembresGroupe] = nameClient;
                           /*On créer un nouveau fichier de log pour le nouveau groupe créé*/
                           nomFichier[0]=0;
                           strncat(nomFichier,groupes[actualGroupe].nameGroupe,sizeof nomFichier - strlen(nomFichier) - 1);
                           strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
                           logFile = fopen(nomFichier,"w");
                           fclose(logFile);
                           /*Message de confirmation que le groupe a été créé -> notification créateur du groupe*/
                           write_client(clients[i].sock,"Vous avez créer un groupe !");
                           /*Log ajouté dans le fichier de logs publiques*/
                           nomFichier[0]=0;
                           strncat(nomFichier,"logs.txt",sizeof nomFichier - strlen(nomFichier) - 1);
                           logFile = fopen(nomFichier,"a");
                           fprintf(logFile,"%s has created a group named %s\n",groupes[actualGroupe].membresGroupe[groupes[actualGroupe].nombreMembresGroupe],groupes[actualGroupe].nameGroupe);
                           fclose(logFile);
                           /*Log ajouté dans la sortie standard du serveur*/
                           printf("%s has created a group named %s\n",groupes[actualGroupe].membresGroupe[groupes[actualGroupe].nombreMembresGroupe],groupes[actualGroupe].nameGroupe);
                           /*On incrémente le nombre de groupes de discussion et le nombre de membres du nouveau groupe*/
                           (groupes[actualGroupe].nombreMembresGroupe)++;
                           actualGroupe++;
                        }
                        else //le nom de groupe existe déjà -> notification envoyé à l'expéditeur de la requête
                        {
                           write_client(clients[i].sock,"Ce nom de groupe existe déjà...Votre demande de création de groupe a échouée...");
                        }
                     }
                  }

                  /*OPTION 5 : REJOINDRE UN GROUPE*/
                  else if(option == 5)
                  {
                     int k,j,n;
                     char nomGroupe[BUF_SIZE];
                     /* On liste les groupes (nom & noms des membres) qui existent*/
                     write_client(clients[i].sock,"LISTE DES GROUPES:");
                     for(k=0;k<actualGroupe;k++)
                     {
                        nomGroupe[0]=0;
                        strncat(nomGroupe,groupes[k].nameGroupe,sizeof nomGroupe - strlen(nomGroupe) - 1);
                        /*Nom du groupe*/
                        write_client(clients[i].sock,"\n\tNOM DU GROUPE: ");
                        write_client(clients[i].sock,nomGroupe);
                        /*Nom des membres du groupe*/
                        write_client(clients[i].sock,"\n\tMEMBRES DU GROUPE: ");
                        for(j=0;j<groupes[k].nombreMembresGroupe;j++)
                        {
                           char * nameClient2=malloc(sizeof(char)*BUF_SIZE);
                           strncat(nameClient2,groupes[k].membresGroupe[j],sizeof nameClient2 - strlen(nameClient2) - 1);
                           write_client(clients[i].sock,nameClient2);
                           if(j!=(groupes[k].nombreMembresGroupe-1))
                           {
                              write_client(clients[i].sock,", ");
                           }
                           else
                           {
                              write_client(clients[i].sock,".\n");
                           }
                        } 
                     }
                     /* On demande le nom du groupe que le client veut rejoindre */
                     write_client(clients[i].sock,"Écrire le nom du groupe (parmi la liste des groupes précédente) que vous voulez rejoindre:");
                     int c = read_client(clients[i].sock, buffer);
                     /* client disconnected */
                     if(c == 0)
                     {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual,1);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                     }
                     else
                     {
                        int groupeExiste = 0; //entier qui vérifie l'existence du groupe : 0->le groupe n'existe pas, 1->le groupe existe
                        int dejaMembre = 0; //entier qui vérifie que le client ne fait pas déjà partie du groupe
                        /*Parcours de tous les groupes qui existent*/
                        for(k=0;k<actualGroupe;k++)
                        {
                           /*On regarde si le nom de groupe existe*/
                           if(strcmp(groupes[k].nameGroupe,buffer)==0)
                           {
                              groupeExiste = 1; //le groupe existe
                              /*On vérifie que la personne qui veut rejoindre le groupe ne fait pas déjà partie du groupe*/
                              char * nameClient4=malloc(sizeof(char)*BUF_SIZE);
                              strncat(nameClient4,clients[i].name,sizeof nameClient4 - strlen(nameClient4) - 1);
                              /*Parcours de la liste des noms des membres du groupe*/
                              for(n=0;n<groupes[k].nombreMembresGroupe;n++)
                              {
                                 if(strcmp(nameClient4,groupes[k].membresGroupe[n])==0)
                                 {
                                    dejaMembre = 1; //le client est déjà membre du groupe 
                                 }
                              }
                              if(dejaMembre == 0) //Si le client n'est pas déjà membre 
                              {
                                 if(groupes[k].nombreMembresGroupe<MAX_CLIENT_GROUPE) //Si le groupe ne dépasse pas sa capacité maximale de nombre de clients membres
                                 {
                                    char * nameClient3=malloc(sizeof(char)*BUF_SIZE);
                                    strncat(nameClient3,clients[i].name,sizeof nameClient3 - strlen(nameClient3) - 1);
                                    /*Ajout du client à la liste des membres du groupe*/
                                    groupes[k].membresGroupe[groupes[k].nombreMembresGroupe] = nameClient3;
                                    /*Confirmation envoyée au client*/
                                    write_client(clients[i].sock,"Vous avez rejoint le groupe!");
                                    /*Log ajouté dans le fichier de logs publiques*/
                                    nomFichier[0]=0;
                                    strncat(nomFichier,"logs.txt",sizeof nomFichier - strlen(nomFichier) - 1);
                                    logFile = fopen(nomFichier,"a");
                                    fprintf(logFile,"%s has joined the group named %s\n",groupes[k].membresGroupe[groupes[k].nombreMembresGroupe],groupes[k].nameGroupe);
                                    fclose(logFile);
                                    /*Log ajouté dans la sortie standard du serveur*/
                                    printf("%s has joined the group named %s\n",groupes[k].membresGroupe[groupes[k].nombreMembresGroupe],groupes[k].nameGroupe);
                                    //NEW : Historique des messages du groupe non lus 
                                    write_client(clients[i].sock,"\nHISTORIQUE DES MESSAGES PRÉCÉDENTS DU GROUPE:\n");
                                    nomFichier[0]=0;
                                    strncat(nomFichier,groupes[k].nameGroupe,sizeof nomFichier - strlen(nomFichier) - 1);
                                    strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
                                    logFile = fopen(nomFichier,"r");
                                    if(logFile != NULL)
                                    {
                                       while (fgets(line, 34, logFile) != NULL) 
                                       {
                                          write_client(clients[i].sock,line);
                                       }
                                       fclose(logFile);
                                    }
                                    /*On incrémente le nombre de membres du groupe*/
                                    (groupes[k].nombreMembresGroupe)++;
                                 }
                                 else //Si le groupe dépasse sa capacité maximale de nombre de clients membres
                                 {
                                    write_client(clients[i].sock,"Le groupe ne peut plus accueillir un nouveau membre, sa capacité maximale est déjà atteinte...");
                                 }
                              }
                              else //le client est déjà membre du groupe 
                              {
                                 write_client(clients[i].sock,"Vous êtes déjà membre du groupe, vous ne pouvez pas rejoindre le groupe plusieurs fois");
                              }
                           }
                        }
                        /*Si le groupe que le client souhaite rejoindre n'existe pas on lui envoie un message*/
                        if(groupeExiste == 0)
                        {
                           write_client(clients[i].sock,"Le groupe que vous voulez rejoindre n'existe pas!");
                        }
                     }
                  }

                  /*OPTION 6: ENVOYER UN MESSAGE À UN GROUPE*/
                  else if(option==6)
                  {
                     write_client(clients[i].sock,"Pour envoyer un message à un groupe -> 'nomDuGroupe:message'.");
                     /*Récupérer le message de type nomGroupe:message dans un buffer*/
                     int c = read_client(clients[i].sock, buffer);
                     /* client disconnected */
                     if(c == 0)
                     {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual,1);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                     }
                     else
                     {
                        /* Séparation du message & du nom du groupe destinataire*/
                        char * groupeDestinataire = strtok(buffer, ":" );
                        char * message = strtok ( NULL,":");
                        if(message != NULL) //Vérification de la validité de la synthaxe du message
                        {
                           /*Traiter l'envoi du message aux membres du groupe*/
                           send_message_to_group(groupes,actualGroupe,clients,actual,groupeDestinataire, client, message, 0);
                        }
                        else //La synthaxe du message n'est pas valide -> notification envoyé à l'expéditeur
                        {
                           write_client(clients[i].sock,"La synthaxe de votre message n'est pas valide, veuillez écrire un message du type -> 'nomGroupe:message'");
                        }
                     }
                  }

                  /*OPTION 7: DÉCONNECTION DU CLIENT*/
                  else if(option == 7)
                  {
                     closesocket(clients[i].sock);
                     remove_client(clients, i, &actual,1);
                     /*Message de déconnection du client envoyé à tout le monde*/
                     strncpy(buffer, client.name, BUF_SIZE - 1);
                     strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                     send_message_to_all_clients(clients, client, actual, buffer, 1,fileName);
                  }
                  break;
            }
         }
      }
   }
   clear_clients(clients, actual);
   end_connection(sock); 
}

void send_file(FILE *fp, int sockfd){
   int n;
   char data[1024] = {0};
   
   while(fgets(data, 1024, fp)!= NULL){
        if(send(sockfd, data, sizeof(data), 0) == -1){
           perror("error in sending file");
        }
        bzero(data, 1024);
   }
}

/*Méthode qui permet l'authentification du client*/
/*Paramètres : name = nom du client (login) - password = mot de passe donné par le client*/
/*Renvoie 0 si l'authentification du client a échouée (mot de passe incorrect ou échec de l'écriture du mot de passe). 
/*Renvoie 1 si l'authentification a réussie (mot de passe correct ou réussite de l'écriture du mot de passe).*/
static int authentification(char * name, char * password)
{
   char nom_du_fichier[BUF_SIZE];
   char password_saved[BUF_SIZE];
   /*On récupère le fichier du type passwordname.txt dans lequel est stocké le mot de passe associé au client (à la première connection)*/
   nom_du_fichier[0]=0;
   strncat(nom_du_fichier,"password",BUF_SIZE - strlen(nom_du_fichier) - 1);
   strncat(nom_du_fichier,name,BUF_SIZE - strlen(nom_du_fichier) - 1);
   strncat(nom_du_fichier,".txt",BUF_SIZE - strlen(nom_du_fichier) - 1);
   FILE * is=fopen(nom_du_fichier,"r");
   if(is!=NULL) //Le fichier existe, le client s'est déjà connecté 
   {
      /*Lecture du mot de passe crypté & stocké dans le fichier*/
      fgets(password_saved,BUF_SIZE ,is);
      /*Cryptage du mot de passe entré par le client lors de la connection*/
      char * p=&password[0];
      while((*p)!='\0')
      {
         cryptage(p);
         ++p;
      }
      /*Comparaison des deux messages cryptés*/
      if(strcmp(password,password_saved)==0) //les 2 mots de passes sont identiques -> mot de passe correct
      {
         fclose(is);
         return 1;
      }
      else //les 2 mots de passes sont différents -> mot de passe incorrect 
      {
         fclose(is);
         return 0;
      }
   }
   else //le fichier n'existe pas, le client ne s'est encore jamais connecté, on crée un nouveau fichier dans lequel on stocke le mot de passe du client
   {
      /*Création d'un nouveau fichier passwordname.txt pour stocker le mot de passe du client*/
      FILE * os =fopen(nom_du_fichier,"w");
      /*Cryptage du mot de passe*/
      char * p=&password[0];
      while((*p)!='\0')
      {
         cryptage(p);
         ++p;
      }
      if ( 1 != fwrite(password,strlen(password), 1,os) ) //l'écriture du mot de passe a échouée
      {
         fclose(os);
         return 0;
      }
      else //l'écriture du mot de passe a réussie
      {
         fclose(os);
         return 1;
      }
   }
}

/*Méthode qui permet d'envoyer un message à un groupe publique*/
/*Paramètres : 
   - groupes : tableau de tous les Groupe créés depuis que le serveur est connecté
   - actualGroupe : entier qui donne le nombre de Groupe créés depuis que le serveur est connecté (le nombre de Groupe dans groupes)
   - clientsConnectes : tableau qui contient tous les Client actuellement connectés sur le serveur 
   - actual : entier qui donne le nombre de Client actuellement connectés au serveur 
   - nameGroup : chaîne de caractères qui contient le nom du groupe auquel le client veut envoyer un message
   - sender : Client qui souhaite envoyer un message au Groupe nommé nameGroup
   - buffer : contient le message que sender souhaite envoyer au Groupe nameGroup
   - from_server : donne le type de message qu'il faut envoyer (Si 0 -> on note le type de message et l'expéditeur, si 1 -> on note juste le contenu et le(s) destinataire(s))*/
static void send_message_to_group(Groupe *groupes, int actualGroupe, Client * clientsConnectes, int actual, char * nameGroup, Client sender, const char *buffer, char from_server)
{
   int i,j,k;
   int destinataireConnecte; //booléen : si 0-> le destinataire du message n'est pas connecté, si 1 -> le destinataire du message est connecté 
   int groupeExiste = 0; //booléen : si 0-> le nom du groupe n'existe pas donc le Groupe n'existe pas, si 1 -> le nom du groupe existe donc le Groupe existe
   int membreGroupe = 0; //booléen : si 0-> l'expéditeur n'est pas membre du groupe, si 1 -> l'expéditeur est membre du groupe
   FILE * logGroupe;
   char nomFichier[BUF_SIZE];
   /*Chaîne de caractères pour le message crypté*/
   char message_crypte[BUF_SIZE];
   message_crypte[0] = 0;
   /* Mise en forme du message (non crypté)*/
   char message[BUF_SIZE];
   message[0] = 0;
   if(from_server == 0) //ni connection, ni déconnection d'un client
   {
      strncat(message,"(PUBLIC GROUPE : ",sizeof message - strlen(message) - 1);
      strncat(message,nameGroup,sizeof message - strlen(message) - 1); //nom du groupe (nameGroup)
      strncat(message, ") From ",sizeof message - strlen(message) - 1);
      strncat(message,sender.name,sizeof message - strlen(message) - 1); //nom de l'expéditeur
      strncat(message, " : ", sizeof message - strlen(message) - 1);
   }
   strncat(message, buffer, sizeof message - strlen(message) - 1); //message
   strncat(message,". To : ",sizeof message - strlen(message) - 1);
   /*Parcours de la liste des groupes existants*/
   for(k=0;k<actualGroupe;k++)
   {
      if(strcmp(nameGroup,groupes[k].nameGroupe)==0)
      {
         groupeExiste = 1; // le groupe existe
         /*Vérification que l'expéditeur est bien membre du groupe : parcours de tous les membres du groupe*/
         for(i = 0; i < groupes[k].nombreMembresGroupe; i++)
         {
            if(strcmp(groupes[k].membresGroupe[i],sender.name)==0)
            {
               membreGroupe = 1; //l'expéditeur est membre du groupe 
            }
         }
         if(membreGroupe == 1) //si l'expéditeur est membre du groupe il peut envoyer un message à ce groupe
         {
            /*On écrit le nom de tous les destinataires du message (tous les membres du groupe) dans le message final*/
            for(i = 0; i < groupes[k].nombreMembresGroupe; i++)
            {
               strncat(message,groupes[k].membresGroupe[i],sizeof message - strlen(message) - 1);
               if(i != (groupes[k].nombreMembresGroupe-1))
               {
                  strncat(message,", ",sizeof message - strlen(message) - 1);
               }
               else
               {
                  strncat(message,".",sizeof message - strlen(message) - 1);
               }
            }
            /*On envoie le message à tous les membres du groupe (connectés ou déconnectés)*/
            for(i = 0; i < groupes[k].nombreMembresGroupe; i++)
            {
               destinataireConnecte = 0;
               /*Parcours de la liste des clients connectés*/
               for(j = 0; j < actual; j++)
               {
                  /* On vérifie si le destinataire est connecté, s'il est connecté on lui envoie le message directement*/
                  if(strcmp(clientsConnectes[j].name,groupes[k].membresGroupe[i])==0)
                  {
                     destinataireConnecte = 1; //le destinataire est connecté
                     /* we don't send message to the sender */
                     if(sender.sock != clientsConnectes[j].sock)
                     {
                        write_client(clientsConnectes[j].sock, message);
                     }
                  }
               }
               /* NEW : Écriture du message personnel (groupe) dans un fichier personnel pour chaque destinataire (membre du groupe privé)*/
               /* Si le destinataire n'est pas connecté, on écrit le message dans son fichier de messages personnels (fichier du type : name.txt)*/
               if(destinataireConnecte==0)
               {
                  nomFichier[0]=0;
                  strncat(nomFichier,groupes[k].membresGroupe[i],sizeof nomFichier - strlen(nomFichier) - 1);
                  strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
                  logGroupe = fopen(nomFichier,"a");
                  fprintf(logGroupe,"%s\n",message);
                  fclose(logGroupe);
               }
            }
         }
      }
   }
   /*Si le groupe existe et l'expéditeur est membre du groupe -> on affiche des logs qui confirment l'envoi du message*/
   if(groupeExiste == 1 && membreGroupe == 1)
   {
      /* Affichage du message dans la sortie standard du serveur*/
      printf("%s\n",message);
      /*Écriture du message dans un fichier de logs propre au groupe publique */
      nomFichier[0]=0;
      strncat(nomFichier,nameGroup,sizeof nomFichier - strlen(nomFichier) - 1);
      strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
      logGroupe = fopen(nomFichier,"a");
      fprintf(logGroupe,"%s\n",message);
      fclose(logGroupe);
      /*Pour le cryptage de césar : on crypte le message envoyé*/
      strcpy(message_crypte,message);
      char * p=&message_crypte[0];
      while((*p)!='\0')
      {
         cryptage(p);
         ++p;
      }
      /* Affichage du message crypté dans la sortie standard du serveur*/
      printf("%s\n",message_crypte);
   }
   /*Si le groupe existe et l'expéditeur n'est pas membre du groupe -> notification envoyée à l'expéditeur*/
   else if(groupeExiste == 1 && membreGroupe == 0)
   {
      write_client(sender.sock,"Vous ne pouvez pas envoyer de message à ce groupe car vous n'êtes pas membre du groupe...");
   }
   /*Si le groupe n'existe pas -> notification envoyée à l'expéditeur*/
   else
   {
      write_client(sender.sock,"Le groupe auquel vous voulez envoyer un message n'existe pas...");
   }
}

/*Méthode qui permet de supprimer toutes les sockets de tous les clients connectés et d'écraser le contenu des fichiers de logs personnels pour tous ces clients*/
static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   char nomFichier[BUF_SIZE];
   FILE * newLog;
   /*On écrase le contenu des fichiers de logs des messages personnels de tous les clients connectés*/
   for(i = 0; i < actual; i++)
   {
      nomFichier[0]=0;
      strncat(nomFichier,clients[i].name,sizeof nomFichier - strlen(nomFichier) - 1);
      strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
      newLog = fopen(nomFichier,"w");
      fclose(newLog);
      closesocket(clients[i].sock);
   }
}

/*Méthode qui permet d'enlever un client du tableau des clients connectés lorsque celui-ci se déconnecte*/
/*La méthode écrase le contenu du fichier de logs de messages personnels pour ce client*/
static void remove_client(Client *clients, int to_remove, int *actual, int suppressionFichier)
{
   if (suppressionFichier == 1)
   {
      /* NEW : 'Suppression' du fichier des logs des messages personnels du client qui se déconnecte*/
      char nomFichier[BUF_SIZE];
      nomFichier[0]=0;
      strncat(nomFichier,clients[to_remove].name,sizeof nomFichier - strlen(nomFichier) - 1);
      strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
      FILE * newLog = fopen(nomFichier,"w");
      fclose(newLog);
   }
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
}

/*Méthode qui permet d'envoyer un message à tous les clients connectés*/
/*Paramètres : 
   - clients : tableau qui contient tous les Client actuellement connectés sur le serveur 
   - sender : Client qui souhaite envoyer un message à tous les Client connectés
   - actual : entier qui donne le nombre de Client actuellement connectés au serveur 
   - buffer : contient le message que sender souhaite envoyer au Groupe nameGroup
   - from_server : donne le type de message qu'il faut envoyer (Si 0 -> on note le type de message et l'expéditeur, si 1 -> on note juste le contenu et le(s) destinataire(s))
   - fileName : chaîne de caractères qui représente le nom du fichier qui contient tous les messages publiques envoyés depuis la connection du serveur*/
static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server, char * fileName)
{
   int i = 0;
   /*Chaîne de caractères pour le message crypté*/
   char message_crypte[BUF_SIZE];
   message_crypte[0] = 0;
   /* Mise en forme du message (non crypté) */
   char message[BUF_SIZE];
   message[0] = 0;
   if(from_server == 0) // ni connection, ni déconnection d'un client
   {
      strncat(message,"(PUBLIC)From ",sizeof message - strlen(message) - 1);
      strncat(message,sender.name,sizeof message - strlen(message) - 1); //nom de l'expéditeur
      strncat(message, " : ", sizeof message - strlen(message) - 1);
   }
   strncat(message, buffer, sizeof message - strlen(message) - 1); //message
   /*Envoi du message à tous les clients connectés*/
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         write_client(clients[i].sock, message);
      }
   }
   //NEW : Write the message in a log file and in the server console
   //Les clients connectés pourront voir le message dans l'historique des messages publiques lors de leur connection
	printf("%s\n",message);
   FILE * log = fopen(fileName,"a");
	fprintf(log,"%s\n",message);
   fclose(log);
   /*Pour le cryptage de césar : cryptage du message envoyé*/
   strcpy(message_crypte,message);
   char * p=&message_crypte[0];
   while((*p)!='\0')
   {
      cryptage(p);
      ++p;
   }
   /*Affichage du message dans la sortie standard du serveur*/
   printf("%s\n",message_crypte);
}

/*Méthode qui permet d'envoyer un message à un client unique*/
/*Paramètres : 
   - clients : tableau qui contient tous les Client actuellement connectés sur le serveur 
   - sender : Client qui souhaite envoyer un message à tous les Client connectés
   - actual : entier qui donne le nombre de Client actuellement connectés au serveur 
   - destinataire : chaîne de caractères qui représente le nom du Client auquel l'expéditeur veut envoyer un message
   - message_a_envoyer : contient le message que sender souhaite envoyer au destinataire
   - from_server : donne le type de message qu'il faut envoyer (Si 0 -> on note le type de message et l'expéditeur, si 1 -> on note juste le contenu et le(s) destinataire(s))*/
static void send_message_to_client(Client * clients,Client sender,int actual,char * destinataire, char *message_a_envoyer, char from_server)
{
   int i = 0;
   FILE * log;
   int destinataireExiste = 0;
   /*Chaîne de caractères pour le message crypté*/
   char message_crypte[BUF_SIZE];
   message_crypte[0] = 0;
   /*Chaîne de caractères pour le nom du fichier qui stocke les logs de messages personnels du destinataire du message*/
   char nomFichier[BUF_SIZE];
   nomFichier[0]=0;
   /*Mise en forme du message (non crypté)*/
   char message[BUF_SIZE];
   message[0] = 0;
   if(from_server == 0) //ni connection, ni déconnection d'un client
   {
      strncat(message,"(PRIVATE)From ",sizeof message - strlen(message) - 1);
      strncat(message,sender.name,sizeof message - strlen(message) - 1);//nom de l'expéditeur
      strncat(message, " : ", sizeof message - strlen(message) - 1);
   }
   strncat(message, message_a_envoyer, sizeof message - strlen(message) - 1);//message
   /*Parcours de tous les clients connectés : si le destinataire en fait partie on lui envoie directement le message*/
   for(i = 0; i < actual; i++)
   {
      if(strcmp(clients[i].name,destinataire)==0)
      {
         destinataireExiste = 1; //le destinataire est connecté 
         write_client(clients[i].sock, message);
      }
   }
   /*Le destinataire ne fait pas partie des clients connectés*/
   /*Envoi d'une notification à l'expéditeur du message*/
   char notification[BUF_SIZE];
   notification[0]=0;
   if(destinataireExiste==0)
   {
      /*Notification : le destinataire du message n'est pas connecté*/
      strncat(notification,"Le destinataire ",sizeof notification - strlen(notification) - 1);
      strncat(notification,destinataire,sizeof notification - strlen(notification) - 1);
      strncat(notification, " n'est pas connecté. Le destinataire pourra lire le message une fois connecté.", sizeof notification - strlen(notification) - 1);
      write_client(sender.sock, notification); 
   }
   /*Stockage du message dans un fichier de messages personnels pour le destinataire, si le destinataire n'est pas connecté il pourra le lire une fois connecté*/ 
   strncat(nomFichier,destinataire,sizeof nomFichier - strlen(nomFichier) - 1);
   strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
   log = fopen(nomFichier,"a");
   fprintf(log,"%s\n",message);
   fclose(log);
   /*Écriture du message dans la sortie standard du serveur*/
   strncat(message,". To : ",sizeof message - strlen(message) - 1);
   strncat(message, destinataire, sizeof message - strlen(message) - 1);
   printf("%s\n",message);
   /*Pour le cryptage de césar : cryptage du message envoyé*/
   strcpy(message_crypte,message);
   char * p=&message_crypte[0];
   while((*p)!='\0')
   {
      cryptage(p);
      ++p;
   }
   /*Écriture du message crypté dans la sortie standard du serveur*/
   printf("%s\n",message_crypte);
}

/*Méthode qui permet d'envoyer le même message à un groupe de personnes*/
/*Paramètres : 
   - clientsConnectes : tableau qui contient tous les Client actuellement connectés sur le serveur
   - actual : entier qui donne le nombre de Client actuellement connectés au serveur 
   - nameClients : tableau de chaînes de caractères qui représentent les noms des Client auxquels l'expéditeur veut envoyer un message
   - sender : Client qui souhaite envoyer un message à tous les Client connectés 
   - nbMembers : entier qui représente le nombre de destinataires auxquels l'expéditeur veut envoyer un message
   - buffer : contient le message que sender souhaite envoyer aux destinataires
   - from_server : donne le type de message qu'il faut envoyer (Si 0 -> on note le type de message et l'expéditeur, si 1 -> on note juste le contenu et le(s) destinataire(s))*/
static void send_message_to_clients(Client * clientsConnectes, int actual, char ** nameClients, Client sender, int nbMembers, const char *buffer, char from_server)
{
   int i,j;
   int destinataireConnecte;
   FILE * logGroupe;
   /*Chaîne de caractères pour les noms des fichiers de logs de messages personnels de chaque destinataire non connecté*/
   char nomFichier[BUF_SIZE];
   /*Chaîne de caractères pour le message crypté*/
   char message_crypte[BUF_SIZE];
   message_crypte[0] = 0;
   /* Mise en forme du message (non crypté)*/
   char message[BUF_SIZE];
   message[0] = 0;
   if(from_server == 0) //ni connection, ni déconnection d'un client
   {
      strncat(message,"(PRIVATE GROUP MESSAGE)From ",sizeof message - strlen(message) - 1);
      strncat(message,sender.name,sizeof message - strlen(message) - 1); //nom de l'expéditeur
      strncat(message, " : ", sizeof message - strlen(message) - 1);
   }
   strncat(message, buffer, sizeof message - strlen(message) - 1); //message
   strncat(message,". To : ",sizeof message - strlen(message) - 1);
   /* Affichage de tous les destinataires : Parcours du tableau namesClients qui contient tous les noms des destinataires*/
   for(i = 0; i < nbMembers; i++)
   {
      strncat(message,nameClients[i],sizeof message - strlen(message) - 1);
      if(i != (nbMembers-1))
      {
         strncat(message,", ",sizeof message - strlen(message) - 1);
      }
      else
      {
         strncat(message,".",sizeof message - strlen(message) - 1);
      }
   }
   /*Affichage du message final dans la sortie standard du serveur*/
   printf("%s\n",message);
   /*Pour le cryptage de césar : cryptage du message final*/
   strcpy(message_crypte,message);
   char * p=&message_crypte[0];
   while((*p)!='\0')
   {
      cryptage(p);
      ++p;
   }
   /*Affichage du message final crypté dans la sortie standard du serveur*/
   printf("%s\n",message_crypte);
   /*Envoi des messages aux destinataires (connectés ou déconnectés)*/
   /*Parcours des noms des destinataires*/
   for(i = 0; i < nbMembers; i++)
   {
      destinataireConnecte = 0;
      /*Parcours des clients connectés*/
      for(j = 0; j < actual; j++)
      {
         /* On vérifie si le destinataire est connecté, s'il est connecté on lui envoie le message directement*/
         if(strcmp(clientsConnectes[j].name,nameClients[i])==0)
         {
            destinataireConnecte = 1; //le destinataire est connecté
            /* we don't send message to the sender */
            if(sender.sock != clientsConnectes[j].sock)
            {
               write_client(clientsConnectes[j].sock, message);
            }
         }
      }
      /* Si le destinataire n'est pas connecté, on écrit le message dans son fichier de messages personnels (fichier du type : name.txt)*/
      if(destinataireConnecte==0)
      {
         nomFichier[0]=0;
         strncat(nomFichier,nameClients[i],sizeof nomFichier - strlen(nomFichier) - 1);
         strncat(nomFichier,".txt",sizeof nomFichier - strlen(nomFichier) - 1);
         logGroupe = fopen(nomFichier,"a");
         fprintf(logGroupe,"%s\n",message);
         fclose(logGroupe);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
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

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

/*Méthode qui permet de crypter un message donné en paramètre (p), en utilisant la méthode de cryptage de César*/
static void cryptage(char *p)
{
   /*****************************************************************************/
   /*                                                                           */
   /*                              crypt                                        */
   /*                                                                           */
   /*   But:                                                                    */
   /*      Crypte le caractère passé en paramètre                               */
   /*                                                                           */
   /*   Interface:                                                              */
   /*      p : pointe le caractère à crypter                                    */
   /*                                                                           */
   /*****************************************************************************/
   enum {BAS, HAUT};
   int casse;

   if (*p >= 'a' && *p <= 'z') casse = BAS;
   else if (*p >= 'A' && *p <= 'Z') casse = HAUT;
   else return;

   *p = *p + DECALAGE;
   //en cas de débrodement (avec le +5 on dépasse le z), dans ce cas c'est z
   if (casse == BAS && *p > 'z' || casse == HAUT && *p > 'Z') *p = *p -26;
}

/*Méthode qui permet de crypter les mots de passe donnés par les clients lors de leur première connection*/
static void cryptage_mdp(char *p)
{
   char * alphabet ="abcdefghijklmnopqrstuvwxyz";
   char *alphabet_crypte ="xstubfjmoaednkvwgihzy";

   int i;
   for(i=0; i<26; ++i)
   {
      if(*p==alphabet[i])
      {
         *p=alphabet_crypte[i];
      }
   }
}

/*Méthode qui permet de décrypter les mots de passe des clients*/
static void decryptage_mdp(char *p)
{
   char alphabet[]="abcdefghijklmnopqrstuvwxyz";
   char alphabet_crypte[]="xstubfjmoaednkvwgihzyclrqp";
   int i;
   for(i=0; i<26; ++i)
   {
      if(*p==alphabet_crypte[i])
      {
         *p=alphabet[i];
      }
   }
}


int main(int argc, char **argv)
{
   char * mainLogFile = "logs.txt"; //Fichier de logs qui contient les messages publiques envoyés depuis le démarrage du serveur
   FILE *logFile = fopen(mainLogFile,"w"); 
   if(logFile == NULL)
   {
      printf("Error in the file!");   
      exit(1);             
   }
   fprintf(logFile,"HISTORIQUE DE TOUS LES MESSAGES PUBLIQUES:\n");
   fclose(logFile);
   
   init();

   app(mainLogFile);

   end();
   
   return EXIT_SUCCESS;
}
