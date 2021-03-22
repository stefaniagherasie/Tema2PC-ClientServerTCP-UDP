/* Tema 2 Protocoale de comunicatii */
/* Gherasie Stefania - 323CB */

#include <stdio.h>
#include <string>
#include <cmath>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_ID client_address client_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{
	int sockfd, i, ret, portno;
	int send_msg, recv_msg;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 4) {
		usage(argv[0]);
	}

	// se obtine numarul portului
	portno = atoi(argv[3]);
	DIE(portno == 0, "Error at obtaining port number");

	// se obtine ID-ul clientului si se trimite la server
    char id_client[10];
	strcpy(id_client, argv[1]);
	DIE(strlen(id_client) > 10, "Error: CLIENT_ID can have maximum 10 characters.");

	// se seteaza struct sockaddr_in pentru a asculta pe portul respectiv
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Error at inet_aton");

	// se goleste multimea de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se deschide socket-ul pt	client
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Error at opening subscriber socket");

	// se conecteaza clientul
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Error at connecting client");

	// se adauga noii file descriptori in multimea read_fds
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);

    // se trimite id-ul la server
	send_msg = send(sockfd, id_client, strlen(id_client), 0);
	DIE(send_msg < 0, "Error at sending ID to server");

	// lista de topicuri la care e abonat si SF-urile lor
	list<topic> topics;


	while (1) {
		tmp_fds = read_fds;

		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Error at select");

		// se primeste un mesaj de la server
		if (FD_ISSET(sockfd, &tmp_fds)) {
			udp_msg msg;
			int recv_msg = recv(sockfd, (char*) &msg, sizeof(udp_msg), 0);
			DIE(recv_msg < 0, "Error at receiving message from server");

			// serverul s-a deconectat, se opreste si clientul
			if(recv_msg == 0) {
				exit(0);
			}
			// se printeaza mesajul primit
			print_msg_from_server(msg);
		}

		// se citeste de la tastatura o comanda
		else if(FD_ISSET(0, &tmp_fds)){			
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);

			// clientul se inchide
			if(strncmp(buffer, "exit", 4) == 0) {
				exit(0);
			}
			// se trimite mesaj de subscribe/unsubscribe
			else {
				send_message_to_server(sockfd, id_client, buffer, topics);
			}
		}
	}

	// se inchide conexiunea
	close(sockfd);

	return 0;
}