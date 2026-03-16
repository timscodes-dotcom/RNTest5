#pragma once

#ifdef _WINDOWS

#include <winsock2.h>
#include <string>
#include <list>

#else

#include <string>
#include <list>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <netdb.h>

#include <cstring>

#include "CRITICAL_SECTION.h"

#ifndef INVALID_SOCKET
  #define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
  #define SOCKET_ERROR   (-1)
#endif
#define closesocket(s) ::close(s)
#define WSAGetLastError() errno
#define Sleep(s) usleep(s*1000)
#include <errno.h>

typedef int SOCKET;
typedef sockaddr_in SOCKADDR_IN;
typedef hostent *LPHOSTENT;
typedef in_addr *LPIN_ADDR;

//typedef unsigned char byte;

#endif

//http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
enum tcp_svr_status
{
	tcp_svr_none		=0,
	tcp_svr_started,
	tcp_svr_fail,
	tcp_svr_fail_select,
	tcp_svr_fail_accept,

	tcp_client_accept	=100,
	tcp_client_connect,
	tcp_client_closing,
	tcp_client_closed,

	sock_send_valid		=200,
	sock_send_invalid,
	sock_send_success,
	sock_send_pending,
	sock_send_fail_and_closesocket,	
};

#ifdef _WINDOWS
extern "C" _declspec(dllexport) int WINSOCK_API_Init();
extern "C" _declspec(dllexport) unsigned char * GetLocalIPAddress();		//need to release byte * outside
extern "C" _declspec(dllexport) bool DomainNameToIP(const char * strHost, unsigned int &nIP);
#else
int WINSOCK_API_Init();
std::string GetLocalIPAddress();
bool DomainNameToIP(const char * strHost, unsigned int &nIP);
#endif

class CTcpServer;

class BinaryBuffer
{
public:
	BinaryBuffer() 
	{
		m_Buf = NULL;
		m_Size = 0;
		m_Offset = 0;
		InitializeCriticalSection(&m_Lock);
	}
	~BinaryBuffer()
	{
		EnterCriticalSection(&m_Lock);
		if (m_Buf) delete [] m_Buf;
		LeaveCriticalSection(&m_Lock);

		DeleteCriticalSection(&m_Lock);
	}

	int Write(char * buf, int size)
	{
		EnterCriticalSection(&m_Lock);
		char * temp = new char[size + m_Size];
		if (m_Size > 0) memcpy(temp, m_Buf, m_Size);
		memcpy(temp+m_Size, buf, size);
		m_Size += size;
		if (m_Buf) delete [] m_Buf;
		m_Buf = temp;
		LeaveCriticalSection(&m_Lock);

		return m_Size - m_Offset;
	}

	int Read(char * buf, int size, bool remove = true)
	{
		EnterCriticalSection(&m_Lock);
		int size_left = m_Size - m_Offset;
		int size_output = size_left > size ? size : size_left;
		memcpy(buf, m_Buf+m_Offset, size_output);
		if (remove == true)
		{
			m_Offset += size;
			if (m_Offset == m_Size)
			{
				delete [] m_Buf;
				m_Buf = NULL;
				m_Size = 0;
				m_Offset = 0;
			}
		}
		LeaveCriticalSection(&m_Lock);
		return size_output;
	}

	int GetSize() { return m_Size - m_Offset; }

private:
	CRITICAL_SECTION m_Lock;
	char * m_Buf;
	int m_Size;
	int m_Offset;
};

