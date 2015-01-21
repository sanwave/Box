
/*
*
*
*
*	HttpServer Class Header File		In Matrix
*
*	Created by Bonbon	2015.01.08
*
*	Updated by Bonbon	2015.01.10
*
*/


#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "file.h"
#include "socket_pack.h"
#include "environment.h"
#include <fcntl.h>

#ifdef WIN32
#include <Ws2tcpip.h>
#else
#include <arpa/net.h>
#endif

#define REQ_BUF_SIZE 1024
#define RESP_BUF_SIZE 1024
#define BACK_LOG 5

namespace Matrix
{

	class HttpServer
	{
	public:
		HttpServer()
		{
#ifdef WIN32
			WSADATA wd;
			int ret = WSAStartup(MAKEWORD(2, 0), &wd);
			if (ret < 0)
			{
				fprintf(stderr, "winsock startup failed\n");
				exit(-1);
			}
#endif
		}

		~HttpServer()
		{
			::Close(m_listenfd);
#ifdef WIN32
			WSACleanup();
#endif
		}

		void Init()
		{
			m_listenfd = Socket(AF_INET, SOCK_STREAM, 0);
			/* bind and listen */
			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(8080);
			inet_pton(AF_INET, INADDR_ANY, &addr.sin_addr);

			Bind(m_listenfd, (struct sockaddr*)&addr, sizeof(addr));
			Listen(m_listenfd, 1023);

			SetNonBlock(m_listenfd);
		}

#ifdef WIN32
		int SetNonBlock(SOCKET sockfd)
		{
			int nNetTimeout=100;//1秒
			//发送时限
			setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout,sizeof(int));
			//接收时限
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout,sizeof(int));
			return 0;
		}
#else

		int SetNonBlock(int sockfd)
		{
			if (fcntl(sockfd, F_SETFL, O_NONBLOCK))
			{
				perror("fcntl");
				return -1;
			}
			return 0;
		}
#endif

		void Run()
		{
#ifdef WIN32
			SOCKET connfd;
#else
			int connfd;			
#endif
			//int fd_A[BACK_LOG];
			struct timeval tv;
			fd_set fdsr;
			int maxsock = m_listenfd;
			//int ret=0;
			//int conn_amount = 0;

			struct sockaddr_in client_addr; // connector's address information
			socklen_t sin_size = sizeof(client_addr);

			while (1)
			{
				FD_ZERO(&fdsr);
				FD_SET(m_listenfd, &fdsr);
				tv.tv_sec = 10;
				tv.tv_usec = 100;

				/*for (int i = 0; i < BACK_LOG; i++) 
				{
					if (fd_A[i] != 0) 
					{
						FD_SET(fd_A[i], &fdsr);
					}
				}*/

				if (0 == Select(maxsock + 1, &fdsr, NULL, NULL, &tv))
				{
					printf("timeout\n");
					continue;
				}

				/*for (int i = 0; i < conn_amount; i++) 
				{
					if (FD_ISSET(fd_A[i], &fdsr)) 
					{
						char buf[REQ_BUF_SIZE] = { 0 };
						ret = recv(fd_A[i], buf, sizeof(buf), 0);
						if (ret <= 0) 
						{
							printf("client[%d] close\n", i);
							Close(fd_A[i]);
							FD_CLR(fd_A[i], &fdsr);
							fd_A[i] = 0;
						}
						else 
						{
							if (ret < REQ_BUF_SIZE)
							{
								memset(&buf[ret], '\0', 1);
							}
							printf("client[%d] send:%s\n", i, buf);
						}
					}
				}*/

				if (FD_ISSET(m_listenfd, &fdsr))
				{
					connfd = Accept(m_listenfd, (struct sockaddr *)&client_addr, &sin_size);
					char request[REQ_BUF_SIZE] = { 0 };

					int ret = recv(connfd, request, sizeof(request), 0);
					std::cout << "ret=" << ret << std::endl;

					if (ret <= 0)
					{

					}
					else if (ret < REQ_BUF_SIZE && NULL != strstr(request, "\r\n\r\n"))
					{
						Log::Write("INFO", request);
						printf(request);
						Response(connfd, request);
					}
					else
					{
						std::string response = "HTTP/1.1 400 BAD REQUEST\r\n\r\n";
						send(connfd, response.c_str(), response.length(), 0);
					}
					Close(connfd);

					/*if (conn_amount < BACK_LOG) 
					{
						fd_A[conn_amount++] = connfd;
						printf("new connection client[%d] %s:%d\n", conn_amount,
							inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						if (connfd > maxsock)
							maxsock = connfd;
					}
					else {
						printf("max connections arrive, exit\n");
						send(connfd, "bye", 4, 0);
						Close(connfd);
						break;
					}*/
				}				
				
			}
		}

		void Response(SOCKET con, const char *request)
		{
			/* get the method */
			if (0 == strncmp(request, "GET", 3))
			{
				GetHandler(con, request + 4);
			}
			else if (0 == strncmp(request, "POST", 4))
			{
				PostHandler(con, request + 5);
			}
			else if (0 == strncmp(request, "HEAD", 4))
			{
				HeadHandler(con, request + 5);
			}
		}

		void GetHandler(SOCKET con, const char *request)
		{
			m_request =std::string(request);

			const char * current_dir = Environment::GetCurrentDir();
			std::string uri = std::string(current_dir) + m_request.substr(0, m_request.find(" ") + 1);
			if (NULL != current_dir)
			{
				delete current_dir;
				current_dir = NULL;
			}
			if (File::Exist(uri.c_str()))
			{
				const char * content = File(uri.data()).Binary(-1);
				size_t file_size = File::GetSize(uri.data());
				char *response = new char[RESP_BUF_SIZE + file_size];

				sprintf_s(response, RESP_BUF_SIZE + file_size, "HTTP/1.0 200 OK\r\nContent-Type: */*\r\nContent-Length: %d\r\n\r\n%s", file_size, content);
				send(con, response, strlen(response), 0);

				if (NULL != response)
				{
					delete response;
					response = NULL;
				}
				if (NULL != content)
				{
					delete content;
					content = NULL;
				}
			}
			else
			{
				std::string response = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
				send(con, response.c_str(), response.length(), 0);
			}
		}

		void PostHandler(SOCKET con, const char *request)
		{
			std::string response = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
			send(con, response.c_str(), response.length(), 0);
		}

		void HeadHandler(SOCKET con, const char *request)
		{
			std::string response = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
			send(con, response.c_str(), response.length(), 0);
		}

	private:
#ifdef WIN32
		SOCKET m_listenfd;
#else
		int m_listenfd;
#endif
		std::string m_request;

	};

}

#endif
