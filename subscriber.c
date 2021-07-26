#include "helpers.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret, r;
	struct sockaddr_in serv_addr;
	fd_set read_fds;	
	fd_set tmp_fds;		
	int fdmax;	

	if (argc < 3) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	proto_tcp_msg message; 
	strcpy(message.id, argv[1]);
	n = send(sockfd, &message, sizeof(proto_tcp_msg), 0);
	DIE(n < 0, "send");

	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	FD_SET(0, &read_fds);

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if (FD_ISSET(0, &tmp_fds)) {
	  		// se citeste de la tastatura
			memset(message.buffer, 0,BUFLEN);
			fgets(message.buffer, BUFLEN - 1, stdin);
			strcpy(message.id, argv[1]);
			// daca se citeste exit se trimite mesaj 
			// serverului ca acesta se inchide
			// serverul il va identifica dupa id
			if (strcmp(message.buffer, "exit\n") == 0) {
				n = send(sockfd, &message, sizeof(proto_tcp_msg), 0);
				DIE(n < 0, "send");
				close(sockfd);
				break;
			}

			// altfel verific ce pattern are mesajul
			char *test = strdup(message.buffer);
			char *token = strtok(test, " \n");
			char *aux = strdup(token);

			// daca primul nu este subscribe sau unsubscribe atunci il consider invalid
			if (aux == NULL || (strcmp(aux, "subscribe") != 0 && strcmp(aux, "unsubscribe") != 0))
				continue;
			// daca nu am alt cuvant dupa subscribe / unsubscribe il consider invalid
			token = strtok(NULL, " \n");
			if (token == NULL)
				continue;
			token[strlen(token)] = '\0';

			// daca este de tip subscribe topic verific daca urmatorul cuvant este 0 sau 1
			// si daca nu mai urmeaza altceva dupa
			if (strcmp("subscribe",aux) == 0) {
				char *char_sf = strtok(NULL," \n");
				if (char_sf == NULL || (strcmp(char_sf,"1") != 0 && strcmp(char_sf,"0") != 0)) 
					continue;
				if (strtok(NULL," \n") != NULL)
					continue;			
			}
			if (strcmp("unsubscribe", aux) == 0)
				if (strtok(NULL," \n") != NULL)
					continue;
			
			// altfel am un mesaj corect si il trimit catre server		
			n = send(sockfd, &message, sizeof(proto_tcp_msg), 0);
			DIE(n < 0, "send");
			if (strncmp("subscribe",message.buffer,9) == 0)
				printf("Subscribed to topic.\n");

			if (strncmp("unsubscribe",message.buffer,11) == 0)
				printf("Unsubscribed from topic.\n");	
		}
		
		if (FD_ISSET(sockfd, &tmp_fds)) {

			memset(message.buffer, 0, BUFLEN);

			int size_recv, total_size = 0;
			int dim;
			char buffer_aux[BUFLEN];

			// citesc de pe socketul venit in dimensiune
			// sizeof int deoarece conform proto_tcp_msg alex
			// primii 4 octeti sunt dimensiunea mesajului
			r = recv(sockfd, &dim, sizeof(int), 0);
			DIE(r < 0, "Eroare la recv dimensiune");
			// in while citesc de pe socket atat cat am dimensiunea
			// practic cumulez in total_size suma octetilor primiti 
			// de pe recv pana cand acestia ajung la dimensiunea
			// specificata de campul dimensiune al protocolului
			int lungime = dim;
			while (1) {

				memset(buffer_aux, 0, BUFLEN);
				if ((size_recv = recv(sockfd, buffer_aux,dim,0)) < 0) {
					DIE(size_recv, "Eroare la recv size_recv");
					break;
				}	
				else {
					total_size += size_recv;
					if (strcmp(buffer_aux, "exit\n") == 0) {
						close(sockfd);
						exit(0);
					}
					printf("%s",buffer_aux);

					if (total_size == lungime)
						break;
					
					dim -= size_recv;
					if (dim <= 0)
						break;
				}
			}
		}
	}
	close(sockfd);
	return 0;
}
