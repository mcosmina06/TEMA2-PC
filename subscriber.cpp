#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
    fprintf(stderr, "Usage: %s client_id serverAdr server_port\n", file);
    exit(0);
}

int main(int argc, char** argv) {
    
    if (argc < 4) {
        usage(argv[0]);
    }

    if (strlen(argv[1]) > 10) {
        cout << "error -> client id is too long" << '\n';
        return -1;
    }

    char buffer[STRLEN_RECV];
    int sockfd = 0, ret;
    int flag = 1;
    messageForServer messageToSend;
    message* messageReceived;
    sockaddr_in serverAdr;
    fd_set fds, tmp_fds;

    // deschidem socket - ul pentru a ne putem conecta la server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "error -> open server socket");

    // completam campurile pentru a fi posibila conexiunea la server
    serverAdr.sin_family = AF_INET;
    serverAdr.sin_port = htons(atoi(argv[3]));

    ret = inet_aton(argv[2], &serverAdr.sin_addr);
    DIE(ret == 0, "error -> incorrect ip server");

    ret = connect(sockfd, (sockaddr*) &serverAdr, sizeof(serverAdr));
    DIE(ret < 0, "error -> connection to server failed");

    ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "error -> unreacheble server");

    // dezactivez algoritmul
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // seteazdescriptorii socketilor
    FD_ZERO(&fds);
    
    FD_SET(sockfd, &fds);
    FD_SET(0, &fds);

    while (true) {
        tmp_fds = fds;

        ret = select(sockfd + 1, &tmp_fds, nullptr, nullptr, nullptr);
        DIE(ret < 0, "error -> select");

        if (FD_ISSET(0, &tmp_fds)) {
            // s-a primit un mesaj de la stdin
            memset(buffer, 0, STRLEN_RECV);
            fgets(buffer, BUFLEN - 1, stdin);
            if (!strcmp(buffer, "exit\n")) {
                break;
            }

            // se verifica mesajul
            if (check_message(messageToSend, buffer)) {
                ret = send(sockfd, (char*) &messageToSend, sizeof(messageToSend), 0);
                DIE(ret < 0, "error -> action to send message failed");
                // se afiseaza mesajele pentru comanda data
                if (strcmp(messageToSend.type, "unsubscribe") == 0) {
                    printf("Client unsubscribed %s\n", messageToSend.name);
                } else {
                    printf("Client subscribed %s\n", messageToSend.name);
                }
            }
        }

        //cazul in care se primeste mesaj de la server
        else if (FD_ISSET(sockfd, &tmp_fds)) {
            memset(buffer, 0, BUFLEN);

            ret = recv(sockfd, buffer, sizeof(message), 0);
            DIE(ret < 0, "error -> recv failed");

            if (ret == 0) {
                // serverul inchide conexiunea
                break;
            } else {
                // afisarea mesajelor
                messageReceived = (message*)buffer;
                if (strcmp(messageReceived->type, "INT") == 0){
                    printf("%s: %d - %s - %s - %d\n", messageReceived->ip, messageReceived->udp_socket, messageReceived->name, 
                        messageReceived->type, messageReceived->case0);
                } else if (strcmp(messageReceived->type, "FLOAT") == 0) {
                    printf("%s: %d - %s - %s - %.8g\n", messageReceived->ip, messageReceived->udp_socket, messageReceived->name, 
                        messageReceived->type, messageReceived->case2);
                } else if (strcmp(messageReceived->type, "SHORT_REAL") == 0){
                    printf("%s: %d - %s - %s - %.2f\n", messageReceived->ip, messageReceived->udp_socket, messageReceived->name, 
                        messageReceived->type, messageReceived->case1);  
                }
                else if (strcmp(messageReceived->type, "STRING") == 0){
                    printf("%s: %d - %s - %s - %s\n", messageReceived->ip, messageReceived->udp_socket, messageReceived->name, 
                        messageReceived->type, messageReceived->data);
                }
            }
        }
    }

    close(sockfd);

    return  0;
}
