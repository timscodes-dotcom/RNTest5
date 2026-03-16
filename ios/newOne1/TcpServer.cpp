//#include "StdAfx.h"
#include "TcpServer.h"
#include "json/json/json.h"

#include "key.h"

bool DomainNameToIP(const char * strHost, unsigned int &nIP)
{
	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(strHost);

	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		LPHOSTENT lphost;
		lphost = gethostbyname(strHost);
		if (lphost != NULL)
			sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			return false;
		}
	}

	//nIP = ntohl(sockAddr.sin_addr.s_addr);
	nIP = sockAddr.sin_addr.s_addr;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

CTcpAccept::CTcpAccept()
{
	strncpy(m_ip, "", sizeof(m_ip));
	m_port = 0;
}

CTcpAccept::CTcpAccept(const char * ip, int port)
{
	SetIpAddress(ip, port);
}

CTcpAccept::~CTcpAccept(void)
{
	Stop();
	DeleteCriticalSection(m_CritRecv);
	delete m_CritRecv;
	DeleteCriticalSection(m_CritSend);
	delete m_CritSend;
}

enum tcp_svr_status CTcpAccept::SendData(const unsigned char * pBuf, int nLen, int nRetryTimes)
{
	if (m_nStatusWrite == sock_send_invalid)
	{
		printf("%d: StatusWrite invalid\n",this->m_nIdx);
		return sock_send_invalid;
	}

	if (m_nStatus != tcp_client_connect && m_nStatus != tcp_client_accept)
	{
		printf("%d: client disconnect\n",this->m_nIdx);
		return sock_send_invalid;
	}

	if (m_socket == INVALID_SOCKET)
	{
		printf("%d: INVALID_SOCKET\n",this->m_nIdx);
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
		printf("%d: send buffer not empty\n",this->m_nIdx);
		return sock_send_invalid;
	}

	EnterCriticalSection(m_CritSend);
	m_pBuf[1] = new unsigned char[nLen];
	m_nSize[1] = nLen;
	memcpy(m_pBuf[1],pBuf,nLen);
	int ret = (int)send(m_socket, (const char *)m_pBuf[1], nLen, 0);
	LeaveCriticalSection(m_CritSend);

	if (ret == nLen ) //successfully sent
	{
		EnterCriticalSection(m_CritSend);
		if (m_pBuf[1])
		{
			delete m_pBuf[1];
			m_pBuf[1] = NULL;
			m_nSize[1] = 0;
		}
		m_nStatusWrite = sock_send_valid;
#ifdef _DEBUG
		//printf("sent (size: %d) \n", nLen);
#endif
		LeaveCriticalSection(m_CritSend);
	}
	else if (ret >= 0 && ret < nLen) // pending
	{
#ifdef _DEBUG
		//printf("### send pending (size: %d/%d) \n", ret,nLen);
#endif
		return sock_send_pending;
	}
	else
    {
		printf("send error; size: %d\n",nLen);
		this->CloseSocket();
		return sock_send_fail_and_closesocket;
    }

	return sock_send_success;
}

bool CTcpAccept::Stop()
{
	CloseSocket();
	m_bRun = false;
	for (int i = 0; i < 2; i++)
	{
		if (m_nSize[i] > 0)
		{
			delete [] m_pBuf[i];
			m_pBuf[i] = NULL;
			m_nSize[i] = 0;
		}
	}
	return true;
}

bool CTcpAccept::Init(CTcpServer * svr, int socket, tcp_svr_status status, int idx)
{
	m_socket = socket;
	
	m_CritRecv = new CRITICAL_SECTION();
	m_CritSend = new CRITICAL_SECTION();
	InitializeCriticalSection(m_CritRecv);
	InitializeCriticalSection(m_CritSend);
	for (int i = 0; i < 2; i++)
	{
		m_pBuf[i] = NULL;
		m_nSize[i] = 0;
	}

	m_Svr = svr;
	m_nIdx = idx;
	m_bRun = true;
	m_nStatus = status;
	m_nStatusWrite = sock_send_valid;

	m_bTaken = false;
	m_tRecvTime = time(NULL);
	m_tKeepaliveTime = time(NULL);
	m_tReleaseTime = 0;

	return true;
}

