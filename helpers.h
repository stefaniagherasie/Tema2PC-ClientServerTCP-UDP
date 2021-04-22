/* Tema 2 Protocoale de comunicatii */
/* Gherasie Stefania - 323CB */

#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <cmath>
#include <vector>
#include <list>
#include <algorithm>
#include <iterator>

using namespace std;

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1560	// dimensiunea maxima a calupului de date
#define MESSAGE_LEN 1560
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare


typedef struct {
	char data[MESSAGE_LEN];
	char ip[20];
	int port;
} udp_msg;

typedef struct {
	char command[15];
	char topic[51];
	int SF;
	char id_client[10];
} tcp_msg;

typedef struct {
	char topic_name[51];
	int SF;
	list<int> subscribers;
} subscription;

typedef struct {
	char id[20];
	int fd;
	bool state;
	list<udp_msg> message_to_send;
} client;

typedef struct {
	char name[50];
	int SF;
} topic;

/*
	----------------------------
		FUNCTII PENTRU SERVER 
	----------------------------
*/


// intoarce indexul clientului cu id-ul dat
int get_index_id(vector<client> clients, char* id_client) {
	for(int i = 0; i < clients.size(); i++) {
		if(strcmp(clients[i].id, id_client) == 0)
			return i;
	}
	return -1;
}

// intoarce indexul clientului cu file descriptorul dat
int get_index_fd(vector<client> clients, int fd) {
	for(int i = 0; i < clients.size(); i++) {
		if(clients[i].fd == fd)
			return i;
	}
	return -1;
}

// verifica daca exista topicul dat
bool has_topic(list<subscription> topics, char* topic, int SF) {
	list <subscription> :: iterator it;
	for(it = topics.begin(); it != topics.end(); it++) {
		if(strcmp(it->topic_name, topic) == 0 && it->SF == SF) 
			return true;
	}	
	return false;
}

// verifica daca lista data contine clientul cu acel index
// indexul corescunde pozitiei din vectorul clients
bool has_client(list<int> subscribers, int index) {
	if(find(subscribers.begin(), subscribers.end(), index) != subscribers.end()) {
		return true;
	}
	return false;
}

// adauga un topic nou in lista
void add_topic(list<subscription> &topics, char* topic, int SF) {
	subscription new_sub;
	memcpy(new_sub.topic_name, topic, 50);
	new_sub.SF = SF;

	topics.push_back(new_sub);
}

// intoarce topicul corespunzator din lista
subscription get_topic(list<subscription> topics, char* topic, int SF) {
	list <subscription> :: iterator it;
	for(it = topics.begin(); it != topics.end(); it++) {
		if(strcmp(it->topic_name, topic) == 0 && it->SF == SF) {
			return *it;
		}
	}
}

// aboneaza un client la topicul dat
void subscribe_client(list<subscription> &topics, char* topic, int SF, int index) {
	list <subscription> :: iterator it;

	for(it = topics.begin(); it != topics.end(); it++) {
		if(strcmp(it->topic_name, topic) == 0 && it->SF == SF) {
			// adauga clientul la lista de abonati a topicului
			it->subscribers.push_back(index);
		}
	}
}

// dezaboneaza clientul de la topic
void unsubscribe_client(list<subscription> & topics, char* topic, int index) {
	list <subscription> :: iterator it;

	for(it = topics.begin(); it != topics.end(); it++) {
		if(strcmp(it->topic_name, topic) == 0) {
			// sterge clientul
			it->subscribers.remove(index);
		}
	}
}

// se trimit mesajele stocate la client
void send_stored_messages(client cl, fd_set read_fds) {
	int send_msg;
	if(FD_ISSET(cl.fd, &read_fds) == true) {
		list <udp_msg> :: iterator it;

		// se trimit toate mesajele stocate la client
		for(it = cl.message_to_send.begin(); it != cl.message_to_send.end(); it++) {
			send_msg = send(cl.fd, (char*) &(*it), sizeof(udp_msg), 0);
			DIE(send_msg < 0, "Error at sending message to TCP clients");
		}
	}
}

