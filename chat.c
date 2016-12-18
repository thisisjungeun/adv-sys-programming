#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>

#define IP "127.0.0.1"
#define PORT 3000
#define MAX_CLIENT 1024
#define MAX_DATA 1024
#define MAX_EVENTS 10
#define MAX_FILE_SIZE 1024 * 1024

static struct termios term_old;
char *file_num[20];
FILE *fout;
void initTermios(void);
void resetTermios(void);

int launch_chat(char* file_name);
int launch_clients(int num_client);
int launch_server(void);
int get_server_status(void);
void SetNonblockingMode(int fd);
void GenFileNum(char **file_num);
int readaline_and_out(FILE *fin, FILE *fout);

int main(int argc, char *argv[])
{
    int ret = -1;
    int num_client;
    struct timeval before, after;
    int duration;

    GenFileNum(file_num);

    if ((argc != 2) && (argc != 3)) {
        usage:  fprintf(stderr, "usage: %s a|m|s|c num_client\n", argv[0]);
        goto leave;
    }
    if ((strlen(argv[1]) != 1))
        goto usage;
    
    gettimeofday(&before, NULL);

    switch (argv[1][0]) {
      case 'a': if (argc != 3)
                    goto usage;
                if (sscanf(argv[2], "%d", &num_client) != 1)
                    goto usage;
                // Launch Automatic Clients
                ret = launch_clients(num_client);
                break;
      case 's': // Launch Server
                ret = launch_server();
                break;
      case 'm': // Read Server Status
                ret = get_server_status();
                break;
      case 'c': // Start_Interactive Chatting Session
                ret = launch_chat(file_num[0]);
                break;
      default:
                goto usage;
    }

    gettimeofday(&after, NULL);

    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec \n", duration/1000000, duration%1000000);

leave:
    return ret;
}

int launch_chat(char* file_name)
{
    int clientSock;
    FILE *file1, *file2;
    struct sockaddr_in serverAddr;
    struct epoll_event ev, events[MAX_EVENTS];
    fd_set rfds, wfds, efds;
    int nfds, epollfd;
    int ret = -1, ch;
    char rdata[MAX_DATA];
    int i = 1, j;
    struct timeval tm;

    if ((ret = clientSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        goto leave;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(IP);
    serverAddr.sin_port = htons(PORT);

    if ((ret = connect(clientSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))) {
        fprintf(stderr, "connect");
        goto leave1;
    }
    printf("[CLIENT] Connected to %s\n", inet_ntoa(*(struct in_addr*)&serverAddr.sin_addr));

    initTermios();

    // start select version of chatting ...
    i = 1;

    epollfd = epoll_create(10);
    if (epollfd == -1) {
	fprintf(stderr, "epoll_create");
	return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = clientSock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientSock, &ev) == -1) {
	fprintf(stderr, "epoll_ctl");
	return 1;
    }

    file1 = fopen(file_name, "wt");
    file2 = fopen(fout, "wt");

    tm.tv_sec = 0; tm.tv_usec = 1000;
    while (1) {
        if (nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1) == -1) {
		fprintf(stderr, "epoll_wait");
		return 1;
	}

	for (j = 0; j < nfds; j++) {
	    if (events[j].data.fd == clientSock) {
		if (!(ret = recv(clientSock, rdata, MAX_DATA, 0))) {
		    printf("Connection closed by remote host.\n");
		    goto leave1;
		}
		else if (ret > 0) {
		    readaline_and_out(file1, file2);
		    if (send(clientSock, "@", 1, 0) == 0) {
			fprintf(stderr, "send");
			return 1;
		    }
		}
		else
		    break;
	    }		
	}
    }
    leave1:
        resetTermios();
        close(clientSock);
    leave:
        return -1;
}

