#pragma once

#include "TcpServer.h"

class CClientThreadInfo;

#ifdef _WINDOWS
class __declspec(dllexport) CTcpClient
#else
class CTcpClient
#endif
{
	friend class CClientThreadInfo;

public:
	CTcpClient(void);
	~CTcpClient(void);

	bool Connect(const char * domain_name, int port, std::string &err, int timeout_seconds = 3);
	void Disconnect();

	inline bool IsRunning() { return m_bRunning; }
	inline SOCKET GetSocket() { return m_sk; }
	inline time_t GetRecvTime() { return m_tRecvTime; }
	int GetSecKeepaliveCheck();
	void SetSecKeepaliveCheck(int sec);
	int GetSecKeepaliveTimeout();
	void SetSecKeepaliveTimeout(int sec);

	virtual void OnConnect() {}
	virtual void SendHeartBeat() {}
	virtual void OnClose() { printf("onClose\n"); }
	virtual int OnRecv(const unsigned char * pBuf, int nLen) { return 0; } //return 0 means suc and will update recv time

	void CloseSocket();

	void SetRunning(bool b) { m_bRunning = b; }

	enum tcp_svr_status SendData(const unsigned char * pBuf, int nLen, int nRetryTimes = 10);

protected:
	void HandleRecv();
	void HandleSend();
	void CheckRecvTime();

protected:
	SOCKET m_sk;
	bool m_bRunning;

	CRITICAL_SECTION * m_CritSend;
	CRITICAL_SECTION * m_CritRecv;
	unsigned char *m_pBuf[2];
	int m_nSize[2];

	enum tcp_svr_status m_nStatusWrite;

	time_t m_tRecvTime;
	time_t m_tSendHeartBeat;
	int m_sec_keepalive_check;
	int m_sec_keepalive_timeout;

	int m_svr_port;
};

class CTcpChatClient : public CTcpClient
{
public:
  CTcpChatClient() {}
  ~CTcpChatClient() {}
  void setEvents(void (*onEvent)(const char *))
  {
      on_event = onEvent;
  }
  
private:
    void (*on_event)(const char *);
};