#ifdef _WINDOWS
class __declspec(dllexport) CTcpAccept
#else
class CTcpAccept
#endif
{
public:

	friend class CTcpServer;

	///////////////////////////////////////////////////////////	
	//pBuf pointer will be deleted automatically after OnRecv. Dont pass the pointer out of the function
	virtual int OnRecv(const unsigned char * pBuf, int nLen);
	
	virtual int OnClose()
	{
		printf("CTcpAccept onClose\n");
		m_tReleaseTime = time(NULL) + 60;
		return 0;
	}

	virtual int SendHeartBeat();

	CTcpAccept();
	CTcpAccept(const char * ip, int port);
	~CTcpAccept(void);
	bool Init(CTcpServer * svr, int socket=INVALID_SOCKET, tcp_svr_status status=tcp_client_closed, int idx=0);
	bool Stop();
	void CloseSocket();
	virtual enum tcp_svr_status SendData(const unsigned char * pBuf, int nLen, int nRetryTimes = 10);

	inline tcp_svr_status GetStatus() { return m_nStatus; }
	inline void SetStatus(tcp_svr_status val) { m_nStatus = val; }
	inline tcp_svr_status GetStatusWrite() { return m_nStatusWrite; }

	inline void SetSocket(int socket);
	inline int GetSocket() { return m_socket; }

	inline void SetRecvTime(time_t t) { m_tRecvTime = t; }
	inline time_t GetRecvTime() { return m_tRecvTime; }
	inline void SetKeepaliveTime(time_t t) { m_tKeepaliveTime = t; }
	inline time_t GetKeepaliveTime() { return m_tKeepaliveTime; }	

	inline bool IsTaken() { return m_bTaken; }
	inline void SetTaken(bool b) { m_bTaken = b; }

	inline CTcpServer * GetServerPtr() { return m_Svr; }
	inline time_t GetReleaseTime() { return m_tReleaseTime; }

	inline const char * GetIp() { return (const char*)m_ip; }
	inline int GetPort() { return m_port; }

	void SetIpAddress(const char * ip, int port)
	{
		strncpy(m_ip, ip, sizeof(m_ip));
		m_port = port;
	}

private:
	void HandleSend();
	void HandleRecv();

protected:
	CTcpServer * m_Svr;
	char m_ip[32];
	int m_port;

	int m_nIdx;
	int m_socket;
	int m_bRun;
	enum tcp_svr_status m_nStatus;
	time_t m_tRecvTime;
	time_t m_tKeepaliveTime;
	time_t m_tReleaseTime;

	CRITICAL_SECTION * m_CritRecv;
	CRITICAL_SECTION * m_CritSend;
	unsigned char *m_pBuf[2];
	int m_nSize[2];

	bool m_bTaken;

	enum tcp_svr_status m_nStatusWrite;
};

#ifdef _WINDOWS
class __declspec(dllexport) CTcpServer
#else
class CTcpServer
#endif
{
public:
	CTcpServer(void);
	~CTcpServer(void);

	///////////////////////////////////////////////////////////	
	virtual void Init();

	virtual bool Start(int PORT, int thread_num, std::string & err);
	bool Stop();
	void CloseClient(int ClientSocket, int SockIdx);
	enum tcp_svr_status SendData(int ClientSocket, const unsigned char * pBuf, int nLen, int SockIdx);

#ifdef _WINDOWS
	static void Thread_Listening(void * p);	
	static void Thread_Recv(void * p);
#else
  void Thread_Listening();
  void Thread_Recv();
#endif

	enum tcp_svr_status GetStatus();
	int GetMasterSocket();
	int GetMaxClientNum();
	int GetClientSocket(int idx);

	int GetSecKeepaliveCheck();
	void SetSecKeepaliveCheck(int sec);
	int GetSecKeepaliveTimeout();
	void SetSecKeepaliveTimeout(int sec);

protected:
	virtual void OnAccept(int new_socket, const char * ip, int port)
	{
		CTcpAccept * sock = new CTcpAccept(ip, port);
		AddClientSocket(new_socket, sock);
	}
	virtual void DeleleteClient(void * sock)
	{
		CTcpAccept * p = (CTcpAccept *) sock;
		delete p;
	}
	void AddClientSocket(int new_socket, CTcpAccept * sock);
private:
	void SetStatus(enum tcp_svr_status status);

	CTcpAccept * GetNextAcceptSocket();
	void SetTaken(CTcpAccept * sock, bool b);

protected:
	std::list<CTcpAccept *> m_pAcceptSocket;
	std::list<CTcpAccept *>::iterator m_AcceptIt;

	int m_nOnlineCount;
	CRITICAL_SECTION * m_CritCloseSocket;

private:
	int m_max_clients;
	int m_thread_num;	
	int m_master_socket;
	enum tcp_svr_status m_status;

#ifdef _WINDOWS
	HANDLE m_hListen;	
	HANDLE * m_hRecv;
#else
  std::thread m_hListen;
  std::thread * m_hRecv;
#endif

	int m_sec_keepalive_check;
	int m_sec_keepalive_timeout;
};

class CTcpChatServer : public CTcpServer
{
public:
  CTcpChatServer() {}
  ~CTcpChatServer() {}
  void setEvents(void (*onEvent)(const char *))
  {
      on_event = onEvent;
  }
  
  virtual void OnAccept(int new_socket, const char * ip, int port)
  {
    CTcpAccept * sock = new CTcpAccept(ip, port);
    AddClientSocket(new_socket, sock);
    
    if (on_event) on_event("{\"svr\":\"svr1\",\"evt\":\"onAccept\"}");
  }
  
protected:
    void (*on_event)(const char *);
};
