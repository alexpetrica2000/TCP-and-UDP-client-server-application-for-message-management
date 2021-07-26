#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <math.h>
#include "helpers.h"
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

#define BUFLEN		1600	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	3200	// numarul maxim de clienti in asteptare


// protocol aplicatie peste tcp
typedef struct {
	int dim;  // dimensiunea in octeti a mesajului
	char buffer[BUFLEN]; // mesajul propriu-zis
	char id[10]; // id-ul senderului (clientului)
} proto_tcp_msg;


// Lista de clienti ce retin vector de siruri de caractere
// in acestea vor fi stocate mesajele de la topicurile la care sunt abonati cu sf = 1
// pe timpul deconectarii acestora
typedef struct ListSFNode{
	char *id;   // id client
	int list_topic_size;   // dimensiunea vectorului
	char topics[100][BUFLEN];  // sirurile de caractere
	struct ListSFNode *next; // nodul urmator din lista
} ListSFNode;

// structura nodului folosit de lista
typedef struct node{
    char *data;   // id
    int sock;   // socket pentru lista de useri si sf pentru lista de topicuri
    struct node *next;  // nodul urmator
} ListNode;

typedef struct List{
	char *topic;    // numele topicului
    ListNode *head;  // primul nod din lista
    ListNode *tail; // ultimul nod
} List;

void insertSFList(ListSFNode **root, char *id);

void updateSFList(ListSFNode ** root, char *id, char *newtopic);

ListSFNode *searchSF(ListSFNode ** root, char *id);

List *createList(char *topic);

ListNode *createListNode(char* data, int sock);

void destroyListNode(ListNode *node);

void destroyListNodes(List *node);

void destroyList(List *list);

int isEmpty(List *list);

void insert(List *list, char *data, int sock);

void delete(List *list, char *data);

ListNode *search(List *list, char *data);

void printList(List *list);



#endif
