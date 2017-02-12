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
#include <strings.h>

#define MAX_CLIENT 128

//definizione coda dei messaggi
typedef struct a{
 char msg[1024];
 int client;
 int letture;
}info_mess;

typedef struct b{
 info_mess info;
 struct b *next;
}nodo;

typedef struct c{
 nodo *primo;
 nodo *ultimo;
 int connessi;
}coda;

void initCoda (coda *c);
int testCodaVuota(coda c);
void OutCoda(coda *c);
void incoda(coda *c, info_mess e);
info_mess InizioCoda(coda c);
//fine definizione coda dei messaggi

typedef struct user{
 int user_id;
 int sockcl_id;
 char user[128];	
 int write_th_end;
 int leggi;
 int connesso;
 int stanza;
 coda *coda_rif;		//riferimento alla coda corrente sulla quale lavora il client
 int moderatore;
}dati_client;

struct free_slot{
 int free;		
 int count;		
 int slots[128];
};

void *ricevi(void *arg);
void *client(void *arg);
void leggi(dati_client *utentei);
int check_slot(struct free_slot *pos_lib);
void mod_slot(struct free_slot *pos_lib, int current_client);
int scrivi_cl(char *stringa, dati_client *utentei);
int leggi_cl(char *stringa, dati_client *utentei);
int stanza_privata_in(dati_client *utentei);
int stanza_privata_out(dati_client *utentei);
int ricerca_utente(char *utente, dati_client *utentei);

dati_client utente[128];
int client_connessi=-1;
struct free_slot pos_lib;
coda coda_mess;		//coda stanza pubblica
coda coda_priv[8];	//vettore con le code delle 8 stanze private
char mod_password[128]="modpass\n";

int main(void){
 
 int sock_id, sock_cl, check, current_client, continua;
 char utente_str[128];
 info_mess msg_pack;
 pthread_t tid[MAX_CLIENT];
 struct sockaddr_in address;			
 pos_lib.free=1;
 pos_lib.count=0;
 
 initCoda (&coda_mess);
 for(int i=0; i<8; i++)
  initCoda(&coda_priv[i]);
 printf("\nChat by Salvatore Cavallaro - Applicazione SERVER\n\nAvvio in corso...\n\n"); 

 sock_id=socket(AF_INET, SOCK_STREAM, 0);
 if(sock_id<0){
  printf("Errore nella creazione del socket - Uscita...\n");
  exit(1);
 }
 address.sin_family=AF_INET;
 address.sin_port=htons(2017);
 address.sin_addr.s_addr=htonl(INADDR_ANY);
 if(bind(sock_id, (struct sockaddr*)&address, sizeof(address))<0){
  printf("Errore durante l'operazione di bind() - Uscita...\n");
  exit(1);
 }
 if(listen(sock_id, MAX_CLIENT)<0){			//Il server può gestire un massimo di 128 connessioni con client contemporaneamente
  printf("Errore durante la listen() - Uscita...\n");
  exit(1);
 }
/*******SERVER CONFIGURATO********/
 printf("Il server e' stato configurato e avviato correttamente!\n\n");
 printf("La password di accesso per il moderatore è: %s\n\n", mod_password);
 printf("In attesa della connessione con il primo client...\n\n");
 
 while(1){						
  if((sock_cl=accept(sock_id, NULL, NULL))<0)	
   printf("Errore durante la accept()\n");
  else{
   if(client_connessi<128){
    client_connessi++;
    current_client=check_slot(&pos_lib);
    if(current_client==-1)
     current_client=client_connessi;
    utente[current_client].user_id=current_client;		//nella struttura dati che descrive il client salvo il suo identif.
    utente[current_client].sockcl_id=sock_cl;			//nella struttura dati che descrive il client salvo il suo identif. socket
    utente[current_client].stanza=0;				//imposto come stanza iniziale la 0 (pubblica)
    utente[current_client].coda_rif=&coda_mess;			//imposto la coda della stanza pubblica
    utente[current_client].moderatore=0;			//imposto la modalità iniziale non moderatore
    utente[current_client].connesso=1;    			//imposto il client come connesso
    do{
     memset(utente_str,'\0',sizeof(utente_str));
     check=read(utente[current_client].sockcl_id, utente_str, 128);    
     if(check<=0){	
      printf("Errore nella lettura dello user dell'utente\n");
      close(sock_cl);
      break;					
     }
     if(ricerca_utente(utente_str, utente)){
      check=write(utente[current_client].sockcl_id, "NO", 128);
      continua=1;
     }  
     else{
      check=write(utente[current_client].sockcl_id, "SI", 128);
      continua=0;
     }
     if(check<0){	
      printf("Errore nell'invio di info al client\n");
      close(sock_cl);
      break;					
     }  
    }while(continua);
    if(check>0){
     strcpy(utente[current_client].user, utente_str);
     printf("Username dell'utente %s impostato correttamente\n\n", utente[current_client].user);
     if(pthread_create(&tid[current_client], NULL, client, (void*)&utente[current_client])<0){
      printf("Errore nella creazione del thread di gestione del client che sta tentando la connessione\n");
      close(sock_cl);
     }
    }
   }  
   else{
    printf("Tentativo di connessione di un client oltre il limite dei 128\n");
    close(sock_cl);
    exit(1);
   }
  }
 } 
}

