#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <cmath>

using namespace std;

#define STRLEN_RECV (sizeof(message) + 1)
#define BUFLEN 1552
#define MAX_CLIENTS 100

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

#endif

// strutura pentru mesajele de tip UDP
struct message {
    int udp_socket;
    char type[11];
    char name[51];
    char ip[16];
    //variabila in cazul in care este un int
    int case0;
    // variabila folosita in cazul in care este un SHORT_INT
    float case1;
    // variabila folosita in cauzl in care este un FLOAT
    float case2;
    // variabila folosita in cazul in care este un STRING
    char data[1501];
};

// structura pentru mesajul pentru server
struct messageForServer {
    char type[15];
    int SF;
    char name[51];
};

// structura ce retine numele topicului si SF
struct topic {
	char name[51];
	int SF;
};

// structura pentru client
struct client {
	bool active;
	char clientID[20];
	int fd;
	// vector de contine topicurile la care este abonat
	vector <topic> topicNames;
	// vector ce retine mesajele pe perioada de inactivitate
	vector <message> history;
};

// functia ce primeste un server si face verificarile necesare pentru 
// combaterea erorilor
bool check_message(messageForServer &message, char* buffer) {
   
    buffer[strlen(buffer) - 1] = '\0';
    char *tok = strtok(buffer, " ");

    if (tok == nullptr) {
        cout << "Invalid command!" << '\n';
        return false;
    }

    if (strcmp(tok, "subscribe") == 0) {
        strcpy(message.type, "subscribe");
    } else if (strcmp(tok, "unsubscribe") == 0) {
        strcpy(message.type, "unsubscribe");
    } else {
        cout << "Invalid command!" << '\n';
        return false;
    }

    tok = strtok(nullptr, " ");

    if (tok == nullptr) {
        cout << "Invalid command!" << '\n';
        return false;
    }
    if (strlen(tok) > 50) {
        cout << "Name for parameter is too long!" << '\n';
        return false;
    }

    strcpy(message.name, tok);

    //daca este o comanda de tip subscribe se analizeaza si cel de al treilea parametru
    if (strcmp (message.type, "subscribe") == 0) {
        tok = strtok(nullptr, " ");

        if (tok == nullptr) {
            cout << "Invalid command!" << '\n';
            return false;
        }

        if (tok[0] != '0' && tok[0] != '1') {
            cout << "Invalid SF!" << '\n';
            return false;
        } 

        // se seteaza SF
        if(tok[0] == '0') {
        	message.SF = 0;
        } else {
        	message.SF = 1;
        }
    }

    return true;
}

// functie ce filtreaza si trimite mesajul la clientii abonati
void sendMessage(vector<client> &clients, char buffer[], char ip[], int socket){
	char topicName[51];
	int type = buffer[50];
	
	// mesajul ce va fi construit
	message toSend;

	//extrag numele topicului  din buffer
	memset(topicName, 0, 51);
	memcpy(topicName, buffer, 50);
	strcpy(toSend.ip, ip);
	strcpy(toSend.name, topicName);
	toSend.udp_socket = socket;

	switch(type) {
		case 0: {
			int sign = buffer[51];
			uint32_t data;
			memset(&data, 0, sizeof(uint32_t));
			memcpy(&data, buffer + 52, sizeof(uint32_t));
			toSend.case0 = ntohl(data);
			if(sign == 1) {
				toSend.case0 = toSend.case0 * (-1);
			}
			strcpy(toSend.type, "INT");
			break;
		}
		case 1: {
			uint16_t data1;
			memset(&data1, 0, sizeof(uint16_t));
			memcpy(&data1, buffer + 51, sizeof(uint16_t));
			toSend.case0 = (float)ntohl(data1) / 100;
			strcpy(toSend.type, "SHORT_REAL");
			break;
		}
		case 2: {
			int sign = buffer[51];
			
			uint32_t data2; 
			memset(&data2, 0, sizeof(uint32_t));
			memcpy(&data2, buffer + 52, sizeof(uint32_t));
			
			int nb = ntohl(data2);
			
			uint8_t exp;
			memset(&exp, 0, sizeof(uint8_t));
			memcpy(&exp, buffer + 52 + sizeof(uint32_t), sizeof(uint8_t));
			
			toSend.case2 = nb/(pow(10,exp));
			if(sign == 1) 
				toSend.case2 = toSend.case2 * (-1);
			strcpy(toSend.type, "FLOAT");
			break;
		}
		case 3: {
			memset(toSend.data, 0 , 1500);
			memcpy(toSend.data, buffer + 51, 1501);
			strcpy(toSend.type, "STRING");
			break;
		}
		default: break;
	}

	// se parcurge vectorul de clienti si se cauta cei care sunt abonati la
	// topicul respectiv; daca sunt inactivi se pastreaza mesajele in history
	for(std::size_t i = 0; i < clients.size(); i++) {
		for(std::size_t j = 0; j < clients[i].topicNames.size(); j++) {
			if(strcmp(clients[i].topicNames[j].name, topicName) == 0) {
				if(clients[i].active) {
					int ret = send(clients[i].fd, (char*)&toSend, 1600, 0);
					DIE(ret < 0, "send message failed");
					break;
				} else if((!clients[i].active) && (clients[i].topicNames[j].SF == 1)) {
					clients[i].history.push_back(toSend);
				}
			}
		}
	}

}
