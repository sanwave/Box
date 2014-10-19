
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
#define DEBUG

class ListenerEvent
{
public:	
	int m_Fd;
	int m_Events;
	void *m_Arg;
	void *m_ListenerPtr;
	void (*m_CallBack)(int m_Fd, int m_Events, void *m_Arg, void *m_ListenerPtr);
	
	int m_Status; // 1: in epoll wait list, 0 not in  
	char m_Buff[128]; // recv data buffer  
	int m_Len, m_Offset;  
	long m_LastActive; // last active time 
	
	void Set(int sFd, void (*callBack)(int, int, void*, void*), void *pListener)
	{
		m_Fd = sFd;      
		m_Events = 0;  
		m_Arg = this;
		m_ListenerPtr=pListener;
		m_CallBack = callBack;  
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
			printf("ERROR::Event Add failed, socket fd is %d, evnets is %d\n", m_Fd, events);  
		else  
			printf("INFO::Event Add OK, soket fd is %d, op is %d, evnets is %0X\n", m_Fd, op, events);  
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

class Listener
{
public:
	int m_EpollFd;
	//static Listener *m_ThisPtr;//=NULL;
	ListenerEvent m_Events[MAX_EVENTS+1]; // m_Events[MAX_EVENTS] is used by listen fd  
		
	~Listener()
	{
		close(m_EpollFd);
		//m_ThisPtr = NULL;
	}
	
	// create & bind listen socket, and add to epoll, set non-blocking 
	int CreateAndBind(short port)
	{		
		int sFd = socket(AF_INET, SOCK_STREAM, 0);  
		if(sFd == -1)
		{
			printf("ERROR::Create socket error: %s(errno: %d)\n",strerror(errno),errno);			
		}
#ifdef DEBUG
		else
		{
			printf("INFO::Server listen fd is %d\n", sFd);  
		}
#endif
				
		sockaddr_in servAddr;  
		bzero(&servAddr, sizeof(servAddr));  //memset, fill with 0
		servAddr.sin_family = AF_INET;  
		servAddr.sin_addr.s_addr = INADDR_ANY;  
		servAddr.sin_port = htons(port);  
		
		if(bind(sFd, (const sockaddr*)&servAddr, sizeof(servAddr))==-1)
		{
			printf("ERROR::Bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		}
		
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
	
	// accept new connections from clients  
	static void AcceptConn(int sFd, int events, void *arg, void *pThis)  
	{
		Listener *m_ThisPtr = (Listener *)pThis;
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
				if(m_ThisPtr->m_Events[i].m_Status == 0)
				{
					break;
				}
			}
			if(i == MAX_EVENTS)  
			{  
				printf("INFO::%s:Max connection limit[%d].", __func__, MAX_EVENTS);  
				break;  
			}
			
			if(m_ThisPtr->SetNonBlock(connFd) < 0)
			{
				//printf("%s: fcntl nonblocking failed:%d", __func__, iret);
				break;
			}
			// add a read event for receive data  
			m_ThisPtr->m_Events[i].Set(connFd, RecvData, pThis);  
			m_ThisPtr->m_Events[i].Add(m_ThisPtr->m_EpollFd, EPOLLIN);
		}while(0);  
		printf("INFO::New connecion from %s:%d,\tTime:%ld,\tIndex:%d\n", inet_ntoa(sin.sin_addr), 
				ntohs(sin.sin_port), m_ThisPtr->m_Events[i].m_LastActive, i);  
	}

	// receive data  
	static void RecvData(int sFd, int events, void *arg, void *pThis)  
	{
		Listener *m_ThisPtr=(Listener *)pThis;
		ListenerEvent *ev = (ListenerEvent*)arg; 
		// receive data
		int len = recv(sFd, ev->m_Buff+ev->m_Len, sizeof(ev->m_Buff)-1-ev->m_Len, 0);
		ev->Del(m_ThisPtr->m_EpollFd);
		if(len > 0)
		{
			ev->m_Len += len;
			ev->m_Buff[len] = '\0';
			printf("INFO::C[%d]:%s\n", sFd, ev->m_Buff);  
			// change to send event  
			ev->Set( sFd, SendData, pThis);
			ev->Add(m_ThisPtr->m_EpollFd, EPOLLOUT);  
		}  
		else if(len == 0)  
		{  
			close(ev->m_Fd);  
			printf("INFO::[fd=%d] pos[%d], closed gracefully.\n", sFd, ev-m_ThisPtr->m_Events);  
		}  
		else  
		{  
			close(ev->m_Fd);  
			printf("INFO::ecv[fd=%d] error[%d]:%s\n", sFd, errno, strerror(errno));  
		}  
	}  

	// send data  
	static void SendData(int sFd, int events, void *arg, void *pThis)  
	{
		Listener *m_ThisPtr=(Listener *)pThis;
		ListenerEvent *ev = (ListenerEvent*)arg; 
		// send data  
		int len = send(sFd, ev->m_Buff + ev->m_Offset, ev->m_Len - ev->m_Offset, 0);
		if(len > 0)  
		{
			printf("INFO::Send data, fd is %d, [%d<->%d]%s\n", sFd, len, ev->m_Len, ev->m_Buff);
			ev->m_Offset += len;
			if(ev->m_Offset == ev->m_Len)
			{
				// change to receive event
				ev->Del(m_ThisPtr->m_EpollFd);
				ev->Set(sFd, RecvData, pThis);  
				ev->Add(m_ThisPtr->m_EpollFd, EPOLLIN);  
			}
		}  
		else
		{
			close(ev->m_Fd);
			ev->Del(m_ThisPtr->m_EpollFd);
			printf("INFO::Send fd is %d, error[%d]\n", sFd, errno);
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
	printf("\n==============       Epoll Server Demo       ==============\n", port);  
    printf("INFO::Server is running on port %d\n", port); 
	
	Listener listener=Listener();	
    // create and bind socket 
    int sFd = listener.CreateAndBind(port);
    
	listener.SetNonBlock(sFd);
	listener.InitEpoll(sFd);
	
    // event loop  
    struct epoll_event epollEvents[MAX_EVENTS];
 
    int checkPos = 0;  
    while(true)
	{  
        // a simple timeout check here, every time 100, better to use a mini-heap, and add timer event  
        long now = time(NULL);  
        for(int i = 0; i < 100; i++, checkPos++) // doesn't check listen fd  
        {  
            if(checkPos == MAX_EVENTS)
			{
				checkPos = 0; // recycle
			}
			
            if(listener.m_Events[checkPos].m_Status != 1)
			{
				continue;
			}
            long duration = now - listener.m_Events[checkPos].m_LastActive;  
            if(duration >= 60) // 60s timeout  
            {
                close(listener.m_Events[checkPos].m_Fd);  
                printf("INFO::[fd=%d] timeout[%ld--%ld].\n", listener.m_Events[checkPos].m_Fd, listener.m_Events[checkPos].m_LastActive, now);
                listener.m_Events[checkPos].Del(listener.m_EpollFd);
            }  
        }  
        // wait for events to happen  
        int fds = epoll_wait(listener.m_EpollFd, epollEvents, MAX_EVENTS, 1000);  
        if(fds < 0)
		{  
            printf("ERROR::epoll_wait error, exit\n");  
            break;  
        }  
        for(int i = 0; i < fds; i++)
		{  
            ListenerEvent *ev = (ListenerEvent *)epollEvents[i].data.ptr;  
            if((epollEvents[i].events & EPOLLIN))// && (ev->m_Events&EPOLLIN)) // read event  
            {  
                ev->m_CallBack(ev->m_Fd, epollEvents[i].events, ev->m_Arg, &listener);  
            }  
            if((epollEvents[i].events & EPOLLOUT))// && (ev->m_Events&EPOLLOUT)) // write event  
            {  
                ev->m_CallBack(ev->m_Fd, epollEvents[i].events, ev->m_Arg, &listener);  
            }  
        }  
    }  
    // free resource  
    return 0;  
}   