int check_slot(struct free_slot *pos_lib){
 if(pos_lib->free==1)
  return -1;
 else{
  if(pos_lib->count==1)
   pos_lib->free=1;
  pos_lib->count--;
  return pos_lib->slots[pos_lib->count];
 }
}

void mod_slot(struct free_slot *pos_lib, int current_client){
 if(pos_lib->free==1)
  pos_lib->free=0;
 pos_lib->slots[pos_lib->count]=current_client;
 pos_lib->count++;
}

void *ricevi(void *arg){		//questa funzione si occuperà di ricevere i messaggi dalla coda corrente e inviarli al client
 dati_client *utentei=(dati_client*)arg;
 info_mess msg_pack;
 int check;
 char messaggio[2048], nstanza[2];
 utentei->leggi=1;
 while(utentei->write_th_end==1){
  if((!testCodaVuota(*(utentei->coda_rif)))&&utentei->leggi){
   msg_pack=InizioCoda(*(utentei->coda_rif));
   if(msg_pack.client!=utentei->user_id){
    memset(messaggio,'\0',sizeof(messaggio));		
    if(utente[msg_pack.client].moderatore==1)
     strcat(messaggio, "MESSAGGIO DAL MODERATORE ");
     strcat(messaggio, utente[msg_pack.client].user);
    if(utentei->stanza<1)
     strcat(messaggio, " [STANZA PUBBLICA]: ");
    else{
     nstanza[0]=48+utentei->stanza;
     nstanza[1]='\0';
     strcat(messaggio, " [STANZA PRIVATA ");
     strcat(messaggio, nstanza);
     strcat(messaggio, "]: ");     
    }
    strcat(messaggio, msg_pack.msg);
    check=write(utentei->sockcl_id, messaggio, 2048);
    if(check<0)
     printf("Errore nella ricezione del messaggio dal server per l'utente %s\n", utentei->user);
    else{
     utentei->leggi=0;
     (*(utentei->coda_rif)).primo->info.letture++;
     if((*(utentei->coda_rif)).connessi==(*(utentei->coda_rif)).primo->info.letture+1){
      OutCoda(&(*(utentei->coda_rif)));
      for(int i=0;i<client_connessi+1;i++){
       if((utente[i].leggi!=-1)&&(utente[i].stanza==utentei->stanza))
        utente[i].leggi=1;
      }
     }
    }
   }
  }
 }
 pthread_exit(NULL); 
}


