#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

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

#include "response.h"
#include "constants.h"
#include "thread_data.h"


void register_user(thData tdL);
int login_user(thData *tdL);
int verify_username(char username[MAX_USERNAME_LENGTH],  sqlite3 *db);
void viewParkingAvailability(thData tdL);
int bookParkingSpace(thData tdL, char areaName[MAX_AREA_LENGTH], char cityName[MAX_CITY_LENGTH]);
void viewParkingHistory(thData tdL);

#endif