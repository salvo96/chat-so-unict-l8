#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>

#define MAX_CLIENT 128

typedef struct user{
 int sock_id;
 char msg_ricevuto[2048];
}dati_client;

dati_client utente;
int write_th_end=1;

void *leggi(void *arg);
void set_username(char *username, int sock_id);

int main(void){

 int sock_id, check;
 char nome_server[128], username[128], msg_send[1024], trash[1];
 struct sockaddr_in server_ind;
 struct hostent *hp;
 pthread_t thread_leggi; 
 
 printf("\nChat by Salvatore Cavallaro - Applicazione CLIENT\n\nAvvio in corso...\n\n");
 
 sock_id=socket(AF_INET, SOCK_STREAM, 0);
 if(sock_id<0){
  printf("Errore durante la creazione socket - Uscita...\n");
  exit(1);
 } 
 server_ind.sin_family=AF_INET;
 printf("Inserire di seguito i parametri per la connessione con il SERVER\n\n");
 printf("Inserire l'indirizzo IP del SERVER (es. 127.0.0.1): ");
 scanf("%s*c", nome_server);
 fflush(stdin);
 hp=gethostbyname(nome_server);
 bcopy((char*)hp->h_addr, (char*)&server_ind.sin_addr, hp->h_length);
 server_ind.sin_port=htons(2017);
 if(connect(sock_id,(struct sockaddr*)&server_ind, sizeof(server_ind))<0){
  printf("Errore durante la connessione con il SERVER - Uscita...\n\n");
  exit(1);
 }
 /*Il client si è quindi connesso con il server*/
 printf("\nConnessione al SERVER avvenuta correttamente!\n\n");
 utente.sock_id=sock_id;
 set_username(username, sock_id);
 printf("\nUsername %s impostato correttamente!\n\n- Ti trovi in chat pubblica\n\n- Per inviare un messaggio scrivilo e premi INVIO\n\n- Per visualizzare i comandi disponibili digita /help\n\n", username);
 if(pthread_create(&thread_leggi, NULL, leggi, (void*)&utente)<0){
  printf("Errore nella creazione del thread di ricezione dei messaggi - Uscita...\n");
  exit(1);
 }
 fgets(trash, 8, stdin); //inserito per sopperire a problemi di buffer
 while(write_th_end==1){
  memset(msg_send,'\0',sizeof(msg_send));
  fgets(msg_send, 1024, stdin);
  if(strcmp(msg_send, "/quit\n")==0)
   break;
  if(write_th_end==0)
   break;
  check=write(sock_id, msg_send, 1024);
  if(check<0)
   printf("Errore nell'invio del messaggio\n");
 }
 if(write_th_end!=0){
  if(pthread_cancel(thread_leggi)<0)
   printf("Errore durante la cancellazione del thread di lettura messaggi\n");
  else
   printf("Disconnessione dal SERVER avvenuta correttamente!\n\n");
 }
 close(sock_id);
 printf("Chiusura del CLIENT...\n");	
}

void set_username(char *username, int sock_id){
 int ask_user=0;
 char control[5];
 do{
  if(ask_user!=1)
   printf("Inserire un nome utente di seguito [MAX 127 caratteri]: ");
  else
   printf("\nIl nome utente è già stato utilizzato, inseriscine un altro [MAX 127 caratteri]: ");
  memset(username,'\0',sizeof(username));
  scanf("%s*c", username);
  fflush(stdin);
  while(sizeof(username)>128){
   printf("Inserire un nome utente più breve: ");
   memset(username,'\0',sizeof(username));
   scanf("%s*c", username);
   fflush(stdin);
  }  
  if(write(sock_id, username, 128)<0){
   printf("Errore durante la comunicazione dello username al server - Uscita...\n\n");
   exit(1);
  }
  read(sock_id, control, 5);
  if(strcmp(control, "NO")==0)
   ask_user=1;
  else
   ask_user=0;
 }while(ask_user);
}

void *leggi(void *arg){
 int check, continua=1;
 while(continua){
  memset(((dati_client*)arg)->msg_ricevuto,'\0',sizeof(((dati_client*)arg)->msg_ricevuto));
  check=read(((dati_client*)arg)->sock_id, ((dati_client*)arg)->msg_ricevuto, 2048);		
 if(check<0)
  printf("Errore nella ricezione del messaggio\n");
 else if(check==0){
  printf("Errore: il server si è disconnesso\n");
  continua=0;
 }
 else														
  printf("%s",((dati_client*)arg)->msg_ricevuto);			
 }
 write_th_end=0;
 pthread_exit(NULL);
}