void *client(void *arg){
 int continua=1, check, tentativi=3, stanzab;
 char password[128], messaggio[1024], user[128];
 info_mess msg_pack;
 pthread_t thread_ricevi;
 ((dati_client*)arg)->write_th_end=1;
 (*(((dati_client*)arg)->coda_rif)).connessi++;
 if(pthread_create(&thread_ricevi, NULL, ricevi, arg)<0){
  printf("Errore nella creazione del thread di ricezione dei messaggi dell'utente %s - Uscita...\n", ((dati_client*)arg)->user);
  ((dati_client*)arg)->write_th_end=0;
 }
 else{
  printf("L'utente %s si è connesso, client connessi: %d\n", ((dati_client*)arg)->user, client_connessi+1);
  while(continua){
   memset(msg_pack.msg,'\0',sizeof(msg_pack.msg));
   check=read(((dati_client*)arg)->sockcl_id, msg_pack.msg, 1024);		//lettura messaggio da client
    if(check<0){
     printf("Errore durante la lettura del messaggio da parte dell'utente %s\n", ((dati_client*)arg)->user);
     continua=0;
     ((dati_client*)arg)->write_th_end=0;
    }
    else if(check==0){
     continua=0;
     ((dati_client*)arg)->write_th_end=0;
    }
    else{ //da questo punto controllo i messaggi che mi arrivano e smisto a seconda che siano 1. comandi 2. messaggi (else if)
     if(strcmp(msg_pack.msg, "/privatein\n")==0){	
      (*(((dati_client*)arg)->coda_rif)).connessi--;
      check=stanza_privata_in(((dati_client*)arg));
      if(check<0){
       scrivi_cl("SERVER [YOU]: Inserito valore non valido\n", ((dati_client*)arg));
       printf("Errore durante la lettura del comando inserito da parte dell'utente %s\n", ((dati_client*)arg)->user);
       continua=0;
       ((dati_client*)arg)->write_th_end=0;
      }
      else if(check==0){
       continua=0;
       ((dati_client*)arg)->write_th_end=0;
      }
     }
     else if(strcmp(msg_pack.msg, "/privateout\n")==0){
      check=stanza_privata_out(((dati_client*)arg));
      if(check<1){
       printf("Errore durante la lettura del messaggio da parte dell'utente %s\n", ((dati_client*)arg)->user);
       continua=0;
       ((dati_client*)arg)->write_th_end=0;
      }
      else
       (*(((dati_client*)arg)->coda_rif)).connessi++;
     }
     else if(strcmp(msg_pack.msg, "/help\n")==0){
      scrivi_cl("\nSERVER [YOU]: Lista e descrizione dei comandi disponibili per l'utente\n\n", ((dati_client*)arg));
      scrivi_cl("/help VISUALIZZAZIONE DI TUTTI I COMANDI DISPONIBILI\n\n", ((dati_client*)arg));
      scrivi_cl("/quit DISCONNESSIONE DAL SERVER E CHIUSURA DEL CLIENT\n\n", ((dati_client*)arg));
      scrivi_cl("/privatein INGRESSO NELLE STANZE PRIVATE [1-8]\n\n", ((dati_client*)arg));
      scrivi_cl("/privateout USCITA DA UNA STANZA PRIVATA E RITORNO ALLA STANZA PUBBLICA\n\n", ((dati_client*)arg));
      scrivi_cl("/connessi VISUALIZZAZIONE DEGLI UTENTI CONNESSI IN UNA SPECIFICA STANZA\n\n", ((dati_client*)arg));
      scrivi_cl("/moderatore SETTAGGIO MODALITA' MODERATORE PREVIA CONOSCENZA DELLA PASSWORD\n\n", ((dati_client*)arg));
      scrivi_cl("/broadcast INVIO DI MESSAGGI IN MODALITA' BROADCAST A TUTTI GLI UTENTI DELLE STANZE PRIVATE\n\n", ((dati_client*)arg));
     }
     else if(strcmp(msg_pack.msg, "/connessi\n")==0){
      scrivi_cl("SERVER [YOU]: Inserisci la stanza [1-8] per vedere gli utenti connessi, 9 per la pubblica\n", ((dati_client*)arg));
      memset(messaggio,'\0',sizeof(messaggio));
      leggi_cl(messaggio, ((dati_client*)arg));
      stanzab=atoi(messaggio);
      if((stanzab<1)||(stanzab>9))
       scrivi_cl("SERVER [YOU]: Valore della stanza inserito scorretto\n", ((dati_client*)arg));
      else{
       if(stanzab==9) 
        stanzab=0;
       scrivi_cl("SERVER [YOU]: Elenco utenti\n", ((dati_client*)arg));
       if(((stanzab==0)&&(coda_mess.connessi==0))||(((stanzab>0)&&(stanzab<9))&&(coda_priv[stanzab-1].connessi==0)))
        scrivi_cl("Non c'è nessun utente connesso nella stanza selezionata\n", ((dati_client*)arg));
       else{
        for(int i=0;i<client_connessi+1;i++)
         if((utente[i].stanza==stanzab)&&(utente[i].connesso==1)){
          memset(user,'\0',sizeof(user));
          strcpy(user, utente[i].user);
          strcat(user, "\n");
          scrivi_cl(user, ((dati_client*)arg));
         }
       }         
      }
     }
     else if(strcmp(msg_pack.msg, "/moderatore\n")==0){ //se possibile delegare ad una funzione
      if(((dati_client*)arg)->moderatore>0)
       scrivi_cl("SERVER [YOU]: E' stata già impostata la modalità MODERATORE\n", ((dati_client*)arg));
      else{
       scrivi_cl("SERVER [YOU]: Inserisci la password MODERATORE\n", ((dati_client*)arg));
       leggi_cl(password, ((dati_client*)arg));
       check=strcmp(password, mod_password);
       if(check!=0)
        while(tentativi>0){
         scrivi_cl("SERVER [YOU]: PASSWORD ERRATA! Hai solo 3 tentativi per sessione. Inserisci la password MODERATORE\n", ((dati_client*)arg));
         memset(password,'\0',sizeof(password));
         leggi_cl(password, ((dati_client*)arg));
         if(strcmp(password, mod_password)==0)
          break;
         tentativi--;
        }
        if(tentativi<1){
         scrivi_cl("SERVER [YOU]: Hai sbagliato la password MODERATORE 3 volte, sarai disconnesso\n", ((dati_client*)arg));
         continua=0;
         ((dati_client*)arg)->write_th_end=0;
        }      
       else if(check==0||tentativi>0){
        scrivi_cl("SERVER [YOU]: Password corretta! Sei in modalità moderatore\n", ((dati_client*)arg));
        ((dati_client*)arg)->moderatore=1;
       }
      }
     }
     else if(strcmp(msg_pack.msg, "/broadcast\n")==0){ //Il messaggio in broadcast per scelta progettuale viene inviato solo alle private rooms
      if(((dati_client*)arg)->moderatore==0)
       scrivi_cl("SERVER [YOU]: Non è possibile accedere al comando in quanto non si è moderatori\n", ((dati_client*)arg));
      else{
       if(client_connessi>0){
        scrivi_cl("SERVER [YOU]: Inserisci il messaggio da inviare in broadcast\n", ((dati_client*)arg));
        leggi_cl(*(&(msg_pack.msg)), ((dati_client*)arg));
        msg_pack.client=((dati_client*)arg)->user_id;
        msg_pack.letture=0;
        for(int i=0;i<8;i++)
         incoda(&coda_priv[i], msg_pack);
       }
       else
        scrivi_cl("SERVER [YOU]: Non c'è nessun utente connesso\n", ((dati_client*)arg));
      }
     }
     else{ 
      if((*(((dati_client*)arg)->coda_rif)).connessi>1){ 
       msg_pack.client=((dati_client*)arg)->user_id;
       msg_pack.letture=0;
       incoda(&(*(((dati_client*)arg)->coda_rif)), msg_pack);	
      }			
     }
    }
   }
  }
 if(client_connessi!=((dati_client*)arg)->user_id)		//l'utente che si è disconnesso non e' in ultima posizione
  mod_slot(&pos_lib, ((dati_client*)arg)->user_id);
 (*(((dati_client*)arg)->coda_rif)).connessi--;
 client_connessi--;
 ((dati_client*)arg)->connesso=0;
 printf("L'utente %s si è disconnesso, client connessi: %d\n", ((dati_client*)arg)->user, client_connessi+1);
 close(((dati_client*)arg)->sockcl_id);
 pthread_exit(NULL); 
}

