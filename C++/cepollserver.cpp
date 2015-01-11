
#include <iostream>
#include <sys/socket.h>  
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdio.h>  
#include <errno.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

#define MAX_EVENTS 500

class BuffEvent
{
public:	
	int m_Fd;
	int m_Events;
	void *m_Arg;
	void *m_EpollServerPtr;
	void (*CallBack)(int m_Fd, int m_Events, void *m_Arg, void *m_EpollServerPtr);
	
	int m_Status; // 1: in epoll wait list, 0 not in  
	char m_Buff[128]; // recv data buffer  
	int m_Len, m_Offset;  
	long m_LastActive; // last active time 
	
	void Set(int sFd, void (*callBack)(int, int, void*, void*), void *pEpollServer)
	{
		m_Fd = sFd;      
		m_Events = 0;  
		m_Arg = this;
		m_EpollServerPtr = pEpollServer;
		CallBack = callBack;  
		m_Status = 0;
		bzero(m_Buff, sizeof(m_Buff)); 
		m_Len = 0; 
		m_Offset = 0;     
		m_LastActive = time(NULL);  
	}
	
	void Add(int epollFd, int events)
	{
		struct epoll_event epv = {0, {0}};  
		int op;  
		epv.data.ptr = this;  
		epv.events = m_Events = events;  
		if(m_Status == 1){  
			op = EPOLL_CTL_MOD;  
		}  
		else{  
			op = EPOLL_CTL_ADD;  
			m_Status = 1;
		}  
		if(epoll_ctl(epollFd, op, m_Fd, &epv) < 0)  
			printf("ERROR::Add event failed, socket fd is %d, evnets is %d\n", m_Fd, events);  
		else  
			printf("INFO::Add event successfully, soket fd is %d, op is %d, evnets is %0X\n", m_Fd, op, events);  
	}
	
	void Del(int epollFd)
	{
		struct epoll_event epv = {0, {0}};  
		if(m_Status != 1)
		{
			return;
		}
		epv.data.ptr = this;  
		m_Status = 0;
		epoll_ctl(epollFd, EPOLL_CTL_DEL, m_Fd, &epv);  
	}	  
};

class CEpollServer
{
public:
	int m_EpollFd;
	in_addr_t m_LocalAddr;
	in_addr_t m_RemoteAddr;
	short m_LocalPort;
	short m_RemotePort;
	BuffEvent m_Events[MAX_EVENTS+1]; // m_Events[MAX_EVENTS] is used by listen fd  

	~CEpollServer()
	{
		close(m_EpollFd);
	}
	
	void GetLocalAddr(int sFd)
	{
		struct sockaddr addr;
		struct sockaddr_in* addr_v4;
		socklen_t addr_len = sizeof(addr);
		
		bzero(&addr, sizeof(addr));  //memset, fill with 0
		if (0 == getsockname(sFd, &addr, &addr_len))
		{
			if (addr.sa_family == AF_INET)
			{
				addr_v4 = (sockaddr_in*)&addr;
				m_LocalAddr = addr_v4->sin_addr.s_addr;
				m_LocalPort = ntohs(addr_v4->sin_port);
			}
		}
	}
	
	void GetRemoteAddr(int sFd)
	{
		struct sockaddr addr;
		struct sockaddr_in* addr_v4;
		socklen_t addr_len = sizeof(addr);
		
		bzero(&addr, sizeof(addr));  //memset, fill with 0
		if (0 == getpeername(sFd, &addr, &addr_len))
		{
			if (addr.sa_family == AF_INET)
			{
				addr_v4 = (sockaddr_in*)&addr;
				m_RemoteAddr = addr_v4->sin_addr.s_addr;
				m_RemotePort = ntohs(addr_v4->sin_port);
			}
		}
	}
	
