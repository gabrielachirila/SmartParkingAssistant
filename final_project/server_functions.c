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
#include <time.h>

#include "server_functions.h" 
#include "database_functions.h"
#include "response.h"
#include "constants.h"
#include "thread_data.h"

int get_user_id_from_database(char username[MAX_USERNAME_LENGTH], sqlite3 *db) {
    char query[100]; 
    snprintf(query, sizeof(query), "SELECT id FROM users WHERE username='%s';", username);

    sqlite3_stmt *stmt;
    int userId = -1; 

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userId = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "[server] SQL error: %s\n", sqlite3_errmsg(db));
    }

    return userId;
}

void register_user(thData tdL) {
  char username[MAX_USERNAME_LENGTH];
  char password[MAX_PASSWORD_LENGTH];

  bzero(username, MAX_USERNAME_LENGTH);
  bzero(password, MAX_PASSWORD_LENGTH);

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  Response response;
  response.success = 0; 

  // printf("username primit: %s\n", username);
  // printf("parola primita: %s\n", password);

  sqlite3 *db = open_database_connection();

  if (sqlite3_exec(db, "BEGIN;", 0, 0, 0) != SQLITE_OK) {
    fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
  }

  if (username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n') {
    printf("Complete with valid username and/or password!\n");
    snprintf(response.message, sizeof(response.message), "Complete with valid username and/or password!\n");
  } 
  else {
    char *ErrMsg = 0;
    int rc;

    if (verify_username(username, db) == 0) {
      printf("Username already used... Try again! \n");
      snprintf(response.message, sizeof(response.message), "Username: %s already used... Try again! \n", username);
    } 
    else if (verify_username(username, db) == 1) {
      char *insert_query = "INSERT INTO users (username, password) VALUES (?, ?)";
      sqlite3_stmt *stmt;

      rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, 0);

      if (rc != SQLITE_OK) {
        fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
        snprintf(response.message, sizeof(response.message), "Db error. Try again!\n");
      } 
      else {
        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) {
          fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
          sqlite3_finalize(stmt);
          snprintf(response.message, sizeof(response.message), "Try again!\n");
        } 
        else {
          rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
          if (rc != SQLITE_OK) {
            fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            snprintf(response.message, sizeof(response.message), "Try again!\n");
          } 
          else {
            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE) {
              fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
              sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);  
              sqlite3_finalize(stmt);
              snprintf(response.message, sizeof(response.message), "Try again!\n");
            } 
            else {
              const char *finalSql = sqlite3_sql(stmt);
              printf("Final SQL: %s\n", finalSql);
              sqlite3_finalize(stmt);
              sqlite3_exec(db, "COMMIT;", 0, 0, 0);
              response.success = 1;
              snprintf(response.message, sizeof(response.message), "Registration successful!\n");
            }
          }
        }
       }
    } 
    else {
      printf("Db error. Try again!\n");
      sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
      snprintf(response.message, sizeof(response.message), "Db error. Try again!\n");
    }
  }
  
  printf("close connection \n");
  close_database_connection(db);

  if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
    perror("[Thread] Error writing to client.\n");
  }

  if (response.success) {
    printf("%s\n", response.message);
  }
}

int verify_username(char username_exists[MAX_USERNAME_LENGTH], sqlite3 *db) {
    sqlite3_stmt *stmt;
    int rc;

    const char *select_query = "SELECT id FROM users WHERE username = ?";
    rc = sqlite3_prepare_v2(db, select_query, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 2; 
    }

    rc = sqlite3_bind_text(stmt, 1, username_exists, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 2;
    }

    int step = sqlite3_step(stmt);

    if (step == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 0; // User with this username exists
    } else {
        sqlite3_finalize(stmt);
        return 1; // No user with this username
    }
}