int stanza_privata_in(dati_client *utentei){ 
 int check, stanza;
 char messaggio[1024];
 utentei->leggi=-1;	//nella fase di inserimento di un comando disabilito la chat pubblica per una migliore visibilità dei comandi da shell
 check=scrivi_cl("SERVER [YOU]: Inserisci la stanza privata alla quale vuoi accedere[1-8]:\n", utentei);
 if(check<0)
  return check;
 check=leggi_cl(messaggio, utentei);
 stanza=atoi(messaggio);
 if((stanza<1)||(stanza>8))
  return -1;
 utentei->stanza=stanza;
 utentei->coda_rif=&coda_priv[stanza-1];
 (*(utentei->coda_rif)).connessi++;
 utentei->leggi=1; 
 check=scrivi_cl("SERVER [YOU]: Hai abbandonato la STANZA PUBBLICA e avuto accesso alla STANZA PRIVATA\n", utentei);
 return check;
}

int stanza_privata_out(dati_client *utentei){
 int check=1;
 if((utentei->stanza)>0){
  utentei->leggi=-1;
  utentei->stanza=0;
  utentei->coda_rif=&coda_mess;
  (*(utentei->coda_rif)).connessi--;
  check=scrivi_cl("SERVER [YOU]: Hai abbandonato la STANZA PRIVATA e sei ritornato alla STANZA PUBBLICA\n", utentei);
  utentei->leggi=1;
 }
 else{
  check=scrivi_cl("SERVER [YOU]: Non ti trovi in nessuna stanza privata\n", utentei);
 }
 return check;
}

