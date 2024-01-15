#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>  
#include <strings.h> 

#include "client_functions.h"
#include "response.h"
#include "constants.h"

void register_user(int sd) {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];

    bzero(username, MAX_USERNAME_LENGTH);
    bzero(password, MAX_PASSWORD_LENGTH);

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(response));

    if (response.success) {
        printf("[client] Registration successful\n");
    } else {
        printf("[client] Registration failed: %s\n", response.message);
    }
}

int login_user(int sd) {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];

    bzero(username, MAX_USERNAME_LENGTH);
    bzero(password, MAX_PASSWORD_LENGTH);

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(response));

    if (response.success) {
        printf("[client] Login successful\n");
        return 1;
    } else {
        printf("[client] Login failed: %s\n", response.message);
        return 0;
    }
}

void view_parking_availability(int sd){
    Response response;
    response.success = 0;

    char city[MAX_CITY_LENGTH];
    char area[MAX_AREA_LENGTH];
    char parking[MAX_PARKING_LENGTH];

    char cityList[MAX_CITY_LIST_LENGTH];
    bzero(cityList, MAX_CITY_LIST_LENGTH);
    
    if (read(sd, cityList, sizeof(cityList)) <= 0) {
        perror("[client] Error reading cities from server.\n");
        return;
    }

    printf("Choose a city from this list where you want to park (Please enter a number between 1 and 42 - corresponding to the city you choose eg. Enter 1 for Alba): \n %s \n", cityList);

    int validCity = 0;

    while (!validCity) {
        printf("Enter the number of the city where you want to park: ");
        fflush (stdout);
        bzero(city, MAX_CITY_LENGTH);
        scanf("%s", city);

        // printf("city selected = %s \n", city);

        if (write(sd, city, sizeof(city)) <= 0) {
            perror("[client] Error writing city to client.\n");
            return;
        }

        if (read(sd, &response, sizeof(Response)) <= 0) {
            perror("[client] Error reading city name from server.\n");
            return;
            }

        if (response.success) {
            validCity = 1;
        } else {
            printf("Error: %s\n", response.message);
        }

        printf("%s \n",response.message);
    }

    char areaList[MAX_AREA_LIST_LENGTH];
    bzero(areaList, MAX_AREA_LIST_LENGTH);

    if (read(sd, areaList, sizeof(areaList)) <= 0) {
        perror("[client] Error reading areas from server.\n");
        return;
    }

    printf("Choose an area in from this list where you want to park (Please enter the number representing the area selected): \n %s \n", areaList);

    int validArea = 0;

    while (!validArea) {
        printf("Enter the number of area where you want to park: ");
        fflush (stdout);
        bzero(area, MAX_AREA_LENGTH);
        scanf("%s", area);

        // printf("area selected = %s \n", area);

        if (write(sd, area, sizeof(area)) <= 0) {
            perror("[client] Error writing area to client.\n");
            return;
        }

        if (read(sd, &response, sizeof(Response)) <= 0) {
            perror("[client] Error reading area name from server.\n");
            return;
            }

        if (response.success) {
            validArea = 1;
        } else {
            printf("[client] Error: %s\n", response.message);
        }

        printf("%s \n",response.message);
    }

    if (read(sd, &response, sizeof(Response)) <= 0) {
        perror("[client] Error reading total spots from server.\n");
        return;
        }

    printf("aici %s\n", response.message);

    if (strcmp(response.message, "There are no available parking spots!") != 0)
    {
        int validStatus = 0;

        while(!validStatus)
        {
            printf("Do you want to reserve a parking space in this area? (Y/N) \n");
            fflush (stdout);
            bzero(parking, MAX_PARKING_LENGTH);
            scanf("%s", parking);
            
            if (write(sd, parking, sizeof(parking)) <= 0) {
                perror("[client] Error writing parking status to client.\n");
                return;
            }

            if (read(sd, &response, sizeof(Response)) <= 0) {
                perror("[client] Error reading area name from server.\n");
                return;
            }

            if (response.success) {
                validStatus = 1;
            } else {
                printf("[client] Error: %s\n", response.message);
            }

            printf("%s \n",response.message);
        }
    }


}

void view_parking_history(int sd){
    Response response;
    bzero(response.message, MAX_MESSAGE_LENGTH);

    if (read(sd, &response, sizeof(Response)) <= 0) {
        perror("[client] Error reading response from server.\n");
        return;
    }

    if (response.success) {
        printf("%s \n", response.message);
    } else {
        printf("[client] Error: %s\n", response.message);
    }
    
}