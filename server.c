#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	int tcp_sockfd, udp_sockfd, newsockfd, portno;
	struct sockaddr_in serv_addr_tcp, serv_addr_udp, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	// lista de utilizatori conectati si de topicuri ( lista de liste )
	// unde fiecare head contine numele topicului si nodurile sunt clientii
	List *user_list = createList("");
	List **topic_list = malloc(1000 * sizeof(struct List*));
	int list_size = 0; // dimensiunea listei  de topicuri

	// Lista folosita de utilizatorii ce au sf 1 la anumite topicuri
	// practic o lista de useri in care am buffere ce retin stringuri
	// pe care le voi transmite cand acestia se conecteaza
	ListSFNode **sf_list = malloc(sizeof(struct ListSFNode*));
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "Probleme socket TCP");

	// dezactivare algoritm Nagle
	int flag = -1;
    int result = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag));
    DIE (result < 0, "Eroare la setare tcp no delay");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr_tcp, 0, sizeof(serv_addr_tcp));
	serv_addr_tcp.sin_family = AF_INET;
	serv_addr_tcp.sin_port = htons(portno);
	serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	serv_addr_udp.sin_family = AF_INET;
	serv_addr_udp.sin_port = htons(portno);
	serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

	int b = bind(udp_sockfd, (struct sockaddr*) &serv_addr_udp, sizeof(serv_addr_udp));
	DIE(b < 0, "bind UDP");

	ret = bind(tcp_sockfd, (struct sockaddr *) &serv_addr_tcp, sizeof(serv_addr_tcp));
	DIE(ret < 0, "bind TCP");

	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se aadauga cei 3 socketi in multime si se alege maximul
	FD_SET(0, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	fdmax = tcp_sockfd > udp_sockfd ? tcp_sockfd : udp_sockfd;

	// structura folosita ca protocol aplicatie peste TCP
	proto_tcp_msg message;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcp_sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// Dezactivare algoritm Nagle
					flag = 1;
   					result = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag));
    				DIE (result < 0, "Eroare la setare tcp no delay");

					n = recv(newsockfd, &message, sizeof(proto_tcp_msg), 0);
					DIE(n < 0, "Eroare la recv tcp");
					
					// se cauta in lista de useri dupa id daca id-ul este folosit sau nu
					// in cazul in care acesta este deja folosit de alt client 
					// se trimite un mesaj de exit clientului ce incearca conectarea
					if (search(user_list,message.id) == NULL)
						insert(user_list, message.id, newsockfd);
					else {
						memset(message.buffer, 0, BUFLEN);
						strcpy(message.buffer, "exit\n");
						message.dim = strlen(message.buffer) + 1;
						n = send(newsockfd, &message, sizeof(int) + message.dim, 0);
						DIE(n < 0, "Eroare la send tcp");
						printf("Client %s already connected.\n", message.id);
						break;
					}
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire si
					// se stabileste maximul
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					
					printf("New client %s connected from %s:%d.\n", message.id,
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

					// daca clientul conectat are topicuri la care este abonat cu sf 1
					// se trimit mesajele stocate de lista catre acesta
					ListSFNode *p = searchSF(sf_list, message.id);
					if (p != NULL && p->list_topic_size != 0) {
						for (int counter = 0; counter < p->list_topic_size; counter++) {
							proto_tcp_msg m;
							strcpy(m.id, p->id);
							strcpy(m.buffer, p->topics[counter]);
							m.dim = strlen(p->topics[counter]) + 1;
							n = send(newsockfd, &m, sizeof(int) + m.dim, 0);
							DIE(n < 0, "Eroare la send tcp");
						}
						p->list_topic_size = 0;
					}
				} else if (i == udp_sockfd) {

					// daca socketul este udp stim formatul mesajului ce va fi primit
					char topic[51];
					char tip_date;
					char continut[1500];
					char buffer_aux[1600];

					memset(&buffer_aux, 0,1600);
					n = recvfrom(i, buffer_aux, sizeof(buffer_aux), 0,(struct sockaddr*) &cli_addr, &clilen);
					DIE(n < 0, "recvfrom UDP");

					// "spargem" mesajul corespunzator in topic tip_date continut
					memcpy(&topic, &buffer_aux, 50);
					topic[50] = '\0';
					tip_date = buffer_aux[50];
					memcpy(&continut, &buffer_aux[51], 1500);

					// ne formam noul mesaj ce trebuie trimis catre clientul/clientii TCP
					if (tip_date == 0) {
						uint32_t num = ntohl(*((uint32_t*)(continut + 1)));

						if(continut[0] == 1)
							num *= -1;
						sprintf(buffer_aux, "%s:%d - %s - INT - %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), topic, num);	
					}

					if (tip_date == 1) {
							double realnum = ntohs(*(uint16_t*)(continut));
							realnum /= 100;
							sprintf(buffer_aux, "%s:%d - %s - SHORT_REAL - %.2f\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), topic, realnum);
					}
					
					if (tip_date == 2) {
						double realnum = ntohl(*(uint32_t*)(continut + 1));
							realnum /= pow(10, continut[5]);
							if (continut[0] == 1)
								realnum *= -1;
							sprintf(buffer_aux, "%s:%d - %s - FLOAT - %f\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), topic, realnum);
					}

					if (tip_date == 3)
						sprintf(buffer_aux, "%s:%d - %s - STRING - %s\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), topic, continut);

					// trimite mesajul tuturor clientilor ce sunt abonati la acest topic, daca sunt activi
					// daca nu sunt activi (adica nu ii gasim in lista de useri), updatam in lista sf
					// (adica clientului corespunzator ii punem mesajul in vectorul de stringuri)
					for (int j = 0; j < list_size; j++) {
						if (strcmp(topic_list[j]->topic, topic) == 0) {
							for (ListNode *p = topic_list[j]->head; p != NULL; p = p->next) {
								ListNode *q = search(user_list, p->data);
								if (q != NULL) {
									proto_tcp_msg m;
									strcpy(m.id, q->data);
									strcpy(m.buffer, buffer_aux);
									m.dim = strlen(buffer_aux) + 1;
									 n = send(q->sock, &m, sizeof(int) + m.dim, 0);
									 DIE(n < 0, "Eroare la send tcp from udp");
								}
								else if (p->sock == 1) {
									updateSFList(sf_list, p->data, buffer_aux);
								}
							}
						}

					}
					// daca primeste mesaj de la STDIN se accepta doar mesajul exit\n
				} else if (i == 0){
					memset(message.buffer, 0, BUFLEN);
					fgets(message.buffer, BUFLEN - 1, stdin);

					if (strcmp(message.buffer, "exit\n") != 0) {
						break;
					}
					else {

						// se trimite tuturor clientilor mesajul de exit pentru a se inchide
						// apoi se inchid si socketurile pentru tcp si udp si se inchide programul
						message.dim = strlen(message.buffer) + 1;
						for (int i = 1; i <= fdmax; i++)
							if (FD_ISSET(i, &read_fds))
								if (i != udp_sockfd && i != tcp_sockfd) {
									n = send(i, &message, sizeof(int) + message.dim, 0);
									DIE(n < 0, "Eroare la send close tcp");
								}
						close(tcp_sockfd);
						close(udp_sockfd);
						exit(0);
					}
				} else {
		
					// daca primeste mesaj de exit se sterge userul din lista clientilor 
					// activi si se inchide socketul pentru acesta + se sterge din multime
					n = recv(i, &message, sizeof(proto_tcp_msg), 0);
					DIE(n < 0, "recv");

					if (strcmp(message.buffer, "exit\n") == 0) {
						printf("Client %s disconnected.\n", message.id);
						delete(user_list, message.id);
						close(i);
						FD_CLR(i, &read_fds);
					} else {
						// prin conventia facuta in subcriber stiu ca mesajele sunt de forma
						// subscribe topic [0-1] sau unsubscribe topic
						char *token = strtok(message.buffer, " \n");
						char *aux = strdup(token);
						int sf;

						token = strtok(NULL, " \n");
						token[strlen(token)] = '\0';

						// daca este de tipul subscribe preiau si parametrul sf
						if (strcmp("subscribe",aux) == 0)
							sf = atoi(strtok(NULL," \n"));
						if (strcmp("subscribe",aux) == 0) {
							// daca acesta este 1 adaug in lista userilor cu sf = 1 userul 
							// ce a trimis mesajul de subscribe
							if (sf == 1) {
								if(searchSF(sf_list, message.id) == NULL)
									insertSFList(sf_list, message.id);
							}
							int j;
							// daca topicul exista deja adaug userul la lista topicului corespunzator
							// altfel creez o lista cu topicul primit si il adaug la aceasta
							for (j = 0; j < list_size; j++) {
								if (strcmp(topic_list[j]->topic, token) == 0) {
									if (search(topic_list[j],message.id) == NULL)
										insert(topic_list[j], message.id, sf);
									break;
								}
							}
							if (j == list_size) {
								topic_list[j] = createList(token);
								insert(topic_list[j], message.id, sf);
								list_size++;
							}
						}
						// in cazul unsubscribeului doar sterg userul cu id-ul corespunzator
						// din fiecare lista de topicuri daca acesta se afla ca nod in cadrul lor
						if (strcmp("unsubscribe", aux) == 0) {
							for (int j = 0; j < list_size; j++){
								if (strcmp(topic_list[j]->topic, token) == 0) {
									delete(topic_list[j], message.id);
								}
							}
						}
					}
				}
			}
		}
	}
	close(tcp_sockfd);
	close(udp_sockfd);
	return 0;
}