int login_user(thData *tdL) {
    int ok = 0;

    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    bzero(username, MAX_USERNAME_LENGTH);
    bzero(password, MAX_PASSWORD_LENGTH);

    if (read(tdL->cl, username, sizeof(username)) <= 0) {
        printf("[Thread %d] Error reading username from client. \n", tdL->idThread);
    }

    if (read(tdL->cl, password, sizeof(password)) <= 0) {
        printf("[Thread %d] Error reading password from client. \n", tdL->idThread);
    }

    Response response;
    response.success = 0; 

    sqlite3 *db = open_database_connection();

    if (username[0] == '\0' || password[0] == '\0') {
        printf("Complete with valid username and/or password!\n");
        snprintf(response.message, sizeof(response.message), "Complete with valid username and/or password!\n");
    } else {
        char select[200] = "SELECT id FROM users WHERE username=? AND password=?;";
        sqlite3_stmt *res;
        int rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "[server] SQL error: %s\n", sqlite3_errmsg(db));
            snprintf(response.message, sizeof(response.message), "Db error... Try again!\n");
        } else {
            sqlite3_bind_text(res, 1, username, -1, SQLITE_STATIC);
            sqlite3_bind_text(res, 2, password, -1, SQLITE_STATIC);

            int step = sqlite3_step(res);

            if (step == SQLITE_ROW) {
                response.success = 1;
                tdL->userId = get_user_id_from_database(username, db);
                printf("tdL.userId = %d \n", tdL->userId);
                snprintf(response.message, sizeof(response.message), "Authentication successful!\n");
                ok = 1;
            } else {
                snprintf(response.message, sizeof(response.message), "Username or password incorrect... Try again!\n");
            }
            
            sqlite3_finalize(res);
        }
    }

    close_database_connection(db);

    if (write(tdL->cl, &response, sizeof(Response)) <= 0) {
        perror("[Thread] Error writing to client.\n");
    }

    if (response.success) {
        printf("%s\n", response.message);
    }

    return ok;
}


int isNumeric(const char *str) {
    char *endptr;
    strtol(str, &endptr, 10);

    // If the conversion stopped at the first non-numeric character, the string is numeric
    return (*endptr == '\0');
}

