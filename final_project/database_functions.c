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

#include "database_functions.h"
#include "response.h"
#include "constants.h"

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

void createTables(sqlite3 *db) {
    char *error_message = 0;

    // Drop tables if they already exist
    char *dropCitiesTableQuery = "DROP TABLE IF EXISTS cities;";
    char *dropAreasTableQuery = "DROP TABLE IF EXISTS areas;";

    if (sqlite3_exec(db, dropAreasTableQuery, 0, 0, &error_message) != SQLITE_OK ||
        sqlite3_exec(db, dropCitiesTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }

    //create users table
    char *createTableQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, password TEXT);";
    if (sqlite3_exec(db, createTableQuery, 0, 0, &error_message) != SQLITE_OK) {
      fprintf(stderr, "SQL error: %s \n", error_message);
      sqlite3_free(error_message);
      return;
    }

    // Create reservations table
    char *createReservationsTableQuery = "CREATE TABLE IF NOT EXISTS reservations ("
                                         "    reservation_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                         "    user_id INTEGER,"
                                         "    city_name TEXT,"
                                         "    area_name TEXT,"
                                         "    reservation_date TEXT,"
                                         "    FOREIGN KEY (user_id) REFERENCES users(id)"
                                         ");";
    if (sqlite3_exec(db, createReservationsTableQuery, 0, 0, &error_message) != SQLITE_OK) {
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
                                "    ('Alba Mall Area', 1, 120),"
                                "    ('Alba City Center', 1, 90),"
                                "    ('Arad Shopping District', 2, 150),"
                                "    ('Arad Historic Center', 2, 180),"
                                "    ('Arges Downtown', 3, 80),"
                                "    ('Arges Old Town', 3, 120),"
                                "    ('Bacau City Center', 4, 100),"
                                "    ('Bacau Coastal Area', 4, 130),"
                                "    ('Bacau Port Area', 4, 160),"
                                "    ('Bihor Mall Area', 5, 120),"
                                "    ('Bihor City Center', 5, 90),"
                                "    ('Bistrita-Nasaud Shopping District', 6, 150),"
                                "    ('Bistrita-Nasaud Historic Center', 6, 180),"
                                "    ('Botosani Downtown', 7, 80),"
                                "    ('Botosani Old Town', 7, 120),"
                                "    ('Braila City Center', 8, 100),"
                                "    ('Braila Coastal Area', 8, 130),"
                                "    ('Braila Port Area', 8, 160),"
                                "    ('Brasov Mall Area', 9, 120),"
                                "    ('Brasov City Center', 9, 90),"
                                "    ('Bucharest Shopping District', 10, 150),"
                                "    ('Bucharest Historic Center', 10, 180),"
                                "    ('Buzau Downtown', 11, 80),"
                                "    ('Buzau Old Town', 11, 120),"
                                "    ('Calarasi City Center', 12, 100),"
                                "    ('Calarasi Coastal Area', 12, 130),"
                                "    ('Calarasi Port Area', 12, 160),"
                                "    ('Caras-Severin Mall Area', 13, 120),"
                                "    ('Caras-Severin City Center', 13, 90),"
                                "    ('Cluj Shopping District', 14, 110),"
                                "    ('Cluj Historic Center', 14, 40),"
                                "    ('Constanta Downtown', 15, 70),"
                                "    ('Constanta Old Town', 15, 90),"
                                "    ('Constanta City Center', 15, 140),"
                                "    ('Constanta Coastal Area', 15, 100),"
                                "    ('Constanta Port Area', 15, 15),"
                                "    ('Covasna Mall Area', 16, 180),"
                                "    ('Covasna City Center', 16, 150),"
                                "    ('Dambovita Shopping District', 17, 160),"
                                "    ('Dambovita Historic Center', 17, 120),"
                                "    ('Dolj Downtown', 18, 90),"
                                "    ('Dolj Old Town', 18, 100),"
                                "    ('Galati City Center', 19, 170),"
                                "    ('Galati Coastal Area', 19, 110),"
                                "    ('Galati Port Area', 19, 90),"
                                "    ('Giurgiu Mall Area', 20, 100),"
                                "    ('Giurgiu City Center', 20, 120),"
                                "    ('Gorj Mall Area', 21, 140),"
                                "    ('Gorj City Center', 21, 120),"
                                "    ('Harghita Shopping District', 22, 140),"
                                "    ('Harghita Historic Center', 22, 80),"
                                "    ('Hunedoara Downtown', 23, 100),"
                                "    ('Hunedoara Old Town', 23, 90),"
                                "    ('Ialomita City Center', 24, 100),"
                                "    ('Ialomita Coastal Area', 24, 100),"
                                "    ('Ialomita Port Area', 24, 100),"
                                "    ('Iasi Mall Area', 25, 100),"
                                "    ('Iasi City Center', 25, 100),"
                                "    ('Ilfov Shopping District', 26, 100),"
                                "    ('Ilfov Historic Center', 26, 100),"
                                "    ('Maramures Mall Area', 27, 100),"
                                "    ('Maramures City Center', 27, 100),"
                                "    ('Mehedinti Shopping District', 28, 100),"
                                "    ('Mehedinti Historic Center', 28, 100),"
                                "    ('Mures Downtown', 29, 100),"
                                "    ('Mures Old Town', 29, 100),"
                                "    ('Neamt City Center', 30, 100),"
                                "    ('Neamt Coastal Area', 30, 100),"
                                "    ('Neamt Port Area', 30, 100),"
                                "    ('Olt Mall Area', 31, 100),"
                                "    ('Olt City Center', 31, 100),"
                                "    ('Prahova Shopping District', 32, 100),"
                                "    ('Prahova Historic Center', 32, 100),"
                                "    ('Salaj Mall Area', 33, 100),"
                                "    ('Salaj City Center', 33, 100),"
                                "    ('Satu Mare Shopping District', 34, 100),"
                                "    ('Satu Mare Historic Center', 34, 100),"
                                "    ('Sibiu Downtown', 35, 100),"
                                "    ('Sibiu Old Town', 35, 100),"
                                "    ('Suceava City Center', 36, 100),"
                                "    ('Suceava Coastal Area', 36, 100),"
                                "    ('Suceava Port Area', 36, 100),"
                                "    ('Teleorman Mall Area', 37, 100),"
                                "    ('Teleorman City Center', 37, 100),"
                                "    ('Timis Shopping District', 38, 100),"
                                "    ('Timis Historic Center', 38, 100),"
                                "    ('Tulcea Downtown', 39, 100),"
                                "    ('Tulcea Old Town', 39, 100),"
                                "    ('Valcea Mall Area', 40, 100),"
                                "    ('Valcea City Center', 40, 100),"
                                "    ('Vaslui Shopping District', 41, 100),"
                                "    ('Vaslui Historic Center', 41, 100),"
                                "    ('Vrancea Downtown', 42, 100),"
                                "    ('Vrancea Old Town', 42, 100);";

    if (sqlite3_exec(db, populateAreasTableQuery, 0, 0, &error_message) != SQLITE_OK) {
        fprintf(stderr, "aici areas SQL error: %s \n", error_message);
        sqlite3_free(error_message);
        return;
    }
}