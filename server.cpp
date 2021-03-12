#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <iterator>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_id serverAdr server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{
	int i, ret;
	char buffer[BUFLEN];
	int sockTCP, sockUDP, sockAUX;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	vector<client> clients;

	fd_set read_fds;
	fd_set tmp_fds;	
	int fdmax;		

	if (argc < 2) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//deschid socket-ul pentru TCP
    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "error -> open TCP socket");

    //deschid socket-ul pentru UDP
    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockUDP < 0, "error -> open UDP socket");

   	DIE(atoi(argv[1]) == 0, "error -> atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "error -> Bind TCP");

    ret = bind(sockUDP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "error -> Bind UDP");

    ret = listen(sockTCP, MAX_CLIENTS);
    DIE(ret < 0, "error -> Listen TCP");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockTCP, &read_fds);
	FD_SET(sockUDP, &read_fds);
	fdmax = sockTCP > sockUDP ? sockTCP : sockUDP;

	while (true) {
		tmp_fds = read_fds; 

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "error -> select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				// cazul in care se primesc date pe socket - ul de listen, TCP
				if (i == sockTCP) {

					// se accepta o noua conexiune
					clilen = sizeof(cli_addr);
					sockAUX = accept(sockTCP, (struct sockaddr *) &cli_addr, &clilen);
					DIE(sockAUX < 0, "error -> accept");

					memset(buffer, 0, BUFLEN);
					ret = recv(sockAUX, buffer, sizeof(buffer),0);
					DIE(ret < 0, "erros -> recive id of client");
					
					// se extrage id - ul clientului
					char id[20];
					strcpy(id, buffer);

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					// si se actualizeaza fdmax daca este cazul
					FD_SET(sockAUX, &read_fds);
					if (sockAUX > fdmax) { 
						fdmax = sockAUX;
					}

					// parcurg vectorul de clienti si daca se gaseste un cleint cu acelasi id se
					// actaulizeaza campul de activitate al acestuia si i se trimit mesajele
					// din history
					int ok = 0;
					for (std::size_t j = 0; j < clients.size(); j++){
						if (strcmp(clients[j].clientID, id) == 0) {
							for (std::size_t k = 0; k < clients[j].history.size(); k++) {
								ret = send(clients[j].fd, (char*)&(clients[j].history[k]), 1600, 0);
								DIE(ret < 0, "send message failed");
							}
							clients[j].active = true;
							ok = 1;
						}
					}

					// in cauzul in care este primul client sau noul client nu a mai fost activ
					// pana acum este adauat direct in vector
					if (clients.size() == 0 || ok == 0) {
						client newClient;
						newClient.fd = sockAUX;
						strcpy(newClient.clientID, id);
						newClient.active  = true;
						clients.push_back(newClient);
					}

					printf("New Client conexion from %s, with port %d and socket %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), sockAUX);

				} else if(i == sockUDP){
					// daca se primesc mesaje de tip UDP se trimit spre filtrare 
					memset(buffer, 0, 1600);
					recvfrom(i, buffer, 1600, 0, (struct sockaddr*) &serv_addr,0);
					sendMessage(clients, buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port)); 
				} else {
					// cazul in care se primesc date de pe unul din socketii de TCP ale clientilor
					memset(buffer, 0, BUFLEN);
					messageForServer msg;
                    
                    ret = recv(i, (char*)&msg, sizeof(msg), 0);
                    DIE(ret < 0, "error -> receive");

					if (ret == 0) {
						// conexiunea s-a inchis
						printf("Client with socket %d disconnected\n", i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						// actualizez starea clientului 
						std::size_t j;
						for(j = 0; j < clients.size(); j++) {
								if(clients[j].fd == i) {
									break;
								}
							}
						clients[j].active = false;
					} else {
						// comada de subscribe sau unsubscribe
						// caut clientul dupa fd
						std::size_t j;
						for(j = 0; j < clients.size(); j++) {
							if(clients[j].fd == i) {
								break;
							}
						}
							
						int ok1 = 0, ok2 = 0;
						if (strcmp(msg.type, "unsubscribe") == 0) {
							if(clients[j].topicNames.size() != 0) {
								for (std::size_t k = 0; k < clients[j].topicNames.size(); k++) {
									//daca topicNameul exista
									if (strcmp(clients[j].topicNames[k].name, msg.name) == 0) {
										// se cauta topicul respectiv si este scos din vector
										vector<topic>::iterator it = clients[j].topicNames.begin();
										while(strcmp(it->name, msg.name) != 0) {
											it++;
										}
										clients[j].topicNames.erase(it);

										ok1 = 1;
										break;
									}
								}
								if (ok1 == 0) {
									printf("Client is not subscribed to %s\n", msg.type);
								}
							}
						} else {
							// topicul ce trebuie adaugat
							topic aux;
							strcpy(aux.name, msg.name);
							aux.SF = msg.SF;

							if(clients[j].topicNames.size() == 0) {
								clients[j].topicNames.push_back(aux);
							}else {
								for (std::size_t k = 0; k < clients[j].topicNames.size(); k++) {
									// daca topicul exista deja verific daca trebuie actualizat SF 
									if(strcmp(clients[j].topicNames[k].name, msg.name) == 0) {
										if(clients[j].topicNames[k].SF == msg.SF) {
											printf("Client is already subscribed to %s\n", msg.name);
											ok2 = 1;
											break;
										} else {
											clients[j].topicNames[k].SF = msg.SF;
											printf("Client change SF topic %s\n", msg.name);
											ok2 = 1;
										}
									}
								}
								// daca nu a fost gasit se adauga
								if(ok2 == 0) {
									clients[j].topicNames.push_back(aux);
								}
							}
						}
						if ((ok1 == 1) || (ok2 == 0)) {
							printf("Client %s %s to %s\n", clients[j].clientID, msg.type, msg.name);
						}
					}
				}
			}
		}
	}

	close(sockTCP);

	return 0;
}