// trimite mesajele primite de la clientul UDP la clientii TCP abonati la topic
void send_topic_message(list<subscription> topics, vector<client> &clients, 
									char* topic, udp_msg msg, fd_set read_fds) {

	int send_msg;
	// se obtin datele pt SF = 0	
	if(has_topic(topics, topic, 0)) {
		subscription sub = get_topic(topics, topic, 0);
		list <int> :: iterator it;

		// se trimite mesajul, iar daca clientul e inactiv acesta se pierde
		for(it = sub.subscribers.begin(); it != sub.subscribers.end(); it++) {	
			if(FD_ISSET(clients[*it].fd, &read_fds)) {
				send_msg = send(clients[*it].fd, (char*) &msg, sizeof(udp_msg), 0);
				DIE(send_msg < 0, "Error at sending message to TCP clients");
			}
		}
	}

	// se obtin datele pt SF = 1
	if(has_topic(topics, topic, 1)) { 
		subscription sub = get_topic(topics, topic, 1);
		list <int> :: iterator it;

		for(it = sub.subscribers.begin(); it != sub.subscribers.end(); it++) {
			// daca clientul este activ se trimite mesajul
			if(clients[*it].state == true) {
				if(FD_ISSET(clients[*it].fd, &read_fds)) {
					// se trimite mesajul
					send_msg = send(clients[*it].fd, (char*) &msg, sizeof(udp_msg), 0);
					DIE(send_msg < 0, "Error at sending message to TCP clients");
				}
			}

			// clientul este inactiv si se stocheaza mesajele
			else if(clients[*it].state == false) {
				clients[*it].message_to_send.push_back(msg);
			}
		}
	}
}

// se adauga un client la vectorul clients
void add_client(vector<client> &clients, char* id_client, int fd, fd_set read_fds) {
	int index = get_index_id(clients, id_client);

	// clientul este nou
	if(index == -1) {
		// se creeaza clientul 
		client cl;
		memset(cl.id, 0, 20);
		memcpy(cl.id, id_client, 20);
		cl.fd = fd;
		cl.state = true;
		
		// se adauga clientul la lista de clienti
		clients.push_back(cl);
	}
	// clientul exista deja si s-a reactivat
	else {
		if(clients[index].state == false) {
			// se seteaza ca activat si se trimit mesajele stocate pana acum
			clients[index].state = true;
			send_stored_messages(clients[index], read_fds);
		}
	}
}


/*
	---------------------------------
		FUNCTII PENTRU SUBSCRIBER
	---------------------------------
*/


// printeaza mesajul primit de server in formatul corespunzator
void print_msg_from_server(udp_msg msg) {

	// se obtine topicul
	char topic[51];
	memset(&topic, 0, 50);
	memcpy(&topic, msg.data, 50);

	// se obtine tipul de mesaj
	uint8_t type = (uint32_t) *(msg.data + 50);

	// se afiseaza inceputul mesajului
	printf ("%s:%d - {%s} - ", msg.ip, msg.port, topic);

	// INT
	if (type == 0) {
		int sign = msg.data[51];
		if(sign == 1) {
			sign = -1;
		}
		else sign = 1;

		uint32_t value;
		memset(&value, 0, sizeof(uint32_t));
		memcpy(&value, msg.data + 52, sizeof(uint32_t));
		
		printf("INT - {%d}\n", sign * ntohl(value));
	}

	// SHORT REAL
	else if (type == 1) {
		uint16_t value;
		memset(&value, 0, sizeof(uint16_t));
		memcpy(&value, msg.data + 51, sizeof(uint16_t));

		printf("SHORT_REAL - {%.2f}\n", (1.0 * ntohs(value)) / 100);
	}

	// FLOAT
	else if (type == 2) {
		int sign = msg.data[51];
		if (sign == 1) {
			sign = -1;
		}
		else sign = 1;

		uint32_t mod_value;
		memset(&mod_value, 0, sizeof(uint32_t));
		memcpy(&mod_value, msg.data + 52, sizeof(uint32_t));

		uint8_t exp_value;
		memset(&exp_value, 0, sizeof(uint8_t));
		memcpy(&exp_value, msg.data + 52 + sizeof(uint32_t), sizeof(uint8_t));

		float float_value = ntohl(mod_value);

		while(exp_value) {
			float_value /= 10;
			exp_value--;
		}

		printf("FLOAT - {%.4f}\n", sign *float_value);
	}

	// STRING
	else if (type == 3) {
		printf("STRING - {%s}\n", msg.data + 51);
	}
}

