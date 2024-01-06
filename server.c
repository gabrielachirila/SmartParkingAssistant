/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
	Intoarce corect identificatorul din program al thread-ului.
  
   
   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <sqlite3.h>


/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
} thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

void createTables(sqlite3 *db);

void register_user(thData tdL, sqlite3 *db);
int login_user(thData tdL, sqlite3 *db);
int verify_username(char username[30],  sqlite3 *db);
void viewParkingAvailability(thData tdL, sqlite3 *db);

typedef struct {
  int success;
  char message[100];
} Response;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

sqlite3 *open_database_connection() {
  sqlite3 *db;
  int rc;

  rc = sqlite3_open("proiect-RC.db", &db);

  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(1);
  } else {
    fprintf(stderr, "Opened database successfully\n");
  }

  return db;
}

void close_database_connection(sqlite3 *db) {
  sqlite3_close(db);
}

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;

  sqlite3 *db = open_database_connection();
  char *error_message = 0;

  char *createTableQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, password TEXT);";
  if (sqlite3_exec(db, createTableQuery, 0, 0, &error_message) != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s \n", error_message);
    sqlite3_free(error_message);
    return -1;
  }

  createTables(db);

  close_database_connection(db);

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      // client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
	{
	  perror ("[server]Eroare la accept().\n");
	  continue;
	}
	
        /* s-a realizat conexiunea, se astepta mesajul */
    
	// int idThread; //id-ul threadului
	// int cl; //descriptorul intors de accept

 

	td=(struct thData*)malloc(sizeof(struct thData));	
	td->idThread=i++;
	td->cl=client;

	pthread_create(&th[i], NULL, &treat, td);	   
				
	}//while    
};				
static void *treat(void * arg)
{		
		struct thData tdL; 
		tdL= *((struct thData*)arg);	
		printf ("[Thread %d] - Asteptam mesajul...\n", tdL.idThread);
		fflush (stdout);		 
		pthread_detach(pthread_self());		
		raspunde((struct thData*)arg);
		/* am terminat cu acest client, inchidem conexiunea */
		close ((intptr_t)arg);
		return(NULL);	
  		
};


