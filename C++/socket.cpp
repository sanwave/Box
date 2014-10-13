#inlclude<iostream>
using namespace std;

#define DEBUG

class Event  
{
public:
	int Fd;
	int Events;  
	void *Arg;  
	void (*CallBack)(int Fd, int Events, void *Arg);  
	int Status; // 1: in epoll wait list, 0 not in  
	char Buff[128]; // recv data buffer  
	int Len, Offset;  
	long LastActive; // last active time 
	
	Set(int fd, void (*call_back)(int, int, void*), void *arg)
	{
		Fd = fd;      
		events = 0;  
		Arg = arg;
		Call_back = call_back;  
		Status = 0;
		bzero(Buff, sizeof(ev->Buff)); 
		Len = 0; 
		Offset = 0;     
		LastActive = time(NULL);  
	}
	
	Add(int epollFd, int events)
	{
		struct epoll_event epv = {0, {0}};  
		int op;  
		epv.data.ptr = this;  
		epv.events = Events = events;  
		if(Status == 1){  
			op = EPOLL_CTL_MOD;  
		}  
		else{  
			op = EPOLL_CTL_ADD;  
			Status = 1;
		}  
		if(epoll_ctl(epollFd, op, Fd, &epv) < 0)  
			printf("Event Add failed[fd=%d], evnets[%d]\n", Fd, Events);  
		else  
			printf("Event Add OK[fd=%d], op=%d, evnets[%0X]\n", Fd, op, Events);  
	}
	
	Del(int epollFd)
	{
		struct epoll_event epv = {0, {0}};  
		if(Status != 1) 
			return;  
		epv.data.ptr = this;  
		Status = 0;
		epoll_ctl(epollFd, EPOLL_CTL_DEL, Fd, &epv);  
	}
};

class Listener
{
public:
	int m_EpollFd;  
	Event m_Events[MAX_EVENTS+1]; // m_Events[MAX_EVENTS] is used by listen fd  

	Listener()
	{
		m_EpollFd = -1;
		
	}
	
	// create & bind listen socket, and add to epoll, set non-blocking 
	int CreateAndBind(short port)
	{		
		int listenFd = socket(AF_INET, SOCK_STREAM, 0);  
		if(listenFd==-1)
		{
			printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
			m_EpollFd = -1;
		}
#ifdef DEBUG
		else
		{
			printf("server listen fd=%d\n", listenFd);  
		}
#endif
				
		sockaddr_in servAddr;  
		bzero(&servAddr, sizeof(servAddr));  //memset, fill with 0
		servAddr.sin_family = AF_INET;  
		servAddr.sin_addr.s_addr = INADDR_ANY;  
		servAddr.sin_port = htons(port);  
		
		if(bind(listenFd, (const sockaddr*)&servAddr, sizeof(servAddr))==-1)
		{
			printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
			m_EpollFd = -1;
		}
		
		if(listen(listenFd, -1)==-1)
		{
			printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
			m_EpollFd = -1;
		}
		
		return m_EpollFd;
	}
	
	int SetNonBlock(int sFd)
	{
		if( fcntl(listenFd, F_SETFL, O_NONBLOCK) )
		{
			perror ("fcntl");
			return -1;
		}// set non-blocking  		
		
		/*int flags, s;

		flags = fcntl (sfd, F_GETFL, 0);
		if (flags == -1)
		{
			perror ("fcntl");
			return -1;
		}

		flags |= O_NONBLOCK;
		s = fcntl (sfd, F_SETFL, flags);
		if (s == -1)
		{
			perror ("fcntl");
			return -1;
		}*/

		return 0;
		
	}
	
	int InitEpoll()
	{
		int epollFd = epoll_create(MAX_EVENTS);  
		if(fd <= 0)
		{
			printf("create epoll failed.%d\n", epollFd);  
			return epollFd;
		}		
		EventSet(&m_Events[MAX_EVENTS], listenFd, AcceptConn, &m_Events[MAX_EVENTS]);  
		// add listen socket  
		EventAdd(epollFd, EPOLLIN, &m_Events[MAX_EVENTS]);  
		// bind & listen  
	}
	