void viewParkingAvailability(thData tdL) {
    Response response;
    char message[MAX_MESSAGE_LENGTH];
    bzero(message, MAX_MESSAGE_LENGTH);

    char cityList[MAX_CITY_LIST_LENGTH];
    bzero(cityList, MAX_CITY_LIST_LENGTH);

    sqlite3 *db;
    db = open_database_connection();

    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0); 

    const char *getCitiesQuery = "SELECT city_name FROM cities";
    sqlite3_stmt *citiesStmt;

    int rc = sqlite3_prepare_v2(db, getCitiesQuery, -1, &citiesStmt, 0);
    if (rc != SQLITE_OK) {
    sprintf(message, "Error preparing SQL statement.\n");
    return;
    }

    int id = 1;

    while (sqlite3_step(citiesStmt) == SQLITE_ROW) {
    char idStr[20];
    sprintf(idStr, "%d", id);

    strcat(cityList, idStr);
    strcat(cityList, ". ");
    strcat(cityList, (const char *)sqlite3_column_text(citiesStmt, 0));
    strcat(cityList, "\n");

    id++;
    }

    sqlite3_finalize(citiesStmt);

    sqlite3_exec(db, "COMMIT", 0, 0, 0);  
    close_database_connection(db);


    // send a list of cities to client so that he can choose one to park
    if (write(tdL.cl, cityList, sizeof(cityList)) <= 0) {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread] Error writing to client.\n");
      return;
    }

    int validCity = 0;
    int cityID;
    char cityName[MAX_CITY_LENGTH];

    while (!validCity) {
        // read the city selected by client
        char selectedCity[MAX_CITY_LENGTH];
        bzero(selectedCity, MAX_CITY_LENGTH);

        // Read the city as a string
        if (read(tdL.cl, selectedCity, sizeof(selectedCity)) <= 0) {
            perror("[Thread] Error reading selected city from client.\n");
            return;
        }

        printf("selectedCity = %s \n", selectedCity);

        if (!isNumeric(selectedCity) || atoi(selectedCity) > 42 || atoi(selectedCity) < 1) {
            response.success = 0;    
            bzero(response.message, MAX_MESSAGE_LENGTH);
            snprintf(response.message, sizeof(response.message), "Please enter a valid number for the city selected (a number between 1 and 42)");

            if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                perror("[Thread] Error writing response to client.\n");
                return;
            }
        }
        else {
            validCity = 1;

            cityID = atoi(selectedCity);
            printf("cityID = %d \n", cityID);
            // Fetch the city_name for the given cityID
            char getCityNameQuery[100];
            snprintf(getCityNameQuery, sizeof(getCityNameQuery), "SELECT city_name FROM cities WHERE city_id=%d;", cityID);

            bzero(cityName, MAX_CITY_LENGTH);

            db = open_database_connection();
            sqlite3_stmt *cityNameStmt;

            sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);  

            rc = sqlite3_prepare_v2(db, getCityNameQuery, -1, &cityNameStmt, 0);
            if (rc == SQLITE_OK && sqlite3_step(cityNameStmt) == SQLITE_ROW) {
                strncpy(cityName, (const char *)sqlite3_column_text(cityNameStmt, 0), MAX_CITY_LENGTH - 1);
            }

            sqlite3_finalize(cityNameStmt);

            sqlite3_exec(db, "COMMIT", 0, 0, 0);  
            close_database_connection(db);


            printf("cityName = %s \n", cityName);

            if (strlen(cityName) > 0) {
                // Send the city name to the client
                response.success = 1;
                bzero(response.message, MAX_MESSAGE_LENGTH);
                snprintf(response.message, sizeof(response.message), "You selected city with name: %s", cityName);

                if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                    perror("[Thread] Error writing city message to client.\n");
                    return;
                }
            }
            else {
                // Handle the case where the cityID is valid but the city name retrieval failed
                response.success = 0;
                snprintf(response.message, sizeof(response.message), "Error retrieving city information.");

                if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                    perror("[Thread] Error writing response to client.\n");
                    return;
                }
            }

        }
    }
    
    char areaList[MAX_AREA_LIST_LENGTH];
    bzero(areaList, MAX_AREA_LIST_LENGTH);


    char getAreasQuery[100];
    sprintf(getAreasQuery, "SELECT area_name FROM areas WHERE city_id=%d", cityID);

    db = open_database_connection();
    sqlite3_stmt *areasStmt;

    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);  

    rc = sqlite3_prepare_v2(db, getAreasQuery, -1, &areasStmt, 0);
    if (rc != SQLITE_OK) {
        sprintf(message, "Error preparing SQL statement.\n");
        return;
    }

    int id_area = 1;

    while (sqlite3_step(areasStmt) == SQLITE_ROW) {
        char idStr[20];
        sprintf(idStr, "%d", id_area);

        strcat(areaList, idStr);
        strcat(areaList, ". ");
        strcat(areaList, sqlite3_column_text(areasStmt, 0));
        strcat(areaList, "\n");

        id_area++;
    }

    sqlite3_finalize(areasStmt);

    sqlite3_exec(db, "COMMIT", 0, 0, 0);  
    close_database_connection(db);


    // send a list of areas from the city selected to client so that he can choose one to park
    if (write(tdL.cl, areaList, sizeof(areaList)) <= 0) {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread] Error writing to client.\n");
        return;
    }

    int validArea = 0;
    int area_number;
    char areaName[MAX_AREA_LENGTH];

    while (!validArea) {
        char selectedArea[MAX_AREA_LENGTH];
        bzero(selectedArea, MAX_AREA_LENGTH);

        if (read(tdL.cl, selectedArea, sizeof(selectedArea)) <= 0) {
            perror("[Thread] Error reading selected area from client.\n");
            return;
        }

        printf("Selected area: %s \n", selectedArea );

        char getNumberOfAreasQuery[100];
        snprintf(getNumberOfAreasQuery, sizeof(getNumberOfAreasQuery),
                "SELECT COUNT(area_id) FROM areas WHERE city_id=%d;", cityID);

        db = open_database_connection();
        int numberOfAreas = -1;
        sqlite3_stmt *areasCountStmt;

        sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);  

        rc = sqlite3_prepare_v2(db, getNumberOfAreasQuery, -1, &areasCountStmt, 0);
        if (rc == SQLITE_OK && sqlite3_step(areasCountStmt) == SQLITE_ROW) {
            numberOfAreas = sqlite3_column_int(areasCountStmt, 0);
        }

        sqlite3_finalize(areasCountStmt);

        sqlite3_exec(db, "COMMIT", 0, 0, 0); 
        close_database_connection(db);


        if (!isNumeric(selectedArea) || atoi(selectedArea) > numberOfAreas || atoi(selectedArea) < 1) {
            response.success = 0;    
            bzero(response.message, MAX_MESSAGE_LENGTH);
            snprintf(response.message, sizeof(response.message), "Please enter a valid number for the area selected");

            if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                perror("[Thread] Error writing response to client.\n");
                return;
            }
        }
        else {
            validArea = 1;
            area_number = atoi(selectedArea);

            printf("cityID = %d \n", cityID);
            

            db = open_database_connection();
            sqlite3_stmt *areaInfoStmt;

            sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);  

            char getAreaInfoQuery[300];
            snprintf(getAreaInfoQuery, sizeof(getAreaInfoQuery),
                "SELECT area_name FROM areas WHERE city_id=%d ORDER BY area_id LIMIT 1 OFFSET %d;",
                cityID, area_number - 1);

            printf("query: %s \n", getAreaInfoQuery);

            rc = sqlite3_prepare_v2(db, getAreaInfoQuery, -1, &areaInfoStmt, 0);
            if (rc == SQLITE_OK && sqlite3_step(areaInfoStmt) == SQLITE_ROW) {
                strncpy(areaName, (const char *)sqlite3_column_text(areaInfoStmt, 0), MAX_AREA_LENGTH - 1);
                printf("Area Name: %s \n", areaName);
            }

            sqlite3_finalize(areaInfoStmt);

            sqlite3_exec(db, "COMMIT", 0, 0, 0); 
            close_database_connection(db);


            if (strlen(areaName) > 0) {
                // Send the city name to the client
                response.success = 1;
                bzero(response.message, MAX_MESSAGE_LENGTH);
                snprintf(response.message, sizeof(response.message), "You selected area with name: %s", areaName);

                if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                    perror("[Thread] Error writing area message to client.\n");
                    return;
                }
            }
            else {
                response.success = 0;
                snprintf(response.message, sizeof(response.message), "Error retrieving area information.");

                if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                    perror("[Thread] Error writing response to client.\n");
                    return;
                }
            }

        }
    }

    int totalSpots = -1;

    char getTotalSpotsQuery[300];
    snprintf(getTotalSpotsQuery, sizeof(getTotalSpotsQuery),
        "SELECT total_spots FROM areas WHERE city_id=%d ORDER BY area_id LIMIT 1 OFFSET %d;",
        cityID, area_number - 1);

    printf("query: %s \n", getTotalSpotsQuery);
    db = open_database_connection();
    sqlite3_stmt *totalSpotsStmt;
    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);  

    rc = sqlite3_prepare_v2(db, getTotalSpotsQuery, -1, &totalSpotsStmt, 0);
    if (rc == SQLITE_OK && sqlite3_step(totalSpotsStmt) == SQLITE_ROW) {
        totalSpots = sqlite3_column_int(totalSpotsStmt, 0);
        printf("Total spots: %d \n", totalSpots);
    }

    sqlite3_finalize(totalSpotsStmt);

    sqlite3_exec(db, "COMMIT", 0, 0, 0); 
    close_database_connection(db);

    int availableSpots = rand() % (totalSpots + 1);
    printf("available spots: %d \n", availableSpots);
    
    response.success = 1;
    int available = 0;
    if (availableSpots > 0)
    {
        available = 1;
        snprintf(response.message, sizeof(response.message), "Available parking spots in %s: %d", areaName, availableSpots);
    }
    else
         snprintf(response.message, sizeof(response.message), "There are no available parking spots!");
    

    if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread] Error writing to client.\n");
    } else {
        printf("[Thread %d] Message successfully transmitted.\n", tdL.idThread);
    }

    if (available == 1)
    {
        char parking[MAX_PARKING_LENGTH];
        int validStatus = 0;

        while(!validStatus)
        {
            bzero(parking, MAX_PARKING_LENGTH);

            if (read(tdL.cl, parking, sizeof(parking)) <= 0) {
                perror("[Thread] Error reading parking status from client.\n");
                return;
            }

            printf("parking = %s \n", parking);

            if (strlen(parking) == 1 && (strcmp(parking,"y") || strcmp(parking,"Y") || strcmp(parking,"n") || strcmp(parking,"N")) )
            {
                validStatus = 1;
                response.success = 1;

                if (strcmp(parking, "y") == 0 || strcmp(parking, "Y") == 0) {
                    int ok = bookParkingSpace(tdL, areaName, cityName);

                    if(ok == 1) {
                        bzero(response.message, MAX_MESSAGE_LENGTH);
                        snprintf(response.message, sizeof(response.message), "You booked a parking space in city: %s, area: %s \n", cityName, areaName);

                        if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                            perror("[Thread] Error writing response to client.\n");
                            return;
                        }
                    }
                }
                else {
                    bzero(response.message, MAX_MESSAGE_LENGTH);
                    snprintf(response.message, sizeof(response.message), "No parking space reserved");

                    if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                        perror("[Thread] Error writing response to client.\n");
                        return;
                    }
                }
            }
            else
            {
                response.success = 0;    
                bzero(response.message, MAX_MESSAGE_LENGTH);
                snprintf(response.message, sizeof(response.message), "Please enter Y/N");

                if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
                    perror("[Thread] Error writing response to client.\n");
                    return;
                }
            }
        }
    }

}

