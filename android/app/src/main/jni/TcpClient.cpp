//#include "StdAfx.h"
#include "TcpClient.h"

#include <fcntl.h>

#ifdef _WINDOWS
HANDLE * g_hClientThreadPool = NULL;
#else
std::thread * g_hClientThreadPool = NULL;
#endif

CTcpClient::CTcpClient(void)
{
	m_bRunning = false;
	m_tRecvTime = 0;
	m_tSendHeartBeat = 0;

	m_sec_keepalive_check = 15;
	m_sec_keepalive_timeout = 60;

	m_CritRecv = new CRITICAL_SECTION();
	m_CritSend = new CRITICAL_SECTION();
	InitializeCriticalSection(m_CritRecv);
	InitializeCriticalSection(m_CritSend);

	m_sk = INVALID_SOCKET;

	for (int i = 0; i < 2; i++)
	{
		m_pBuf[i] = NULL;
		m_nSize[i] = 0;
	}

	m_nStatusWrite = sock_send_valid;
}

CTcpClient::~CTcpClient(void)
{
	Disconnect();

	for (int i = 0; i < 2; i++)
	{
		if (m_nSize[i] > 0)
		{
			delete [] m_pBuf[i];
			m_pBuf[i] = NULL;
			m_nSize[i] = 0;
		}
	}

	while (m_bRunning) Sleep(3);

	DeleteCriticalSection(m_CritSend);
	delete m_CritSend;
	DeleteCriticalSection(m_CritRecv);
	delete m_CritRecv;
}

#include <vector>
class CClientThreadInfo
{
	friend class CTcpClient;
public:
	CClientThreadInfo(int i) 
	{
		InitializeCriticalSection(&m_Crit);
		idx = i; 
	}
	~CClientThreadInfo() 
	{
		DeleteCriticalSection(&m_Crit);
	}
	void AddClient(CTcpClient * p)
	{
		EnterCriticalSection(&m_Crit);
		p->SetRunning(true);
		m_Client.push_back(p);		
		LeaveCriticalSection(&m_Crit);
	}

	void HandleRecv()
	{
		EnterCriticalSection(&m_Crit);
		std::list<CTcpClient*>::iterator it = m_Client.begin();
		while (it != m_Client.end())
		{
			CTcpClient* client = (CTcpClient*)(*it);
#ifdef _DEBUG
		//printf("1 Client_Thread_Pool looping %I64d\n", time(NULL));
#endif
			client->HandleRecv();
#ifdef _DEBUG
		//printf("2 Client_Thread_Pool looping %I64d\n", time(NULL));
#endif
			client->HandleSend();
#ifdef _DEBUG
		//printf("3 Client_Thread_Pool looping %I64d\n", time(NULL));
#endif
			client->CheckRecvTime();
#ifdef _DEBUG
		//printf("4 Client_Thread_Pool looping %I64d\n", time(NULL));
#endif
			if (client->m_sk == INVALID_SOCKET)
			{
				it = m_Client.erase(it);
				client->SetRunning(false);
			}
			else
			{
				it++;
			}
		}
		LeaveCriticalSection(&m_Crit);
	}

	int idx;
	CRITICAL_SECTION m_Crit;
	std::list<CTcpClient*> m_Client;
};

bool g_ClientPool_Runing = true;
std::vector<CClientThreadInfo*> g_ClientThreadInfo;
#ifdef _WINDOWS
static void Client_Thread_Pool(void * p)
#else
void Client_Thread_Pool(void * p)
#endif
{
	CClientThreadInfo * pInfo = (CClientThreadInfo*)p;
	while (g_ClientPool_Runing)
	{
#ifdef _DEBUG
		//printf("Client_Thread_Pool looping %I64d\n", time(NULL));
#endif
		pInfo->HandleRecv();
		Sleep(1);
	}
}

void AddClient(CTcpClient * p)
{
	static int idx = 0;
	int size = g_ClientThreadInfo.size();
	g_ClientThreadInfo[idx]->AddClient(p);
	idx = (idx+1) % size;
}

bool CTcpClient::Connect(const char * domain_name, int port, std::string &err, int timeout_seconds)
{
	//thread pool
	if (g_hClientThreadPool == NULL)
	{
		err = "";
#ifdef _WINDOWS
		SYSTEM_INFO si;
		::GetSystemInfo(&si);
		int count = si.dwNumberOfProcessors;
#if _DEBUG
		count = 1;
#endif
		g_hClientThreadPool = new HANDLE[count];
		for (int i = 0; i < count; i++) g_hClientThreadPool[i] = NULL;
		for (int i = 0; i < count; i++)
		{
			CClientThreadInfo * info = new CClientThreadInfo(i);
			g_ClientThreadInfo.push_back(info);
			g_hClientThreadPool[i] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)Client_Thread_Pool,(LPVOID)info,0,0);
			if (g_hClientThreadPool[i] == NULL) 
			{
				err = "create thread pool failed";
				break;
			}
		}