int launch_server(void)
{
    int serverSock, acceptedSock[20];
    struct sockaddr_in Addr;
    struct epoll_event ev, events[MAX_EVENTS];
    struct timeval tm;
    socklen_t AddrSize = sizeof(Addr);
    char data[MAX_DATA], *p;
    int ret, count, i = 1, j;
    int epollfd, nfds;
    int num_accepted = 0;

    if ((ret = serverSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket");
        goto leave;
    }

    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (void *)&i, sizeof(i));

    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_port = htons(PORT);
    if ((ret = bind(serverSock, (struct sockaddr*)&Addr,sizeof(Addr)))) {
        fprintf(stderr, "bind");
        goto error;
    }

    if ((ret = listen(serverSock, 10))) {
        fprintf(stderr, "listen");
        goto error;
    }

    epollfd = epoll_create(10);
    if (epollfd == -1) {
	fprintf(stderr, "epoll_create");
	exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = serverSock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSock, &ev) == -1) {
	fprintf(stderr, "epoll_ctl");
	exit(EXIT_FAILURE);
    }

    tm.tv_sec = 0; tm.tv_usec = 1000;
    while (1) {
        if ((nfds = epoll_wait(epollfd, events, MAX_DATA, -1)) < 0) {
            fprintf(stderr, "epoll_wait");
            goto error;
        }
       
        //printf("[%d]", count); fflush(stdout);
        for (j = 0; j < nfds; j++) {
            if (events[j].data.fd == serverSock) { 
		if ((acceptedSock[num_accepted++] = accept(serverSock, (struct sockaddr*)&Addr, &AddrSize)) == -1) {
		    ret = -1;
		    fprintf(stderr, "accept");
		    exit(EXIT_FAILURE);
		}

		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = acceptedSock[num_accepted-1];

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptedSock[num_accepted - 1], &ev) == -1) {
			fprintf(stderr, "epoll_ctl");
			exit(EXIT_FAILURE);
		}
		printf("[SERVER] Connected to %s\n", inet_ntoa(*(struct in_addr*)&Addr.sin_addr));
	   }
	   else {
	       if ((ret = count = recv(events[j].data.fd, data, MAX_DATA, 0)) == 0) {
		   fprintf(stderr, "Connection closed by client.\n");
		   break;
               }  

                if (ret < 0) {
		    fprintf(stderr, "recv");
		    break;
	        }

		if (num_accepted >= 20) {
		    for (i = 0; i < num_accepted; i++) {
			if (send(acceptedSock[i], "$", 1, 0) == 0) {
			    fprintf(stderr, "send");
			    close(acceptedSock[i]);
			    return 1;
			}
		    }
		}

	    }
        }   
    }
      
    for (i = 0; i < num_accepted; i++) {
	if (send(acceptedSock[i], "%", 1, 0) == 0) {
	    fprintf(stderr, "send");
	    close(acceptedSock[i]);
	    return 1;
	}
	close(acceptedSock[i]);
    }

    error:
        close(serverSock);
    leave:
        return ret;
}

int launch_clients(int num_client)
{
    int serverSock;
    pthread_t clientThread[MAX_CLIENT];
    void *thread_return;
    struct sockaddr_in serverAddr;
    int i;

    serverSock = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(IP);
    serverAddr.sin_port = htons(PORT);

    if (connect(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
	fprintf(stderr, "connect error\n");
	return 1;
    }

    for (i = 0; i < 20; i++) 
	pthread_create(&clientThread[i], NULL,(void*) launch_chat(file_num[i]),(void*)serverSock);
    

    for (i = 0; i < 20; i++) 
	pthread_join(clientThread[i], &thread_return);

    return 0;
}

int get_server_status(void)
{
    return 0;
}

/* Initialize new terminal i/o settings */
void initTermios(void) 
{
    struct termios term_new;

    tcgetattr(0, &term_old); /* grab old terminal i/o settings */
    term_new = term_old; /* make new settings same as old settings */
    term_new.c_lflag &= ~ICANON; /* disable buffered i/o */
    term_new.c_lflag &= ~ECHO;   /* set no echo mode */
    tcsetattr(0, TCSANOW, &term_new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
    tcsetattr(0, TCSANOW, &term_old);
}

void SetNonblockingMode(int fd) 
{ 
    int flag=fcntl(fd, F_GETFL, 0);
 
    fcntl(fd, F_SETFL, flag|O_NONBLOCK); 
} 

void GenFileNum(char **file_num)
{
    int i;
    char name[2];

    for (i = 0; i < 20; i++) {
	name[0] = '0' + (i+1)/10;
        name[1] = '0' + (i+1)%10;
	strcpy(file_num[i], "/tmp/file_00");
	strcat(file_num[i], name);
    }
}

int readaline_and_out(FILE *fin, FILE *fout) 
{     
    int ch, count = 0; 

 
    do { 
        if ((ch = fgetc(fin)) == EOF) { 
            if (!count) 
                return 1; 
            else { 
                fputc(0x0a, fout); 
                break; 
            } 
        } 
        fputc(ch, fout); 
        count++; 
    } while (ch != 0x0a); 
    return 0; 
} 
   