void raspunde(void *arg)
{
  int option, i=0;
	struct thData tdL; 
	tdL= *((struct thData*)arg);

  // Open the database connection
    sqlite3 *db = open_database_connection();
    char msgrasp[200]=" ";

    while(1) {
      
      	if (read (tdL.cl, &option, sizeof(int)) <= 0)
			{
			  printf("[Thread %d]\n",tdL.idThread);
			  perror ("Eroare la read() de la client.\n");
			
			}
	
      printf ("[Thread %d]Mesajul a fost receptionat...Optiunea %d\n",tdL.idThread, option);
      // Process the client request based on the received option
      if (option == 1) {
          printf("Optiunea 1 \n");
          register_user(tdL, db);
      } \
      else if (option == 2) {
          printf("Optiunea 2 \n");
          int ok = login_user(tdL, db);

          if (ok == 1) {
            int option1;

            while(1) {

              // dupa ce autentificarea s-a realizat cu succes, trimitem informatii dupa logare
              if (read (tdL.cl, &option1, sizeof(int)) <= 0)
              {
                printf("[Thread %d]\n",tdL.idThread);
                perror ("Eroare la read() de la client.\n");
              
              }

              if (option1 == 1)
              {
                printf("[Thread %d] Optiunea %d: View parking availability \n",tdL.idThread, option1);
                // viewParkingAvailability(tdl, db);
              }
              else if (option1 == 2)
              {
                printf("[Thread %d] Optiunea %d: View parking history \n",tdL.idThread, option1);
              }
              else if (option1 == 3)
              {
                printf("[Thread %d] Optiunea %d: Find nearest parking spot \n",tdL.idThread, option1);  
              }
              else if(option1 == 4)
              {
                printf("[Thread %d] Optiunea %d: Logout \n",tdL.idThread, option1);
                break;
              }
              else 
              {
                printf("[Thread %d] Optiunea %d: Optiune invalida \n",tdL.idThread, option1);
              }

            }
          }
      }
      else if (option == 3) {
          printf("Optiunea 3 \n");
          printf("[Thread %d] Clientul a iesit din aplicatie\n", tdL.idThread);

            /*pregatim mesajul de raspuns */
          bzero(msgrasp,200);
          strcat(msgrasp,"Clientul a iesit din aplicatie\n");
          printf("Mesajul de raspuns este: %s", msgrasp);

          if (write (tdL.cl , msgrasp, 200) <= 0)
          {
            printf("[Thread %d] ",tdL.idThread);
            perror ("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
            break;

      }
      else {
          printf("[Thread %d] Optiune invalida. Incercati din nou\n", tdL.idThread);
          /*pregatim mesajul de raspuns */
          bzero(msgrasp,200);
          strcat(msgrasp,"Optiune invalida. Incercati din nou\n");
          printf("Mesajul de raspuns este: %s", msgrasp);

          if (write (tdL.cl , msgrasp, 200) <= 0)
          {
            printf("[Thread %d] ",tdL.idThread);
            perror ("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
      }

    }
    

    // Close the database connection
    close_database_connection(db);

}

void register_user(thData tdL, sqlite3 *db) {
  char username[30];
  char password[30];

  bzero(username, 30);
  bzero(password, 30);

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  Response response;
  response.success = 0; // Default to failure
  printf("username primit: %s\n", username);
  printf("parola primita: %s\n", password);

  if (username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n') {
    printf("Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!\n");
    snprintf(response.message, sizeof(response.message), "Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!");
  } else {
    char *ErrMsg = 0;
    int rc;

    if (verify_username(username, db) == 0) {
      printf("Username-ul ales este deja folosit de alt utilizator. Incercati din nou! \n");
      snprintf(response.message, sizeof(response.message), "Username-ul %s este deja folosit de alt utilizator. Incercati altul!", username);
    } else if (verify_username(username, db) == 1) {
      char *insert_query = "INSERT INTO users (username, password) VALUES (?, ?)";
      sqlite3_stmt *stmt;

      rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, 0);

      if (rc != SQLITE_OK) {
        fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
        snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
      } else {
        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) {
          fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
          sqlite3_finalize(stmt);
          snprintf(response.message, sizeof(response.message), "Eroare la bind_text. Incearcati din nou!");
        } else {
          rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
          if (rc != SQLITE_OK) {
            fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            snprintf(response.message, sizeof(response.message), "Eroare la bind_text. Incearcati din nou!");
          } else {
            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE) {
              fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
              sqlite3_finalize(stmt);
              snprintf(response.message, sizeof(response.message), "Eroare la inserarea in baza de date. Incearcati din nou!");
            } else {
              sqlite3_finalize(stmt);
              sqlite3_exec(db, "COMMIT;", 0, 0, 0);
              response.success = 1;
              snprintf(response.message, sizeof(response.message), "Inregistrarea s-a realizat cu succes!");
            }
          }
        }
      }
    } else {
      printf("Eroare baza de date. Incearcati din nou!\n");
      snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
    }
  }

  // Send the response structure to the client
  if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
    perror("[Thread] Error writing to client.\n");
  }

  if (response.success) {
    printf("%s\n", response.message);
  }
}

int verify_username(char username_exists[30], sqlite3 *db)
{
    char *ErrMsg = 0;
    sqlite3_stmt *res;
    int rc;
    
    char select[200] = "SELECT id from users WHERE username=\'";
    strcat(select, username_exists);
    strcat(select, "\';");

    // printf("%s\n", select);
    fflush(stdout);

    rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

    if( rc != SQLITE_OK )
        {
            return 2; // eroare la select
        } 
    else
    {
        int step = sqlite3_step(res);

        if (step == SQLITE_ROW)
        {
          return 0; // am gasit un utilizator cu acest username
        }
        else 
        {
          return 1; // nu s-a gasit niciun utilizator cu acest username
        }
    }

}


