/* Tema 2 Protocoale de comunicatii */
/* Gherasie Stefania - 323CB */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{
	int tcp_sockfd, udp_sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int i, ret, recv_msg, send_msg;
	socklen_t clilen;


	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se obtine numarul portului
	portno = atoi(argv[1]);
	DIE(portno == 0, "Error at obtaining port number");

	// se seteaza struct sockaddr_in pentru a asculta pe portul respectiv
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// se goleste multimea de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se deschid socket-urile pt TCP si UDP
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "Error at opening socket for TCP");

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "Unable to open socket for TCP");

	// se face bind pentru TCP si UDP
	ret = bind(tcp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Error at binding TCP");	

	ret = bind(udp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Error at binding UDP");

	// se face listen pentru TCP 
	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "Error at listening TCP");

	// se adauga file descriptorii in multimea read_fds
    FD_SET(0, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);
    FD_SET(udp_sockfd, &read_fds);
    fdmax = std::max(tcp_sockfd, udp_sockfd);

    // vector cu clientii si lista de topic-uri
    vector<client> clients;
    list<subscription> topics;

    while(1) {
    	tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Error at select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				// s-a primit o comanda de la stdin
				if (i == 0) {
					// se obtine comanda
					memset(buffer, 0 , BUFLEN);
                    fgets(buffer, BUFLEN, stdin);

                    // se primeste "exit"
                    if (!strcmp(buffer, "exit\n")) {
                        exit(0);
                    } else {
                        printf("Usage: (exit)\n");
                	}
                }

				// a venit un mesaj de la socketul de TCP
				else if (i == tcp_sockfd) {
					// serverul primeste o cerere de conexiune si o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "Error at accept");

					// se dezactiveaza protocolul Neagle
					int flag = 0;
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, 
														(char *)&flag, sizeof(int));

					// se adauga noul socket intors de accept() read_fds
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					// se obtine ID-ul clientului
					memset(buffer, 0, 20);
					recv_msg = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(recv_msg < 0, "Error at receiving client ID");

					// se adauga clientul in vectorul de clienti 
					// daca acesta exista deja se activeaza
					add_client(clients, buffer, newsockfd, read_fds);

					// printeaza "New client (CLIENT_ID) connected from IP:PORT."
					printf("New Client <%s> connected from %s:%d.\n", buffer,
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				}

				// a venit un mesaj de la socketul de UDP
				else if (i == udp_sockfd) {
					struct sockaddr_in from_station;
                    socklen_t sock_len = sizeof(from_station);

                    // se primeste un mesaj de la clientul UDP
                    memset(buffer, 0 , BUFLEN);
                    recv_msg = recvfrom(udp_sockfd, buffer, BUFLEN, 0, 
                    					(struct sockaddr *)&from_station, &sock_len);
					DIE(recv_msg <= 0, "Error at receiving message from UDP client");

					// se construieste mesajul de la UDP
					udp_msg msg;
					memset(msg.data, 0, BUFLEN);
					memcpy(msg.data, buffer, BUFLEN);
					memset(msg.ip, 0, 20);
					memcpy(msg.ip, inet_ntoa(from_station.sin_addr), 20);
					msg.port = ntohs(from_station.sin_port);

					// se extrage topicul mesajului
					char topic[51];
					memset(&topic, 0, 50);
					memcpy(&topic, msg.data, 50);

					// se trimite mesajul daca exista clienti abonati la acel topic
					send_topic_message(topics, clients, topic, msg, read_fds);
				}

				// a venit un mesaj de la socketul de TCP
				else {
					// se obtine mesajul de subscribe/unsubscribe
					memset(buffer, 0, BUFLEN);
					tcp_msg msg;
					int recv_msg = recv(i, (char*) &msg, sizeof(tcp_msg), 0);
					DIE(recv_msg < 0, "Error at receiving message from TCP clients");

					// clientul a fost deconectat
					if(recv_msg == 0) {
						// se obtine indexul clientului si se marcheaza ca dezactivat
						int index = get_index_fd(clients, i);
						clients[index].state = false;

						// se inchide conexiunea si se sterge clientul din read_fds
						close(i);
						FD_CLR(i, &read_fds);

						// se printeaza mesajul de deconectare
						printf("Client <%s> disconnected.\n", clients[index].id);
					}

					// se actualizeaza topicurile si clientii
					else if(recv_msg > 0) {
						int index = get_index_id(clients, msg.id_client);

						// mesaj de subscribe
						if(strcmp(msg.command, "subscribe") == 0) {
							// topicul nu exista deja, se adauga si se aboneaza clientul
							if(has_topic(topics, msg.topic, msg.SF) == false){
								add_topic(topics, msg.topic, msg.SF);
								subscribe_client(topics, msg.topic, msg.SF, index);
							}
							// se aboneaza clientul la un topic deja existent
							else {
								subscribe_client(topics, msg.topic, msg.SF,  index);
							}
						}

						// mesaj de unsubscribe
						else if(strcmp(msg.command, "unsubscribe") == 0) {
							unsubscribe_client(topics, msg.topic, index);
						}
					}
				}
			}
		}
	}
    
	// se inchid conexiunile
    close(udp_sockfd);
    close(tcp_sockfd);

    return 0;
}