#else
    int count = 5;
    g_hClientThreadPool = new std::thread[count];
    for (int i = 0; i < count; i++)
    {
      CClientThreadInfo * info = new CClientThreadInfo(i);
      g_ClientThreadInfo.push_back(info);
      g_hClientThreadPool[i] = std::thread(&Client_Thread_Pool, (void*)info);
    }
#endif
		if (err != "")
		{
			g_ClientPool_Runing = false;
			for (int i = 0; i < count; i++)
			{
#ifdef _WINDOWS
				if (g_hClientThreadPool[i])
				{
					WaitForSingleObject(g_hClientThreadPool[i],INFINITE);
					CloseHandle(g_hClientThreadPool[i]);
					g_hClientThreadPool[i] = NULL;
				}
#else
        if (g_hClientThreadPool[i].joinable()) g_hClientThreadPool[i].join();
#endif

			}
			delete [] g_hClientThreadPool;
      g_hClientThreadPool = NULL;
			return false;
		}
	}

	Disconnect();

	for (int i = 0; i < 200; i++)
	{
		if (m_bRunning) Sleep(10);
		else break;
	}
	if (m_bRunning)
	{
		err = "socket already running";
		return false;
	}

	for (int i = 0; i < 2; i++)
	{
		if (m_nSize[i] > 0)
		{
			delete [] m_pBuf[i];
			m_pBuf[i] = NULL;
			m_nSize[i] = 0;
		}
	}

	/////////////////////////////////////////
	err = "";