// verifica daca topicul exista deja si il adauga in caz contrar
bool add_topic(topic new_topic, list<topic> &topics) {
	list <topic> :: iterator it;

	for(it = topics.begin(); it != topics.end(); it++) {
		// am gasit topicul si dam mesaj de comanda invalida
		if(strcmp(new_topic.name, it->name) == 0) {
			if(new_topic.SF == it->SF) {
				printf("Client is already subscribed to topic <%s>.\n", new_topic.name);
				return false;
			}
			else if(new_topic.SF != it->SF) {
				printf("Client is already subscribed to topic <%s> with other SF.\n", 
																new_topic.name);
				return false;
			}
		}
	}

	// adaugam topicul in lista
	topics.push_back(new_topic);
	return true;
}


// verifica daca topicul exista si il sterge in caz afirmativ
bool delete_topic(topic old_topic, list<topic> &topics) {
	list <topic> :: iterator it;

	for(it = topics.begin(); it != topics.end(); it++) {
		// am gasit topicul si il stergem
		if(strcmp(it->name, old_topic.name) == 0){
			topics.erase(it);
			return true;
		}
	}

	// topicul nu exista si se afiseaza mesaj de comanda invalida
	printf("Client is not subscribed to topic <%s>\n", old_topic.name);
	return false;

}

// se construieste un mesaj de abonare/dezabonare si se trimite la server
void send_message_to_server(int sockfd, char* id_client, char* buffer, list<topic> &topics) {

	// se creeaza un mesaj de abonare
	tcp_msg msg;

	// setam ID-ul clientului de la care provine mesajul
	memset(&msg.id_client, 0, 10);
	memcpy(&msg.id_client, id_client, strlen(id_client));

	// obtinere comanda (subscribe sau unsubscribe)
	char *token = strtok(buffer, " \n"); 
	if(token == nullptr) {
		printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
		return;
	}
	// setare comanda
	memset(&msg.command, 0, 15);
	memcpy(&msg.command, token, strlen(token));


	// obtinere topic
	token = strtok(nullptr, " \n"); 
	if(token == nullptr) {
		printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
		return;
	}
	if(strlen(token) > 50) {
		printf("Usage: The second parameter is too long - maximum 50 characters.\n");
		return;
	}
	//setare topic
	memset(&msg.topic, 0, 50);
	memcpy(&msg.topic, token, strlen(token));


	// comanda de subscribe
	if (strcmp(msg.command, "subscribe") == 0) {
		// obtinere SF
		token = strtok(nullptr, " \n");
		if(token == nullptr) {
			printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
			return;
		}
		// setare SF
		msg.SF = atoi(token);
		if(msg.SF != 1 && msg.SF != 0) {
			printf("SF can only be 0/1.\n");
			return;
		}

		token = strtok(nullptr, " \n");
		if(token != nullptr) {
			printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
			return;
		}

		// se testeaza daca comanda e valida
		topic new_topic;
		memset(new_topic.name, 0, 50);
		memcpy(new_topic.name, msg.topic, 50);
		new_topic.SF = msg.SF;
		if(add_topic(new_topic, topics) == false) {
			return;
		}


		// trimitere mesaj la server 
		int send_msg = send(sockfd, (char*) &msg, sizeof(tcp_msg), 0);
		DIE(send_msg < 0, "Error at sending message");

		// printare mesaj de confirmare
		printf("Client subscribed to topic <%s>\n", msg.topic);

	}

	// comanda de unsubscribe
	else if (strcmp(msg.command, "unsubscribe") == 0) {
		token = strtok(nullptr, " \n");
		if(token != nullptr) {
			printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
			return;
		}

		// setam SF la -1 deoarece pentru aceasta comanda nu ne trebuie SF-ul
		msg.SF = -1;

		// se testeaza daca comanda e valida
		topic old_topic;
		memset(old_topic.name, 0, 50);
		memcpy(old_topic.name, msg.topic, 50);
		old_topic.SF = msg.SF;
		if(delete_topic(old_topic, topics) == false) {
			return;
		}


		// trimitere mesaj la server 		
		int send_msg = send(sockfd, (char*) &msg, sizeof(tcp_msg), 0);
		DIE(send_msg < 0, "Error at sending message");

		// printare mesaj de confirmare
		printf("Client unsubscribed from topic <%s>\n", msg.topic);
	}

	// comanda gresita
	else {
		printf("Usage: (subscribe <topic> <SF>) | (unsubscribe <topic>) | (exit)\n");
	}
}



#endif