int login_user(thData tdL, sqlite3 *db) {
    int ok = 0;
    char username[30];
    char password[30];

    bzero(username, 30);
    bzero(password, 30);

    if (read(tdL.cl, username, sizeof(username)) <= 0) {
        printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
    }

    if (read(tdL.cl, password, sizeof(password)) <= 0) {
        printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
    }

    Response response;
    response.success = 0; // Default to failure
    printf("username primit: %s\n", username);
    printf("parola primita: %s\n", password);

    if (username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n') {
        printf("Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!\n");
        snprintf(response.message, sizeof(response.message), "Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!");
    } else {
        char *ErrMsg = 0;
        sqlite3_stmt *res;
        int rc;

        char select[200] = "SELECT id from users WHERE username=\'";
        strcat(select, username);
        strcat(select, "\' and Password=\'");
        strcat(select, password);
        strcat(select, "\';");

        rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
            snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
        } else {
            int step = sqlite3_step(res);

            if (step == SQLITE_ROW) {
                response.success = 1;
                snprintf(response.message, sizeof(response.message), "Autentificarea s-a realizat cu succes!");
                ok = 1;
            } else {
                snprintf(response.message, sizeof(response.message), "Username/parola gresita. Incercati din nou!");
            }
        }
    }

    // Send the response structure to the client
    if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
        perror("[Thread] Error writing to client.\n");
    }

    if (response.success) {
        printf("%s\n", response.message);
    }

    if (ok == 1)
      return 1;
    else
      return 0;
}


