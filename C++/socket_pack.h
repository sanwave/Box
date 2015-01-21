
#include "log.h"

using namespace Matrix;

#ifdef WIN32

#include <winsock2.h>
#pragma comment(lib,"Ws2_32");

SOCKET Socket(int family, int type, int protocol)
{
	SOCKET sockfd;
	if ((sockfd = socket(family, type, protocol)) < 0)
	{
		Log::Write("ERROR", "create socket error");
		::closesocket(sockfd);
		exit(-1);
	}
	return (sockfd);
}

int Bind(SOCKET sockfd, struct sockaddr * addr, int len)
{
	int n;
	if ((n = bind(sockfd, addr, len)) < 0)
	{
		Log::Write("ERROR", "bind socket error");
		::closesocket(sockfd);
		exit(-1);
	}
	return (n);
}

int Listen(SOCKET sockfd, int backlog)
{
	int n;
	if ((n = listen(sockfd, backlog)) == -1)
	{
		Log::Write("ERROR", "listen socket error");
		::closesocket(sockfd);
		exit(-1);
	}
	return (n);
}

SOCKET Accept(SOCKET sockfd, struct sockaddr * addr, int * len)
{
	SOCKET connfd;
	if ((connfd = accept(sockfd, addr, len)) == -1)
	{
		Log::Write("ERROR", "accept socket error");
		//::close(connfd);
		//::close(sockfd);
		//exit(-1);
	}
	return (connfd);
}

int Connect(SOCKET sockfd, const struct sockaddr * addr, int len)
{
	int n;
	if ((n = connect(sockfd, addr, len)) < 0)
	{
		Log::Write("ERROR", "connect socket error");
		::closesocket(sockfd);
		exit(-1);
	}
	return (n);
}

int Send(SOCKET sockfd, const char * buff, int len, int flags)
{
	int n;
	n = send(sockfd, buff, len, flags);
	return (n);
}

int Recv(SOCKET connfd, char * buff, int len, int flags)
{
	int n;
	n = recv(connfd, buff, len, 0);
	return (n);
}

int Close(SOCKET fd)
{
	return ::closesocket(fd);
}

int Select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds,const timeval * timeout)
{
	int n;
	if ((n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
	{
		Log::Write("ERROR", "select error");
		exit(-1);
	}
	return n;
}

int Read(SOCKET connfd, char * buff, int len, int flags)
{
	int n;
	n = recv(connfd, buff, len, 0);
	return n;
}

#elif linux

int Socket(int family, int type, int protocol)
{
	int sockfd;
	if ((sockfd = socket(family, type, protocol)) < 0)
	{
		Log::Write("ERROR", "create socket error");
		::close(sockfd);
		exit(-1);
	}
	return (sockfd);
}

int Bind(int sockfd, struct sockaddr * addr, socklen_t len)
{
	int n;
	if ((n = bind(sockfd, addr, sizeof(addr))) < 0)
	{
		Log::Write("ERROR", "bind socket error");
		::close(sockfd);
		exit(-1);
	}
	return (n);
}

int Listen(int sockfd, int backlog)
{
	int n;
	if ((n = listen(sockfd, backlog)) == -1)
	{
		Log::Write("ERROR", "listen socket error");
		::close(sockfd);
		exit(-1);
	}
	return (n);
}

int Accept(int sockfd, struct sockaddr * addr, socklen_t * len)
{
	int connfd;
	if ((connfd = accept(sockfd, NULL, NULL)) == -1)
	{
		Log::Write("ERROR", "accept socket error");
		//::close(connfd);
		//::close(sockfd);
		//exit(-1);
	}
	return (connfd);
}

int Connect(int sockfd, const struct sockaddr * addr, socklen_t len)
{
	int n;
	if ((n = connect(sockfd, addr, sizeof(addr))) < 0)
	{
		Log::Write("ERROR", "connect socket error");
		::close(sockfd);
		exit(-1);
	}
	return (n);
}

int Send(int sockfd, const char * buff, int len, int flags)
{
	int n;
	n = send(sockfd, buff, len, flags);
	return (n);
}

int Recv(int connfd, char * buff, int len, int flags)
{
	int n;
	n = recv(connfd, buff, len, 0);
	return (n);
}

int Close(int fd)
{
	::close(fd);
}

#endif