void getCurrentDateTime(char *dateTime) {
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    strftime(dateTime, MAX_DATE_TIME_LENGTH, "%Y-%m-%d %H:%M:%S", tm_info);
}

int bookParkingSpace(thData tdL, char areaName[MAX_AREA_LENGTH], char cityName[MAX_CITY_LENGTH]) {
    sqlite3 *db;
    close_database_connection(db);
    db = open_database_connection();
    sqlite3_stmt *stmt;

    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

    const char *insertReservationQuery =
        "INSERT INTO reservations (user_id, city_name, area_name, reservation_date) VALUES (?, ?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, insertReservationQuery, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, tdL.userId);
    sqlite3_bind_text(stmt, 2, cityName, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, areaName, -1, SQLITE_STATIC);

    char reservationDate[MAX_DATE_TIME_LENGTH];
    getCurrentDateTime(reservationDate);
    sqlite3_bind_text(stmt, 4, reservationDate, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error executing SQL statement: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
        sqlite3_finalize(stmt);
        close_database_connection(db);
        return 0;
    }

    sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(db);
    printf("Last row id: %lld\n", lastRowId);

    sqlite3_finalize(stmt);

    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    close_database_connection(db);

    return 1;
}


void viewParkingHistory(thData tdL) {
    Response response;
    char query[200];
    snprintf(query, sizeof(query), "SELECT * FROM reservations WHERE user_id=%d;", tdL.userId);

    sqlite3 *db = open_database_connection();
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[server] SQL error: %s\n", sqlite3_errmsg(db));
        return;
    }

    char allReservations[MAX_MESSAGE_LENGTH];
    bzero(allReservations, MAX_MESSAGE_LENGTH);
    strcat(allReservations, "Your parking history is: \n");

    int foundReservations = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        foundReservations = 1;

        int reservationId = sqlite3_column_int(stmt, 0);
        int userId = sqlite3_column_int(stmt, 1);
        const char *cityName = (const char *)sqlite3_column_text(stmt, 2);
        const char *areaName = (const char *)sqlite3_column_text(stmt, 3);
        const char *reservationDate = (const char *)sqlite3_column_text(stmt, 4);

        char reservationInfo[MAX_MESSAGE_LENGTH];
        snprintf(reservationInfo, sizeof(reservationInfo), "Reservation from date: %s in city: %s and area: %s\n", reservationDate, cityName, areaName);
        strcat(allReservations, reservationInfo);
    }

    sqlite3_finalize(stmt);

    bzero(response.message, MAX_MESSAGE_LENGTH);

    if (!foundReservations) {
        snprintf(response.message, sizeof(response.message), "No reservations found!"); 
    }
    else{
        strcat(response.message, allReservations);
        response.message[sizeof(response.message) - 1] = '\0';
    }

    printf("response.message = %s \n", response.message);

    response.success = 1;
    if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
        perror("[Thread] Error writing reservations to client.\n");
    }

    close_database_connection(db);
}