#ifdef _WINDOWS
	m_sk=WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
  m_sk=socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (m_sk==INVALID_SOCKET)
	{
		err = "fail to create WSASocket";
		return false;
	}

	sockaddr_in addr;
  bzero((char*)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
  
	unsigned int ip;
	if (!DomainNameToIP(domain_name,ip))
	{
		err = "unknown domain name";
		return false;
	}
	addr.sin_addr.s_addr = ip;

	m_svr_port = port;

	//non-block
#ifdef _WINDOWS
	unsigned long ul=1;
	if (ioctlsocket(m_sk, FIONBIO,(unsigned long *)&ul) == SOCKET_ERROR)//non-block mode
  {
    err = "fail to set non-block mode socket";
    CloseSocket();
    return false;
  }
#else
  /*
    if (fcntl(m_sk, F_SETFL, O_NONBLOCK) < 0)//non-block mode
    {
      err = "fail to set non-block mode socket";
      CloseSocket();
      return false;
    }
   */
#endif

	struct timeval tv;
    tv.tv_sec = timeout_seconds;  // 3 seconds timeout
    tv.tv_usec = 0;

	setsockopt(m_sk, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

	//connect
	while (1)
	{
		if(connect(m_sk, (sockaddr*)&addr, sizeof(addr))==SOCKET_ERROR)
		{
#ifdef _WINDOWS
			int r=WSAGetLastError();
			if(r==WSAEWOULDBLOCK || r==WSAEALREADY /*WSAEINVAL*/)
			{
				Sleep(20);
				continue;
			}
			else if(r==WSAEISCONN)//�׽���ԭ���Ѿ����ӣ���
			{
				break;
			}
			printf("### connect fail: %d\n", r);
#else
      int r = errno;
      if(r==EALREADY /*WSAEINVAL*/)
      {
        Sleep(20);
        continue;
      }
      else if(r==EISCONN)//�׽���ԭ���Ѿ����ӣ���
      {
        break;
      }
      printf("### connect fail: %d\n", r);
#endif
			err = "connect datasvr failed";
			CloseSocket();
			return false;
		}
		else
		{
#ifndef _WINDOWS
      int flags = fcntl(m_sk, F_GETFL);
      flags |= O_NONBLOCK;
      int ret = fcntl(m_sk, F_SETFL, flags);
      if(ret == -1){
        err = "O_NONBLOCK failed";
        CloseSocket();
        return false;
      }

#endif
			break;
		}
	}
	AddClient(this);

	//
	m_tRecvTime = time(NULL);
	m_tSendHeartBeat = m_tRecvTime;

	OnConnect();
	return true;
}

void CTcpClient::CloseSocket()
{
	m_CritRecv->lock();
	m_CritSend->lock();
	if (m_sk != INVALID_SOCKET)
	{
		closesocket(m_sk);
		m_sk = INVALID_SOCKET;			

		m_CritRecv->unlock();
		m_CritSend->unlock();
		OnClose();
		return;
	}
	m_CritRecv->unlock();
	m_CritSend->unlock();
}

void CTcpClient::Disconnect()
{
	m_CritRecv->lock();
	m_CritSend->lock();
	if (m_sk != INVALID_SOCKET)
	{
#ifdef _WINDOWS
    shutdown( m_sk, SD_BOTH );
#else
    shutdown( m_sk, SHUT_RDWR );
#endif
		CloseSocket();
	}
	m_CritRecv->unlock();
	m_CritSend->unlock();
}

inline int CTcpClient::GetSecKeepaliveCheck(void)
{
	return m_sec_keepalive_check;
}

inline void CTcpClient::SetSecKeepaliveCheck(int sec)
{
	m_sec_keepalive_check = sec;
}

inline int CTcpClient::GetSecKeepaliveTimeout(void)
{
	return m_sec_keepalive_timeout;
}

inline void CTcpClient::SetSecKeepaliveTimeout(int sec)
{
	m_sec_keepalive_timeout = sec;
}

void CTcpClient::HandleRecv()
{
	CMySafeLock safeLock(m_CritRecv);
#if 0
	fd_set readfds;

	FD_ZERO(&readfds);			
	FD_SET(m_sk, &readfds);			

	timeval timout = {0,1};
	//timeout is NULL , so wait indefinitely
	int activity = select( 0 , &(readfds) , NULL, NULL , &timout);
	if (activity  == SOCKET_ERROR) //select fail; == 0 timeout
	{
		CloseSocket();
	}
	else if (activity == 0) //timeout
	{
		return;
	}	

	if (m_sk != INVALID_SOCKET)
	{
		if (FD_ISSET( m_sk , &(readfds))) 
		{
			char buffer[1025];  //data buffer of 1K
			int valread;
			//Check if it was for closing , and also read the incoming message
			valread = recv( m_sk , buffer, 1024, 0);
			if (valread == 0)
			{
				struct sockaddr_in address;
				int addrlen = sizeof(address);
				//Somebody disconnected , get his details and print
				getpeername(m_sk , (struct sockaddr*)&address , (int*)&addrlen);
				printf("recv=0 disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                
				CloseSocket();
			}
			else if (valread < 0)	// no buffer to read
			{
			}
			//Echo back the message that came in
			else
			{
				//printf("#### onrecv: sock %d\n",m_sk);
				if (OnRecv((const unsigned char*)buffer,valread) == 0)
				{
					m_tRecvTime = time(NULL);
				}
			}
		}
	}
#else

	if (m_sk == INVALID_SOCKET) return;

	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(m_sk, &readfds);

	timeval timout = {0,100000};
	//timeout is NULL , so wait indefinitely
	int activity = select( m_sk+1 , &(readfds) , NULL, NULL , &timout);
	if (activity  == SOCKET_ERROR) //select fail; == 0 timeout
	{
		CloseSocket();
	}
	else if (activity == 0) //timeout
	{
		return;
	}

	char buffer[1025];
#ifdef _DEBUG
	//printf("1 recv start %I64d\n", time(NULL));
#endif
	int ret = recv(m_sk,buffer,1024,0);
#ifdef _DEBUG
	//printf("1 recv end %d port %d\n", ret, m_svr_port);
#endif
	if (ret == SOCKET_ERROR)
	{
#ifdef _WINDOWS
		int r = WSAGetLastError();
		if(r == WSAEWOULDBLOCK)
		{
			//std::cout<<"û���յ����������ص����ݣ���"<<std::endl;
			//Sleep(10);
			//continue;
			return;
		}
		else //if(r==WSAENETDOWN)
		{
			struct sockaddr_in address;
			int addrlen = sizeof(address);
			//Somebody disconnected , get his details and print
			getpeername(m_sk , (struct sockaddr*)&address , (int*)&addrlen);
			printf("recv lasterr=%d disconnected , ip %s , port %d \n", r, inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            
			CloseSocket();
			return;
		}
#else
    int r = errno;
    if(r == EWOULDBLOCK || r == EAGAIN)
    {
      //std::cout<<"û���յ����������ص����ݣ���"<<std::endl;
      //Sleep(10);
      //continue;
      return;
    }
    else //if(r==WSAENETDOWN)
    {
      struct sockaddr_in address;
      int addrlen = sizeof(address);
      //Somebody disconnected , get his details and print
      //getpeername(m_sk , (struct sockaddr*)&address , (int*)&addrlen);
      //printf("recv lasterr=%d disconnected , ip %s , port %d \n", r, inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            
      CloseSocket();
      return;
    }
#endif
	}
	else if (ret == 0)
	{
		struct sockaddr_in address;
		int addrlen = sizeof(address);
		//Somebody disconnected , get his details and print
		//getpeername(m_sk , (struct sockaddr*)&address , (int*)&addrlen);
		//printf("recv 0 disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
		CloseSocket();
		return;
	}
	else
	{
		if (OnRecv((const unsigned char*)buffer,ret) == 0)
		{
			m_tRecvTime = time(NULL);
		}
	}
#endif
}

void CTcpClient::HandleSend()
{
	CMySafeLock safeLock(m_CritSend);

	if (m_sk == INVALID_SOCKET) return;

	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(m_sk, &writefds);

	timeval timout = {0,1};
	//timeout is NULL , so wait indefinitely
	int activity = select( 0 , NULL , &(writefds), NULL , &timout);
	if (activity == SOCKET_ERROR) //select fail; == 0 timeout
	{
		CloseSocket();
	}
	else if (activity == 0) //timeout
	{
		return;
	}

	if (m_sk != INVALID_SOCKET)
	{
		if (FD_ISSET( m_sk , &(writefds))) 
		{
			if (m_pBuf[1])
			{
				delete m_pBuf[1];
				m_pBuf[1] = NULL;
				m_nSize[1] = 0;
			}
			if (m_nStatusWrite == sock_send_invalid)
			{
				m_nStatusWrite = sock_send_valid;
			}
		}
		else
		{
			m_nStatusWrite = sock_send_invalid;
		}
	}
}

void CTcpClient::CheckRecvTime()
{
	CMySafeLock safeLock(m_CritSend);

	if (m_sk == INVALID_SOCKET) return;

	time_t tNow = time(NULL);
	if (tNow - m_tRecvTime > m_sec_keepalive_timeout)
	{
		printf("keepalive timeout \n");
		CloseSocket();
	}

	//else if (tNow - m_tRecvTime > m_sec_keepalive_check)
	if (m_sk != INVALID_SOCKET)
	{
		if (tNow - m_tSendHeartBeat > m_sec_keepalive_check)
		{
			SendHeartBeat();
			m_tSendHeartBeat = tNow;
		}

		//sock->SetRecvTime(tNow);
	}
}

enum tcp_svr_status CTcpClient::SendData(const unsigned char * pBuf, int nLen, int nRetryTimes)
{
	CMySafeLock safeLock(m_CritSend);
	
	if (m_nStatusWrite == sock_send_invalid)
	{
		//printf("%d: StatusWrite invalid\n",this->m_nIdx);
		return sock_send_invalid;
	}

	if (!m_bRunning)
	{
		//printf("%d: client disconnect\n",this->m_nIdx);
		return sock_send_invalid;
	}

	if (m_sk == INVALID_SOCKET)
	{
		//printf("%d: INVALID_SOCKET\n",this->m_nIdx);
		return sock_send_invalid;
	}

	int count = 0;
	while (m_pBuf[1])
	{
		Sleep(10);
		if (++count == nRetryTimes) break;
	}
	if (m_pBuf[1])
	{
		//printf("%d: send buffer not empty\n",this->m_nIdx);
		return sock_send_invalid;
	}

	m_pBuf[1] = new unsigned char[nLen];
	m_nSize[1] = nLen;
	memcpy(m_pBuf[1],pBuf,nLen);
	int ret = send(m_sk, (const char *)m_pBuf[1], nLen, 0);

	printf("send ret: %d\n", ret);
	if (ret == nLen ) //successfully sent
	{
		if (m_pBuf[1])
		{
			delete m_pBuf[1];
			m_pBuf[1] = NULL;
			m_nSize[1] = 0;
		}
		m_nStatusWrite = sock_send_valid;
	}
	else if (ret >= 0 && ret < nLen) // pending
	{
		return sock_send_pending;
	}
	else if(ret==SOCKET_ERROR)
	{
#ifdef _WINDOWS
		int r=WSAGetLastError();
		if(r==WSAEWOULDBLOCK)
		{
			return sock_send_invalid;
		}
		else
		{
			printf("send error; lasterr: %d\n",r);
			CloseSocket();
			return sock_send_fail_and_closesocket;
		}
#else
    int r = errno;
    if(r==EAGAIN || r==EWOULDBLOCK)
    {
      return sock_send_invalid;
    }
    else
    {
      printf("send error; lasterr: %d\n",r);
      CloseSocket();
      return sock_send_fail_and_closesocket;
    }
#endif
	}
	else
    {
		printf("send error; ret: %d\n",ret);
		CloseSocket();
		return sock_send_fail_and_closesocket;
    }

	return sock_send_success;
}
