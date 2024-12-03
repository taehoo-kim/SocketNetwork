#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<time.h>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define BUF_SIZE 100
#define MAX_CLNT 100
#define MAX_IP 30

void * handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
void menu(char port[]);


/***************************/

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
char answer[BUF_SIZE];
int check = 1;
int pr = 0;
int number = 0; //number of client
int* resault;
int start = 0;
char* nick;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;

	/** time log **/
	struct tm *t;
	time_t timer = time(NULL);
	t = localtime(&timer);

	if (argc != 2)
	{
		printf(" Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	menu(argv[1]);

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	printf("number of Client : ");
	scanf("%d", &number);
	int np;
	printf("number of problem : ");
	scanf("%d", &np);
	resault = (int*)malloc(sizeof(int) * (number));
	nick = (char*)malloc(sizeof(char) * (number));
	for (int i = 0; i < number; i++)
		resault[i] = 0;


	for(int i=0;i<number;i++)
	{
		t = localtime(&timer);
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf(" Connceted client IP : %s ", inet_ntoa(clnt_adr.sin_addr));
		printf("(%d-%d-%d %d:%d)\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min);
	}

	close(serv_sock);
	

	pthread_mutex_lock(&mutx);
	start = 1;
	pthread_mutex_unlock(&mutx);
	fgets(answer, BUF_SIZE, stdin);
	strcpy(answer, "heckasdfqwerasdfzxcvbuffer");//비우는용

	for (int i=0;i<np;) 
	{
		if (check != 0) {
			pthread_mutex_lock(&mutx);
			check = 0;
			pthread_mutex_unlock(&mutx);
			printf("\nyou need to enter answer\n");
			fgets(answer, BUF_SIZE, stdin);
			char s_msg[21] = "server > Game Start\n";
			send_msg(s_msg, 20);
			printf("%s", s_msg);
		}
		else {
			char name[10] = "server > ";
			char name_msg[10 + BUF_SIZE];
			char msg[BUF_SIZE];
			fgets(msg, BUF_SIZE, stdin);
			sprintf(name_msg, "%s %s", name, msg);
			send_msg(name_msg, strlen(name_msg));
		}
		i = pr;
	}

	//for (int i = 0; i < number; i++) printf("%d\n", resault[i]);

	int max = resault[0];
	int index = 0;
	for (int i = 0; i < number; i++) {
		if (resault[i] > max) {
			max = resault[i];
			index = i;
		}
	}

	char winner[50];
	sprintf(winner,"Winner is %s %d", &nick[index], max);
	send_msg(winner, strlen(winner));
	printf("%s", winner);

	int out;
	if ((out = open("./winner.txt", O_CREAT | O_WRONLY | O_TRUNC)) != -1)
		write(out, winner, strlen(winner));

	else perror("out");
	//FILE *fp = fopen("./winner.txt", "w");
	//fputs(winner, fp);
	//fclose(fp);

	return 0;
}

void *handle_clnt(void *arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];

	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
	{
		send_msg(msg, str_len);
		for (int j = 0; j < str_len; j++) printf("%c",msg[j]);

		if (start == 1) {
			if (strstr(msg, answer) != NULL) {
				pthread_mutex_lock(&mutx);
				check = 1;
				++pr;
				pthread_mutex_unlock(&mutx);
				for (int j = 0; j < number; j++) {
					if (clnt_sock == clnt_socks[j]) {
						char correct_msg[20 + BUF_SIZE];
						msg[str_len] = 0;
						sprintf(correct_msg, "Coreect : %s", msg);
						send_msg(correct_msg, strlen(correct_msg));
						printf("%s\n", correct_msg);
						resault[j] += 1;
						char *term = strtok(msg, " ");
						strcpy(&nick[j],term);
						break;
					}
				}
			}/////
		}
	}

	// remove disconnected client
	pthread_mutex_lock(&mutx);
	for (i = 0; i<clnt_cnt; i++)
	{
		if (clnt_sock == clnt_socks[i])
		{
			while (i++<clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len)
{
	int i;
	pthread_mutex_lock(&mutx);
	for (i = 0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len);
	pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void menu(char port[])
{
	system("clear");
	printf(" **** speed Quiz ****\n");
	printf(" server port    : %s\n", port);
	printf(" ****          Log         ****\n\n");
}