void createTables(sqlite3 *db) {
    char *error_message = 0;

    // Drop tables if they already exist
    char *dropCitiesTableQuery = "DROP TABLE IF EXISTS cities;";
    char *dropAreasTableQuery = "DROP TABLE IF EXISTS areas;";
    char *dropParkingSpotsTableQuery = "DROP TABLE IF EXISTS parking_spots;";

    if (sqlite3_exec(db, dropParkingSpotsTableQuery, 0, 0, &error_message) != SQLITE_OK ||
        sqlite3_exec(db, dropAreasTableQuery, 0, 0, &error_message) != SQLITE_OK ||
        sqlite3_exec(db, dropCitiesTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }
    // Create cities table
    char *createCitiesTableQuery = "CREATE TABLE IF NOT EXISTS cities ("
                                   "    city_id INTEGER PRIMARY KEY,"
                                   "    city_name TEXT NOT NULL"
                                   ");";
    if (sqlite3_exec(db, createCitiesTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }

    // Populate cities table
char *populateCitiesTableQuery = "INSERT INTO cities (city_name) VALUES"
                                 "    ('Alba'),"
                                 "    ('Arad'),"
                                 "    ('Arges'),"
                                 "    ('Bacau'),"
                                 "    ('Bihor'),"
                                 "    ('Bistrita-Nasaud'),"
                                 "    ('Botosani'),"
                                 "    ('Braila'),"
                                 "    ('Brasov'),"
                                 "    ('Bucuresti'),"
                                 "    ('Buzau'),"
                                 "    ('Calarasi'),"
                                 "    ('Caras-Severin'),"
                                 "    ('Cluj'),"
                                 "    ('Constanta'),"
                                 "    ('Covasna'),"
                                 "    ('Dambovita'),"
                                 "    ('Dolj'),"
                                 "    ('Galati'),"
                                 "    ('Giurgiu'),"
                                 "    ('Gorj'),"
                                 "    ('Harghita'),"
                                 "    ('Hunedoara'),"
                                 "    ('Ialomita'),"
                                 "    ('Iasi'),"
                                 "    ('Ilfov'),"
                                 "    ('Maramures'),"
                                 "    ('Mehedinti'),"
                                 "    ('Mures'),"
                                 "    ('Neamt'),"
                                 "    ('Olt'),"
                                 "    ('Prahova'),"
                                 "    ('Salaj'),"
                                 "    ('Satu Mare'),"
                                 "    ('Sibiu'),"
                                 "    ('Suceava'),"
                                 "    ('Teleorman'),"
                                 "    ('Timis'),"
                                 "    ('Tulcea'),"
                                 "    ('Valcea'),"
                                 "    ('Vaslui'),"
                                 "    ('Vrancea');";

    if (sqlite3_exec(db, populateCitiesTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }

    // Create areas table with additional column for total_spots
    char *createAreasTableQuery = "CREATE TABLE IF NOT EXISTS areas ("
                                  "    area_id INTEGER PRIMARY KEY,"
                                  "    area_name TEXT NOT NULL,"
                                  "    city_id INTEGER,"
                                  "    total_spots INTEGER,"
                                  "    FOREIGN KEY (city_id) REFERENCES cities(city_id)"
                                  ");";
    if (sqlite3_exec(db, createAreasTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }
char *populateAreasTableQuery = "INSERT INTO areas (area_name, city_id, total_spots) VALUES"
                                // Areas for Alba County
                                "    ('Alba Mall Area', 1, 120),"
                                "    ('Alba City Center', 1, 90),"
                                // Areas for Arad County
                                "    ('Arad Shopping District', 2, 150),"
                                "    ('Arad Historic Center', 2, 180),"
                                // Areas for Arges County
                                "    ('Arges Downtown', 3, 80),"
                                "    ('Arges Old Town', 3, 120),"
                                // Areas for Bacau County
                                "    ('Bacau City Center', 4, 100),"
                                "    ('Bacau Coastal Area', 4, 130),"
                                "    ('Bacau Port Area', 4, 160),"
                                // Areas for Bihor County
                                "    ('Bihor Mall Area', 5, 120),"
                                "    ('Bihor City Center', 5, 90),"
                                // Areas for Bistrita-Nasaud County
                                "    ('Bistrita-Nasaud Shopping District', 6, 150),"
                                "    ('Bistrita-Nasaud Historic Center', 6, 180),"
                                // Areas for Botosani County
                                "    ('Botosani Downtown', 7, 80),"
                                "    ('Botosani Old Town', 7, 120),"
                                // Areas for Braila County
                                "    ('Braila City Center', 8, 100),"
                                "    ('Braila Coastal Area', 8, 130),"
                                "    ('Braila Port Area', 8, 160),"
                                // Areas for Brasov County
                                "    ('Brasov Mall Area', 9, 120),"
                                "    ('Brasov City Center', 9, 90),"
                                // Areas for Bucuresti (Bucharest) County
                                "    ('Bucharest Shopping District', 10, 150),"
                                "    ('Bucharest Historic Center', 10, 180),"
                                // Areas for Buzau County
                                "    ('Buzau Downtown', 11, 80),"
                                "    ('Buzau Old Town', 11, 120),"
                                // Areas for Calarasi County
                                "    ('Calarasi City Center', 12, 100),"
                                "    ('Calarasi Coastal Area', 12, 130),"
                                "    ('Calarasi Port Area', 12, 160),"
                                // Areas for Caras-Severin County
                                "    ('Caras-Severin Mall Area', 13, 120),"
                                "    ('Caras-Severin City Center', 13, 90),"
                                // Areas for Cluj County
                                "    ('Cluj Shopping District', 14, 110),"
                                "    ('Cluj Historic Center', 14, 40),"
                                // Areas for Constanta County
                                "    ('Constanta Downtown', 15, 70),"
                                "    ('Constanta Old Town', 15, 90),"
                                "    ('Constanta City Center', 15, 140),"
                                "    ('Constanta Coastal Area', 15, 100),"
                                "    ('Constanta Port Area', 15, 15),"
                                // Areas for Covasna County
                                "    ('Covasna Mall Area', 16, 180),"
                                "    ('Covasna City Center', 16, 150),"
                                // Areas for Dambovita County
                                "    ('Dambovita Shopping District', 17, 160),"
                                "    ('Dambovita Historic Center', 17, 120),"
                                // Areas for Dolj County
                                "    ('Dolj Downtown', 18, 90),"
                                "    ('Dolj Old Town', 18, 100),"
                                // Areas for Galati County
                                "    ('Galati City Center', 19, 170),"
                                "    ('Galati Coastal Area', 19, 110),"
                                "    ('Galati Port Area', 19, 90),"
                                // Areas for Giurgiu County
                                "    ('Giurgiu Mall Area', 20, 100),"
                                "    ('Giurgiu City Center', 20, 120),"
                                // Areas for Gorj County
                                "    ('Gorj Mall Area', 21, 140),"
                                "    ('Gorj City Center', 21, 120),"
                                // Areas for Harghita County
                                "    ('Harghita Shopping District', 22, 140),"
                                "    ('Harghita Historic Center', 22, 80),"
                                // Areas for Hunedoara County
                                "    ('Hunedoara Downtown', 23, 100),"
                                "    ('Hunedoara Old Town', 23, 90),"
                                // Areas for Ialomita County
                                "    ('Ialomita City Center', 24, 100),"
                                "    ('Ialomita Coastal Area', 24, 100),"
                                "    ('Ialomita Port Area', 24, 100),"
                                // Areas for Iasi County
                                "    ('Iasi Mall Area', 25, 100),"
                                "    ('Iasi City Center', 25, 100),"
                                // Areas for Ilfov County
                                "    ('Ilfov Shopping District', 26, 100),"
                                "    ('Ilfov Historic Center', 26, 100),"
                                // Areas for Maramures County
                                "    ('Maramures Mall Area', 27, 100),"
                                "    ('Maramures City Center', 27, 100),"
                                // Areas for Mehedinti County
                                "    ('Mehedinti Shopping District', 28, 100),"
                                "    ('Mehedinti Historic Center', 28, 100),"
                                // Areas for Mures County
                                "    ('Mures Downtown', 29, 100),"
                                "    ('Mures Old Town', 29, 100),"
                                // Areas for Neamt County
                                "    ('Neamt City Center', 30, 100),"
                                "    ('Neamt Coastal Area', 30, 100),"
                                "    ('Neamt Port Area', 30, 100),"
                                // Areas for Olt County
                                "    ('Olt Mall Area', 31, 100),"
                                "    ('Olt City Center', 31, 100),"
                                // Areas for Prahova County
                                "    ('Prahova Shopping District', 32, 100),"
                                "    ('Prahova Historic Center', 32, 100),"
                                // Areas for Salaj County
                                "    ('Salaj Mall Area', 33, 100),"
                                "    ('Salaj City Center', 33, 100),"
                                // Areas for Satu Mare County
                                "    ('Satu Mare Shopping District', 34, 100),"
                                "    ('Satu Mare Historic Center', 34, 100),"
                                // Areas for Sibiu County
                                "    ('Sibiu Downtown', 35, 100),"
                                "    ('Sibiu Old Town', 35, 100),"
                                // Areas for Suceava County
                                "    ('Suceava City Center', 36, 100),"
                                "    ('Suceava Coastal Area', 36, 100),"
                                "    ('Suceava Port Area', 36, 100),"
                                // Areas for Teleorman County
                                "    ('Teleorman Mall Area', 37, 100),"
                                "    ('Teleorman City Center', 37, 100),"
                                // Areas for Timis County
                                "    ('Timis Shopping District', 38, 100),"
                                "    ('Timis Historic Center', 38, 100),"
                                // Areas for Tulcea County
                                "    ('Tulcea Downtown', 39, 100),"
                                "    ('Tulcea Old Town', 39, 100),"
                                // Areas for Valcea County
                                "    ('Valcea Mall Area', 40, 100),"
                                "    ('Valcea City Center', 40, 100),"
                                // Areas for Vaslui County
                                "    ('Vaslui Shopping District', 41, 100),"
                                "    ('Vaslui Historic Center', 41, 100),"
                                // Areas for Vrancea County
                                "    ('Vrancea Downtown', 42, 100),"
                                "    ('Vrancea Old Town', 42, 100);";

    if (sqlite3_exec(db, populateAreasTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "aici ares SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }
}