	// accept new connections from clients  
	void AcceptConn(int fd, int events, void *arg)  
	{  
		struct sockaddr_in sin;  
		socklen_t len = sizeof(struct sockaddr_in);  
		int nfd, i;  
		// accept  
		if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) == -1)  
		{
			if(errno != EAGAIN && errno != EINTR)  
			{  
			}
			printf("%s: accept, %d", __func__, errno);  
			return;
		}
		do  
		{
			for(i = 0; i < MAX_EVENTS; i++)
			{
				if(m_Events[i].Status == 0)
				{
					break;
				}
			}
			if(i == MAX_EVENTS)  
			{  
				printf("%s:max connection limit[%d].", __func__, MAX_EVENTS);  
				break;  
			}
			
			if(SetNonBlock(nfd) < 0)
			{
				printf("%s: fcntl nonblocking failed:%d", __func__, iret);
				break;
			}
			// add a read event for receive data  
			m_Events[i]->Set(&m_Events[i], nfd, RecvData);  
			m_Events[i]->Add(m_epollFd, EPOLLIN);
		}while(0);  
		printf("new conn[%s:%d][time:%ld], pos[%d]\n", inet_ntoa(sin.sin_addr), 
				ntohs(sin.sin_port), m_Events[i].LastActive, i);  
	}

	// receive data  
	void RecvData(int fd, int events, void *arg)  
	{  
		Event *ev = (Event*)arg; 
		// receive data
		int len = recv(fd, ev->Buff+ev->len, sizeof(ev->Buff)-1-ev->len, 0);
		ev->Del(m_epollFd);
		if(len > 0)
		{
			ev->len += len;
			ev->Buff[len] = '\0';
			printf("C[%d]:%s\n", fd, ev->Buff);  
			// change to send event  
			ev->Set(ev, fd, SendData);
			ev->Add(m_epollFd, EPOLLOUT);  
		}  
		else if(len == 0)  
		{  
			close(ev->fd);  
			printf("[fd=%d] pos[%d], closed gracefully.\n", fd, ev-m_Events);  
		}  
		else  
		{  
			close(ev->fd);  
			printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));  
		}  
	}  

	// send data  
	void SendData(int fd, int events, void *arg)  
	{  
		struct myevent_s *ev = (struct myevent_s*)arg; 
		// send data  
		int len = send(fd, ev->buff + ev->s_offset, ev->len - ev->s_offset, 0);
		if(len > 0)  
		{
			printf("send[fd=%d], [%d<->%d]%s\n", fd, len, ev->len, ev->buff);
			ev->s_offset += len;
			if(ev->s_offset == ev->len)
			{
				// change to receive event
				ev->Del(g_epollFd);
				ev->Set(ev, fd, RecvData);  
				ev->Add(g_epollFd, EPOLLIN);  
			}
		}  
		else
		{
			close(ev->fd);
			ev->Del(g_epollFd);
			printf("send[fd=%d] error[%d]\n", fd, errno);
		}
	}	
	
};

int main(int argc, char **argv)  
{

	/*printf("argc=%d\n",argc);
	for(int i=0;i<argc;++i)
	{
		printf("argv[%d]=%s\n",i,argv[i]);
	}*/
	
    unsigned short port = 12345; // default port  
    if(argc == 2){  
        port = atoi(argv[1]);  
    }
	
	Listener listener=Listener();	
	
    // create and bind socket 
    listener.CreateAndBind(port);  
    
    // event loop  
    struct epoll_event events[MAX_EVENTS];  
    printf("server running:port[%d]\n", port);  
    int checkPos = 0;  
    while(1){  
        // a simple timeout check here, every time 100, better to use a mini-heap, and add timer event  
        long now = time(NULL);  
        for(int i = 0; i < 100; i++, checkPos++) // doesn't check listen fd  
        {  
            if(checkPos == MAX_EVENTS)
			{
				checkPos = 0; // recycle
			}
			
            if(listener.m_Events[checkPos].Status != 1)
			{
				continue;
			}
            long duration = now - listener.m_Events[checkPos].LastActive;  
            if(duration >= 60) // 60s timeout  
            {
                close(listener.m_Events[checkPos].Fd);  
                printf("[fd=%d] timeout[%ld--%ld].\n", listener.m_Events[checkPos].fd, listener.m_Events[checkPos].last_active, now);
                EventDel(listener.m_epollFd, &listener.m_Events[checkPos]);
            }  
        }  
        // wait for events to happen  
        int fds = epoll_wait(listener.m_epollFd, events, MAX_EVENTS, 1000);  
        if(fds < 0)
		{  
            printf("epoll_wait error, exit\n");  
            break;  
        }  
        for(int i = 0; i < fds; i++)
		{  
            myevent_s *ev = (struct myevent_s*)events[i].data.ptr;  
            if((events[i].events&EPOLLIN)&&(ev->events&EPOLLIN)) // read event  
            {  
                ev->call_back(ev->fd, events[i].events, ev->arg);  
            }  
            if((events[i].events&EPOLLOUT)&&(ev->events&EPOLLOUT)) // write event  
            {  
                ev->call_back(ev->fd, events[i].events, ev->arg);  
            }  
        }  
    }  
    // free resource  
    return 0;  
}   