int scrivi_cl(char *stringa, dati_client *utentei){
 int check;
 char buf[2048];
 strcpy(buf, stringa);
 check=write(utentei->sockcl_id, buf, 2048);
 if(check<0)
  printf("Errore nella ricezione del messaggio dal server per l'utente %s\n", utentei->user);
 return check;
}

int leggi_cl(char *stringa, dati_client *utentei){
 int check;
 check=read(utentei->sockcl_id, stringa, 1024);
 if(check<0)
  printf("Errore durante la lettura del messaggio da parte dell'utente %s", utentei->user);
 return check; 
}

int ricerca_utente(char *utente, dati_client *utentei){
 for(int i=0; i<client_connessi+1;i++)
  if(utentei[i].connesso==1)
   if(strcmp(utentei->user, utente)==0)
    return 1;
 return 0;
}

//funzioni coda di messaggi
void initCoda (coda *c){
 c->connessi=0;
 c->primo = c->ultimo = NULL;
}

int testCodaVuota(coda c){
 return c.primo==NULL;
}

void OutCoda(coda *c) {
 nodo *aux;
 if(!testCodaVuota(*c)) {
  aux = c->primo;
  c->primo = c->primo->next;
  free(aux);
  if (c->primo==NULL) c->ultimo =NULL;
 }
}

void incoda(coda *c, info_mess e){
 nodo *aux;
 aux = (nodo *) malloc (sizeof(nodo));
 aux -> info = e;
 aux->next = NULL;
 if (testCodaVuota(*c)) 
  c->primo=c->ultimo=aux;
 else{
  c->ultimo->next = aux;
  c->ultimo = aux;
 }
}

info_mess InizioCoda(coda c) {
 if (!testCodaVuota(c)) 
  return c.primo->info; 
}
