#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>

#define MAXLINE 4096

int main(int argc, char** argv)
{
    int sockfd, n;
    char recvline[4096], sendline[4096];
    struct sockaddr_in servaddr;
	short port = 6666;
	const char *dest = NULL;

    if( argc == 1)
	{
		dest = "127.0.0.1";
		port = 6666;
    }
	else if (argc == 2)
	{
		dest = argv[1];
		port = 6666;
	}
	else if ( argc == 3)
	{
		dest = argv[1];
		port = atoi(argv[2]);
	}
	else
	{
		printf("usage: ./client <ipaddress> <port>\n");
		exit(0);
	}

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET, dest, &servaddr.sin_addr) <= 0)
	{
		printf("ERROR::inet_pton error for %s\n",argv[1]);
		exit(0);
    }

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
    }

    printf("INFO::Send msg to server: \n");
    fgets(sendline, 4096, stdin);
    if( send(sockfd, sendline, strlen(sendline), 0) < 0)
    {
		printf("ERROR::Send message error: %s(errno: %d)\n", strerror(errno), errno);
		exit(0);
    }

    close(sockfd);
    exit(0);
}