	int CreateAndBind(short port)
	{		
		int sFd = socket(AF_INET, SOCK_STREAM, 0);  
		if(sFd == -1)
		{
			printf("ERROR::Create socket error: %s(errno: %d)\n",strerror(errno),errno);			
		}
		else
		{
			printf("INFO::Server listen fd is %d\n", sFd);  
		}

		sockaddr_in servAddr;  
		bzero(&servAddr, sizeof(servAddr));  //memset, fill with 0
		servAddr.sin_family = AF_INET;  
		servAddr.sin_addr.s_addr = INADDR_ANY;  
		servAddr.sin_port = htons(port);  
		
		if(bind(sFd, (const sockaddr*)&servAddr, sizeof(servAddr))==-1)
		{
			printf("ERROR::Bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		}
		
		m_LocalPort = servAddr.sin_port;
		
		if(listen(sFd, -1)==-1)
		{
			printf("ERROR::Listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		}
		
		return sFd;
	}
	
	int SetNonBlock(int nFd)
	{
		if( fcntl(nFd, F_SETFL, O_NONBLOCK) )
		{
			perror ("fcntl");
			return -1;
		}// set non-blocking		
		return 0;		
	}
	
	int InitEpoll(int sFd)
	{
		m_EpollFd = epoll_create(MAX_EVENTS);  
		if(m_EpollFd <= 0)
		{
			printf("ERROR::Create epoll failed, return %d \n", m_EpollFd);
		}
		m_Events[MAX_EVENTS].Set(sFd, this->AcceptConn, this);  
		// add listen socket  
		m_Events[MAX_EVENTS].Add(m_EpollFd, EPOLLIN);
		// bind & listen
		return m_EpollFd;
	}
	
	static void AcceptConn(int sFd, int events, void *arg, void *pEpollServer)  
	{
		CEpollServer *pThis = (CEpollServer *)pEpollServer;
		struct sockaddr_in sin;  
		socklen_t len = sizeof(struct sockaddr_in);  
		int connFd, i;  
		// accept  
		if((connFd = accept(sFd, (struct sockaddr*)&sin, &len)) == -1)  
		{
			if(errno != EAGAIN && errno != EINTR)  
			{
			}
			printf("ERROR::%s: accept, %d", __func__, errno);  
			return;
		}
		do  
		{
			for(i = 0; i < MAX_EVENTS; i++)
			{
				if(pThis->m_Events[i].m_Status == 0)
				{
					break;
				}
			}
			if(i == MAX_EVENTS)  
			{  
				printf("INFO::%s:Max connection limit[%d].", __func__, MAX_EVENTS);  
				break;  
			}
			
			if(pThis->SetNonBlock(connFd) < 0)
			{
				//printf("%s: fcntl nonblocking failed:%d", __func__, iret);
				break;
			}
			// add a read event for receive data  
			pThis->m_Events[i].Set(connFd, RecvData, pThis);  
			pThis->m_Events[i].Add(pThis->m_EpollFd, EPOLLIN);
				
		}while(0);			
		printf("INFO::New connecion from %s:%d,\tTime:%ld,\tIndex:%d\n", inet_ntoa(sin.sin_addr), 
				ntohs(sin.sin_port), pThis->m_Events[i].m_LastActive, i);  
	}

	static void RecvData(int sFd, int events, void *arg, void *pEpollServer)  
	{
		CEpollServer *pThis=(CEpollServer *)pEpollServer;
		BuffEvent *pEvent = (BuffEvent*)arg; 
		// receive data
		int len = recv(sFd, pEvent->m_Buff+pEvent->m_Len, sizeof(pEvent->m_Buff)-1-pEvent->m_Len, 0);
		pEvent->Del(pThis->m_EpollFd);
		if(len > 0)
		{
			pEvent->m_Len += len;
			pEvent->m_Buff[len] = '\0';
			printf("INFO::Receive a msg:%s, from connection fd = %d\n", pEvent->m_Buff, sFd);
			
			char ans[150];
			sprintf(ans, "You said %s. This is from CEpollServer.\n", 
				pEvent->m_Buff/*, inet_ntoa(pThis->m_LocalAddr), pThis->m_LocalPort*/);
			
			// change to send event  
			pEvent->Set(sFd, SendData, pThis);
			pEvent->Add(pThis->m_EpollFd, EPOLLOUT);
						
			send(sFd, ans, strlen(ans), 0);
		}  
		else if(len == 0)  
		{
			close(pEvent->m_Fd);  
			printf("INFO::[fd=%d] pos[%d], closed gracefully.\n", sFd, pEvent-pThis->m_Events);  
		}  
		else  
		{  
			close(pEvent->m_Fd);  
			printf("ERROR::ecv[fd=%d] error[%d]:%s\n", sFd, errno, strerror(errno));  
		}  
	}  

	static void SendData(int sFd, int events, void *arg, void *pEpollServer)  
	{
		CEpollServer *pThis=(CEpollServer *)pEpollServer;
		BuffEvent *pEvent = (BuffEvent*)arg; 
		// send data  
		int len = send(sFd, pEvent->m_Buff + pEvent->m_Offset, pEvent->m_Len - pEvent->m_Offset, 0);
		if(len > 0)
		{
			printf("INFO::Send data, fd is %d, [%d<->%d]%s\n", sFd, len, pEvent->m_Len, pEvent->m_Buff);
			pEvent->m_Offset += len;
			if(pEvent->m_Offset == pEvent->m_Len)
			{
				// change to receive event
				pEvent->Del(pThis->m_EpollFd);
				pEvent->Set(sFd, RecvData, pThis);  
				pEvent->Add(pThis->m_EpollFd, EPOLLIN);  
			}
		}  
		else
		{
			close(pEvent->m_Fd);
			pEvent->Del(pThis->m_EpollFd);
			printf("ERROR::Send fd is %d, error[%d]\n", sFd, errno);
		}
	}
	
	void Run()
	{
		struct epoll_event epollEvents[MAX_EVENTS];	 
		int checkPos = 0;  
		
		while(true)
		{
			long now = time(NULL);  
			for(int i = 0; i < 100; i++, checkPos++) // doesn't check listen fd  
			{  
				if(checkPos == MAX_EVENTS)
				{
					checkPos = 0; // recycle
				}				
				if(m_Events[checkPos].m_Status != 1)
				{
					continue;
				}
				long duration = now - m_Events[checkPos].m_LastActive;  
				if(duration >= 60) // 60s timeout  
				{
					close(m_Events[checkPos].m_Fd);  
					printf("INFO::[fd=%d] timeout[%ld--%ld].\n", m_Events[checkPos].m_Fd, m_Events[checkPos].m_LastActive, now);
					m_Events[checkPos].Del(m_EpollFd);
				}  
			}
			int fds = epoll_wait(m_EpollFd, epollEvents, MAX_EVENTS, 1000);  
			if(fds < 0)
			{  
				printf("ERROR::epoll_wait error, exit\n");  
				break;  
			}  
			for(int i = 0; i < fds; i++)
			{  
				BuffEvent *pEvent = (BuffEvent *)epollEvents[i].data.ptr;  
				if((epollEvents[i].events & EPOLLIN))// && (pEvent->m_Events&EPOLLIN)) // read event  
				{  
					pEvent->CallBack(pEvent->m_Fd, epollEvents[i].events, pEvent->m_Arg, this);  
				}
				if((epollEvents[i].events & EPOLLOUT))// && (pEvent->m_Events&EPOLLOUT)) // write event  
				{  
					pEvent->CallBack(pEvent->m_Fd, epollEvents[i].events, pEvent->m_Arg, this);  
				}
			}
		}
	}
};

int main(int argc, char **argv)  
{	
    unsigned short port = 12345; // default port  
    if(argc == 2)
	{
        port = atoi(argv[1]);  
    }
	printf("\n\n==============       Epoll Server Demo       ==============\n\n", port);  
    printf("INFO::Server is running on port %d\n", port); 
	
	CEpollServer server = CEpollServer();
    int sFd = server.CreateAndBind(port);    
	server.SetNonBlock(sFd);
	server.InitEpoll(sFd);    
	server.Run();	
    return 0;  
}