void CTcpAccept::HandleRecv()
{
	if (m_socket == INVALID_SOCKET) return;
	fd_set readfds;

	FD_ZERO(&readfds);			
	FD_SET(m_socket, &readfds);			

	timeval timout = {0,100000};
	//timeout is NULL , so wait indefinitely
	int activity = select( m_socket+1 , &(readfds) , NULL, NULL , &timout);
	if (activity  == SOCKET_ERROR) //select fail; == 0 timeout
	{
		CloseSocket();
	}
	else if (activity == 0) //timeout
	{
		return;
	}


	if (m_socket != INVALID_SOCKET)
	{
		if (FD_ISSET( m_socket , &(readfds))) 
		{
			printf("handle recv\n");
			char buffer[1025];  //data buffer of 1K
			int valread;
			//Check if it was for closing , and also read the incoming message
			valread = (int)recv( m_socket , buffer, 1024, 0);
			if (valread == 0)
			{
				struct sockaddr_in address;
        socklen_t addrlen = sizeof(address);
				//Somebody disconnected , get his details and print
				getpeername(m_socket , (struct sockaddr*)&address , &addrlen);
				printf("recv=0 disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                
				//closesocket( m_socket );
				CloseSocket();
			}
			else if (valread < 0)	// no buffer to read
			{
			}
			//Echo back the message that came in
			else
			{
				printf("#### onrecv: sock %d\n",m_socket);
				if (OnRecv((const unsigned char*)buffer,valread) == 0)
				{
					SetRecvTime(time(NULL));
				}
			}
		}
	}
	else {
		printf("invalid client sock\n");
	}
}

void CTcpAccept::HandleSend()
{
	if (m_socket == INVALID_SOCKET) return;
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(m_socket, &writefds);

	timeval timout = {0,100};
	//timeout is NULL , so wait indefinitely
	int activity = select( m_socket+1 , NULL , &(writefds), NULL , &timout);
	if (activity == SOCKET_ERROR) //select fail; == 0 timeout
	{
		CloseSocket();
	}
	else if (activity == 0) //timeout
	{
		return;
	}

	if (m_socket != INVALID_SOCKET)
	{
		if (FD_ISSET( m_socket , &(writefds))) 
		{
			EnterCriticalSection(m_CritSend);
			if (m_pBuf[1])
			{
				delete m_pBuf[1];
				m_pBuf[1] = NULL;
				m_nSize[1] = 0;
			}
			if (m_nStatusWrite == sock_send_invalid)
			{
				printf("%d can sent now\n", m_nIdx);
				m_nStatusWrite = sock_send_valid;
			}
			LeaveCriticalSection(m_CritSend);
		}
		else
		{
			m_nStatusWrite = sock_send_invalid;
		}
	}
}

void CTcpServer::SetTaken(CTcpAccept * sock, bool b)
{
	EnterCriticalSection(m_CritCloseSocket);
	sock->SetTaken(b);
	LeaveCriticalSection(m_CritCloseSocket);
}

#ifdef _WINDOWS
void CTcpServer::Thread_Recv(void * p)
{
	CTcpServer * pThis = (CTcpServer*)p;
#else
void CTcpServer::Thread_Recv()
{
  CTcpServer * pThis = this;
#endif
	CTcpAccept * sock = NULL;
	while (pThis->GetStatus() == tcp_svr_started)
	{
		if (sock)
		{
			pThis->SetTaken(sock, false);
		}

		sock = pThis->GetNextAcceptSocket();
		if (sock == NULL)
		{
			Sleep(1);
			continue;
		}

		if (sock->GetStatus() == tcp_client_closing)
		{
			sock->OnClose();
			sock->SetStatus(tcp_client_closed);
		}/*
		else if (sock->GetStatus() == tcp_client_accept)
		{
			sock->SetStatus(tcp_client_connect);
			sock->OnAccept();			
		}*/

		if (sock->GetSocket() != INVALID_SOCKET)
		{
			sock->HandleRecv();
			sock->HandleSend();

			time_t tNow = time(NULL);
			if (tNow - sock->GetRecvTime() > sock->GetServerPtr()->GetSecKeepaliveTimeout())
			{
				struct sockaddr_in address;
        socklen_t addrlen = sizeof(address);
				//Somebody disconnected , get his details and print
				getpeername(sock->GetSocket() , (struct sockaddr*)&address , &addrlen);
#ifdef _WINDOWS
				printf("keepalive timeout , ip %s , port %d ��%I64d/%d)\n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), sock->GetRecvTime(), sock->GetServerPtr()->GetSecKeepaliveTimeout());
#else
				printf("keepalive timeout , ip %s , port %d %lld/%d)\n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), sock->GetRecvTime(), sock->GetServerPtr()->GetSecKeepaliveTimeout());
#endif
				//closesocket( sock->GetSocket() );
				sock->CloseSocket();
			}
			else if (tNow - sock->GetKeepaliveTime() > sock->GetServerPtr()->GetSecKeepaliveCheck())
			{
				sock->SendHeartBeat();
				sock->SetKeepaliveTime(tNow);
			}
		}

		Sleep(1);
	}
}

void CTcpAccept::SetSocket(int socket)
{
	EnterCriticalSection(m_CritRecv);
	m_socket = socket;
	m_nStatus = tcp_client_accept;
	m_nStatusWrite = sock_send_valid;
	m_tRecvTime = time(NULL);
	m_tKeepaliveTime = time(NULL);
	LeaveCriticalSection(m_CritRecv);	
}

void CTcpAccept::CloseSocket()
{
	EnterCriticalSection(m_CritRecv);
	if (m_socket != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		shutdown( m_socket, SD_BOTH );
#else
    shutdown( m_socket, SHUT_RDWR );
#endif
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
		m_nStatus = tcp_client_closing;
		m_nStatusWrite = sock_send_invalid;
	}
	//OnClose();
	LeaveCriticalSection(m_CritRecv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

CTcpServer::CTcpServer()
{
	m_thread_num = 0;
  m_hRecv = NULL;
  m_CritCloseSocket = NULL;
}

CTcpServer::~CTcpServer(void)
{
	Stop();	
	if (m_hRecv) delete [] m_hRecv;
  if (m_CritCloseSocket) {
    DeleteCriticalSection(m_CritCloseSocket);
    delete m_CritCloseSocket;
  }
}

inline int CTcpServer::GetSecKeepaliveCheck(void)
{
	return m_sec_keepalive_check;
}

inline void CTcpServer::SetSecKeepaliveCheck(int sec)
{
	m_sec_keepalive_check = sec;
}

int CTcpAccept::SendHeartBeat()
{
	return 0;
}

inline int CTcpServer::GetSecKeepaliveTimeout(void)
{
	return m_sec_keepalive_timeout;
}

inline void CTcpServer::SetSecKeepaliveTimeout(int sec)
{
	m_sec_keepalive_timeout = sec;
}

void CTcpServer::Init()
{
	m_thread_num = 0;
	m_status = tcp_svr_none;

#ifdef _WINDOWS
	m_hListen = NULL;
#endif
  m_hRecv  = NULL;
  
	m_CritCloseSocket = new CRITICAL_SECTION();
	InitializeCriticalSection(m_CritCloseSocket);

	m_sec_keepalive_check	= 2;
	m_sec_keepalive_timeout	= 120;

	m_max_clients = 10000;
}

bool CTcpServer::Start(int PORT, int thread_num, std::string & err)
{
	err = "";
	if (thread_num <= 0) return false;
	//Stop();
	
	Init();
  m_thread_num = thread_num;
	/*
	for (int i = 0; i < max_clients; i++)
	{
		if (!m_pAcceptSocket[i].Start(this,i)) 
		{		
			return false;
		}
	}
	*/
	

	//create a master socket
    if( (m_master_socket = socket(AF_INET , SOCK_STREAM , 0)) == INVALID_SOCKET) 
    {
        err = "create master socket failed";
        return false;
    }

	int opt = 1;
	//set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(m_master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        err = "setsockopt failed";
        return false;
    }

	struct sockaddr_in address;
	//type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

	//bind the socket to localhost port 8888
    if (bind(m_master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        err = "bind failed";
        return false;
    }
    printf("Listener on port %d \n", PORT);

	//try to specify maximum of 3 pending connections for the master socket
    if (listen(m_master_socket, 3) < 0)
    {
        err = "listen failed";
        return false;
    }

	//accept the incoming connection
    int addrlen = sizeof(address);
    printf("Waiting for connections ...\n");
	m_status = tcp_svr_started;

#ifdef _WINDOWS
	m_hListen = CreateThread(0,0,(LPTHREAD_START_ROUTINE)Thread_Listening,(LPVOID)this,0,0);
	//SetThreadPriority(m_hClose,THREAD_PRIORITY_BELOW_NORMAL);
	if (!m_hListen) 
	{
		err = "create listen thread failed";
		return false;
	}
#else
  m_hListen = std::thread(&CTcpServer::Thread_Listening, this);
#endif

	m_pAcceptSocket.clear();
	m_pAcceptSocket.push_back(NULL);
	m_AcceptIt = m_pAcceptSocket.begin();

	if (m_hRecv) delete [] m_hRecv;
#ifdef _WINDOWS
	m_hRecv = new HANDLE[thread_num];
	//memset(m_hRecv, 0, sizeof(HANDEL)*thread_num);
	for (int i = 0; i < thread_num; i++) m_hRecv[i] = NULL;
	for (int i = 0; i < thread_num; i++)
	{
		m_hRecv[i] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)Thread_Recv,(LPVOID)this,0,0);
		if (m_hRecv[i] == NULL) 
		{
			err = "create recv thread failed";
			return false;
		}
	}
#else
  m_hRecv = new std::thread[thread_num];
  //for (int i = 0; i < thread_num; i++) m_hRecv[i] = NULL;
  for (int i = 0; i < thread_num; i++)
  {
    m_hRecv[i] = std::thread(&CTcpServer::Thread_Recv, this);
  }
#endif

	return true;
}

CTcpAccept * CTcpServer::GetNextAcceptSocket()
{
	time_t tNow = time(NULL);
	CTcpAccept * sock = NULL;
	EnterCriticalSection(m_CritCloseSocket);
	while (1)
	{
		if (m_AcceptIt == m_pAcceptSocket.end()) m_AcceptIt = m_pAcceptSocket.begin();
		sock = (CTcpAccept *)(*m_AcceptIt);		
		if (sock) 
		{
			if (sock->IsTaken())
			{
				LeaveCriticalSection(m_CritCloseSocket);
				return NULL;
			}
			if (sock->GetStatus() == tcp_client_closed && tNow > sock->GetReleaseTime())
			{
				m_AcceptIt = m_pAcceptSocket.erase(m_AcceptIt);
				printf("delete one socket\n");
				DeleleteClient((void*)sock);
				continue;
			}
			sock->SetTaken(true);
		}
		m_AcceptIt ++;
		break;
	}
	LeaveCriticalSection(m_CritCloseSocket);	
	return sock;
}

bool CTcpServer::Stop()
{
	m_status = tcp_svr_none;

#ifdef _WINDOWS
	if (m_hListen)
	{
		WaitForSingleObject(m_hListen,INFINITE);
		CloseHandle(m_hListen);
		m_hListen = NULL;
	}
#else
  if (m_hListen.joinable()) m_hListen.join();
#endif

#ifdef _WINDOWS
	for (int i = 0; i < m_thread_num; i++)
	{
		if (m_hRecv[i]) 
		{
			WaitForSingleObject(m_hRecv[i],INFINITE);
			CloseHandle(m_hRecv[i]);
			m_hRecv[i] = NULL;
		}
	}
#else
  for (int i = 0; i < m_thread_num; i++)
  {
    if (m_hRecv[i].joinable()) m_hRecv[i].join();
  }
#endif
  
  m_thread_num = 0;
  
  if (m_CritCloseSocket) {
    EnterCriticalSection(m_CritCloseSocket);
    std::list<CTcpAccept *>::iterator it;
    for (it = m_pAcceptSocket.begin(); it != m_pAcceptSocket.end(); it++)
    {
      CTcpAccept * sock = (CTcpAccept *)(*it);
      if (sock)
      {
        sock->Stop();
        delete sock;
      }
    }
    m_pAcceptSocket.clear();

    closesocket(m_master_socket);
    
    LeaveCriticalSection(m_CritCloseSocket);
  }

	return true;
}

#ifdef _WINDOWS
void CTcpServer::Thread_Listening(void * p)
{
	CTcpServer * pThis = (CTcpServer*)p;
#else
void CTcpServer::Thread_Listening()
{
  CTcpServer * pThis = this;
#endif
	char buffer[1025];  //data buffer of 1K
      
    //set of socket descriptors
    fd_set readfds;

	int master_socket = pThis->GetMasterSocket();
	int max_sd;
	int max_clients = pThis->GetMaxClientNum();	
	int new_socket , activity, valread;
	struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
    
	pThis->m_nOnlineCount = 0;
	int tickcount = 0;
	while (pThis->GetStatus() == tcp_svr_started)
	{
		//clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

		timeval timout = {1,0};
		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
#ifdef _WINDOWS
        activity = select( /*max_sd + 1*/0 , &readfds , NULL , NULL , &timout);
#else
    activity = select( max_sd + 1 , &readfds , NULL , NULL , &timout);
#endif
    
		if ((activity == SOCKET_ERROR)/* && (errno!=EINTR)*/) 
        {
			int err_code = WSAGetLastError();
			printf("select failed[%d]\n", err_code);
			pThis->SetStatus(tcp_svr_fail_select);
			break;
        }
		else if (activity == 0) //time limit expired
		{
			Sleep(10);
			continue;
		}

		//If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                printf("accept failed\n");
				pThis->SetStatus(tcp_svr_fail_accept);
				break;
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
			/*
			char message[32] = "accept";
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            {
                perror("send");
            }
			*/
              
            //puts("Welcome message sent successfully");
            pThis->OnAccept(new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        }

		Sleep(10);
	}
}

void CTcpServer::CloseClient(int ClientSocket, int SockIdx)
{
	struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
	//Somebody disconnected , get his details and print
    getpeername(ClientSocket , (struct sockaddr*)&address , (socklen_t*)&addrlen);
    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

	//Close the socket and mark as 0 in list for reuse
    closesocket( ClientSocket );
}

/*
int CTcpAccept::OnClose()
{
	m_tReleaseTime = time(NULL) + 60;
	return 0;
}
*/

int CTcpAccept::OnRecv(const unsigned char * pBuf, int nLen)
{
	//test
	SendData(pBuf,nLen);

	return 0;
}

void CTcpServer::AddClientSocket(int new_socket, CTcpAccept * sock)
{
	sock->Init(this, new_socket, tcp_client_accept);
	//sock->SetSocket(new_socket);

	EnterCriticalSection(m_CritCloseSocket);
	m_pAcceptSocket.push_back(sock);
	m_nOnlineCount ++;
	LeaveCriticalSection(m_CritCloseSocket);
}

int CTcpServer::GetClientSocket(int idx) 
{ 
	//if (idx >= 0 && idx < m_thread_num) return m_pAcceptSocket[idx].GetSocket();
	return -1;
}

enum tcp_svr_status CTcpServer::GetStatus() 
{ 
	return m_status; 
}

void CTcpServer::SetStatus(enum tcp_svr_status status)
{
	m_status = status;
}

int CTcpServer::GetMasterSocket() 
{ 
	return m_master_socket; 
}

int CTcpServer::GetMaxClientNum() 
{ 
	return m_max_clients; 
}	

enum tcp_svr_status CTcpServer::SendData(int ClientSocket, const unsigned char * pBuf, int nLen, int SockIdx)
{
	return tcp_svr_none;//m_pAcceptSocket[SockIdx].SendData(pBuf,nLen);	
}

#ifdef _WINDOWS
extern "C" _declspec(dllexport) int WINSOCK_API_Init()
{
	int nErr;
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2,2);
	nErr = WSAStartup(wVersionRequested, &wsaData );

	if (nErr != 0)
	{
		printf("Failed to init WSA\n");
	}
	return nErr;
}
#else
int WINSOCK_API_Init()
  {
    return 0;
  }
#endif

#ifdef ANDROID_DEV

#ifdef _WINDOWS
extern "C" _declspec(dllexport) unsigned char * GetLocalIPAddress()
#else
std::string GetLocalIPAddress()
#endif
{
	char szHostName[128];
	int nErr;

	strcpy(szHostName,"localhost");
  /*
	if (gethostname(szHostName,128) != 0)
	{
		nErr = WSAGetLastError();
		return NULL;
	}
   */

  Json::Value jVal;
	hostent * myhost = gethostbyname(szHostName);
	if (myhost == NULL)
	{
		//nErr = WSAGetLastError();
		//return NULL;
    return "";
	}
  jVal["officalName"] = myhost->h_name;
  jVal["ipAddress"] = inet_ntoa(*(struct in_addr*)myhost->h_addr);
  struct in_addr ** addr_list = (struct in_addr **)myhost->h_addr_list;
  Json::Value ip_array;
  for(int i = 0; addr_list[i] != NULL; i++) {
    ip_array.append(inet_ntoa(*addr_list[i]));
  }
  jVal["allAddress"] = ip_array;
/*
	sockaddr_in HostAddr;
	memcpy(&(HostAddr.sin_addr.s_addr),myhost->h_addr_list[0],myhost->h_length);

  HostAddr.sin_addr.s_addr
	unsigned char * str = new unsigned char[4];
	str[0] = HostAddr.sin_addr.S_un.S_un_b.s_b1;
	str[1] = HostAddr.sin_addr.S_un.S_un_b.s_b2;
	str[2] = HostAddr.sin_addr.S_un.S_un_b.s_b3;
	str[3] = HostAddr.sin_addr.S_un.S_un_b.s_b4;
	return str;
 */
  return jVal.toFastString();
}

#else

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <cstring>
  
std::string GetLocalIPAddress()
{
	struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *addr = nullptr;
    void *tmpAddrPtr = nullptr;

	Json::Value jVal;
	Json::Value ip_array;
    if (getifaddrs(&interfaces) == -1) {
        //std::cerr << "Failed to get network interfaces" << std::endl;
		jVal["ret"] = "Failed to get network interfaces";
		jVal["allAddress"] = ip_array;
        return jVal.toFastString();
    }

	jVal["ret"] = "ok";
	jVal["officalName"] = "";
	jVal["ipAddress"] = "";
    for (addr = interfaces; addr != nullptr; addr = addr->ifa_next) {
        if (addr->ifa_addr->sa_family == AF_INET) { // Check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)addr->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //std::cout << "Interface: " << addr->ifa_name << " IP Address: " << addressBuffer << std::endl;
			ip_array.append(addressBuffer);
        }
    }
	jVal["allAddress"] = ip_array;

    if (interfaces) {
        freeifaddrs(interfaces);
        interfaces = nullptr;
    }

	return jVal.toFastString();
}
#endif
