#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;

   /* send our name */
   write_server(sock, name);

   while(1)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         fgets(buffer, BUF_SIZE - 1, stdin);
         {
            char *p = NULL;
            p = strstr(buffer, "\n");
            if(p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }
         write_server(sock, buffer);
         /*OPTION 3 CHOISIE*/
         if(strcmp(buffer, "3") == 0)/*dans ce cas-là on envoie un fichier*/
         {
            printf("Write filename\n");
            char fileNameToSend[100];
            scanf("%s",&fileNameToSend);
            FILE *fileToSend = fopen(fileNameToSend, "r");
            if(fileToSend == NULL)
            {
               perror("Error in reading file ");
            }
            send_file(fileToSend, sock);
            printf("File data sent successfully \n");
         }
         
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if(n == 0)
         {
            printf("Signing out !\n");
            break;
         }
         puts(buffer);
         
         char *fileToken = strtok(buffer, "/");
         
         if(strcmp(fileToken, "FILE")==0)
         {
            char *fileName = strtok(NULL, "/");
            char *content = strtok(NULL, "/");
            write_file(fileName, content);
         }
     	   fflush(stdin);
     

      }
   }

   end_connection(sock);
}

static void write_file(char *fileName, char * content){
   FILE *fp;
   char fullName[100];
   strcpy(fullName, "");
   strcat(fullName, "./received/");/*Le fichier sera placé dans le répertoire received*/
   strcat(fullName, fileName);
   fp = fopen(fullName, "w");
   fputs(content, fp);
   fclose(fp);

}

static void send_file(FILE *fp, int sockfd){
   int n;
   char data[1024] = {0};
   while(fgets(data, 1024, fp) != NULL)
   {
   	if(send(sockfd, data, sizeof(data),0) == -1)
      {
   		perror("Error in sending file");
   	}
   	bzero(data, 1024);
   }
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
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

static void write_server(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
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
