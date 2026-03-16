/*
 * TcpPacketServer.h
 *
 *  Created on: Oct 1, 2024
 *      Author: ytim
 */

#ifndef SRC_TCPPACKETSERVER_H_
#define SRC_TCPPACKETSERVER_H_

#include "TcpClient.h"
#include "./json/json/json.h"

#pragma pack (push)
#pragma pack (1)

#define MEMBER_ENCRYPT_KEY				"1a@S"
#define MEMBER_QRCODE_ENCRYPT_KEY		"4d*F"

#define QRCODE_ENCRYPT_KEY				"1@345678901&12131415202122p32425"
#define QRCODE_ENCRYPT_IV				"@1B2c3)4e5F6&7H8"

#define MEMBER_TYPE_OWNER    0
#define MEMBER_TYPE_ADMIN    1
#define MEMBER_TYPE_MEMBER    2

#define MEMBER_STATUS_VALID    0
#define MEMBER_STATUS_JOINING  1
#define MEMBER_STATUS_READONLY  2
#define MEMBER_STATUS_DELETED  3

#define MSG_STATUS_VALID    0
#define MSG_STATUS_DELETED  1

#define MSG_TYPE_TEXT		1
#define MSG_TYPE_FILE		2

struct PACKET_HEADER {
	int size;
};

struct TCP_PACKET {
	PACKET_HEADER header;
	char data[1];
};

class CTcpPacketAccept : public CTcpAccept {
public:


	CTcpPacketAccept(const char * ip, int port, void (*onEvent)(const char *), const char * key, void * pServer, void * grpObj) {
		on_event = onEvent;
		m_svrKey = key;
		m_pServer = pServer;
		m_grpObj = grpObj;
    	m_bLogined = false;
		m_client_memberid = -1;
		m_tmConnect = time(NULL);
		//CTcpAccept(ip, port);
	}

	virtual int OnRecv(const unsigned char * pBuf, int nLen);
	virtual int SendHeartBeat();
	virtual int OnClose();
  
  bool isLogined() { return m_bLogined; }

public:
	void SendData_string(std::string & data);
	void SendData_json(Json::Value & jMsg);
	void OnRecv_clientLogin(Json::Value & jMsg);
	void OnRecv_joinGroup(Json::Value & jMsg);
	void OnRecv_reqMember(Json::Value & jMsg);
  void OnRecv_reqMessage(Json::Value & jMsg);
  void OnRecv_sendMsg(Json::Value & jMsg);
  void OnRecv_deleteMsg(Json::Value & jMsg);
  void OnRecv_addEmoji(Json::Value & jMsg);
  void OnRecv_removeEmoji(Json::Value & jMsg);

private:
	BinaryBuffer m_RecvTemp;
	void (*on_event)(const char *);
	std::string m_svrKey;
	void * m_pServer;
	void * m_grpObj;
  	bool m_bLogined;
	long long m_tmConnect;
	int m_client_memberid;
	char m_encryptKey[9];
	char m_encryptIv[9];
};

class CTcpPacketServer : public CTcpServer {
public:
	void SetServerKey(const char * key) { m_svrKey = key; }
	void setEvents(void (*onEvent)(const char *))
	  {
		  on_event = onEvent;
	  }
	void broadcast(char * data, int len);
  void broadcast(std::string & data);
  void broadcast(Json::Value & jMsg);

	void set_AES_member_key(const char * key) {
		AES_member_key = key;
	}
	std::string get_AES_member_key() {
		return AES_member_key;
	}
	void set_AES_member_iv(const char * iv) {
		AES_member_iv = iv;
	}
	std::string get_AES_member_iv() {
		return AES_member_iv;
	}

	bool DecryptClientLogin(const char * createTime, unsigned int id, const char * key);
	bool DecryptClientLogin(Json::Value & jLoginKey);
  
	void setGrpObj(void * p) {
		m_grpObj = p;
	}
	void * getGrpObj() {
		return m_grpObj;
	}

	int updateMemberCount();
	int getMemberCount() { return m_member_count; }

protected:
	virtual void OnAccept(int new_socket, const char * ip, int port);
	void (*on_event)(const char *);

private:
	std::string m_svrKey;

	std::string AES_member_key;
	std::string AES_member_iv;
  
  	void * m_grpObj;
	int m_member_count;
};

class CTcpPacketClient : public CTcpClient {
public:
	void SetServerKey(const char * svrKey, const char * clientLoginCreateTime=NULL, const char * clientLoginKey=NULL) { 
		m_svrKey = svrKey; 
		m_client_login_createTime = clientLoginCreateTime?clientLoginCreateTime:"";
		m_client_login_key = clientLoginKey?clientLoginKey:"";
		m_bLogined = false;
	}
	void setEvents(void (*onEvent)(const char *))
	  {
		  on_event = onEvent;
	  }
  void setFilePort(int port) {
    m_filePort = port;
  }
  void setGrpObj(void * p) {
    m_grpObj = p;
  }
  void SetJoinCipher(const char * cipher, Json::Value & vJoiner) {
	  m_szJoiner = vJoiner["name"].asString();
	  m_szIcon = vJoiner["icon"].toFastString();
    m_client_joinCipher = cipher;
  }
  
  void setMemberId(unsigned int id) {
    m_memberid = id;
  }
  unsigned int getMemberId() {
    return m_memberid;
  }
	void connectThread();
	void asyncConnet(const char * domain_name, int port);
	void asyncConnet();
	void stop();
  void sendMsg(const char * msg, int len);
	virtual void OnClose();
	virtual void OnConnect();
	virtual void SendHeartBeat();
	virtual int OnRecv(const unsigned char * pBuf, int nLen);

public:
	void SendData_string(std::string & data);
	void SendData_json(Json::Value & jMsg);
	void onRecv_OnClientLogin(Json::Value & jMsg);
	void onRecv_onJoinGroup(Json::Value & jMsg);
	void onRecv_addOnlineMember(Json::Value & jMsg);
	void ReqMember();
  void onRecv_onReqMember(Json::Value & jMsg);
  void onRecv_onReqMessage(Json::Value & jMsg);
  void onRecv_newMember(Json::Value & jMsg);
  void ReqMessage();
  void onRecv_onSendMsg(Json::Value & jMsg);
  void onRecv_newMsg(Json::Value & jMsg);
  void onRecv_updateMsg(Json::Value & jMsg);
  void onRecv_changeMember(Json::Value & jMsg);
  void onRecv_deleteMsgFile(Json::Value & jMsg);
  std::string uploadFile(Json::Value & jFile, int msgId);
  std::string downloadFile(Json::Value & jFile, int msgId, int idx, char* downloadFileName);

protected:
	void (*on_event)(const char *);

private:
	BinaryBuffer m_RecvTemp;

	std::string m_svrKey;
	std::string m_client_login_createTime;
	std::string m_client_login_key;
	std::string m_client_joinCipher;
	unsigned int m_memberid;
	unsigned int m_memberType;
	bool m_bLogined;
	bool m_bReadyMember;
  	bool m_bReadyMsg;

	std::string m_szJoiner;
	std::string m_szIcon;

	std::thread m_connectThread;
	//CRITICAL_SECTION m_connectLock;
	std::string m_server1;
	int m_port1;
  	int m_filePort;
	bool m_bConnecting1;
	bool m_bStopped1;
  
  	void * m_grpObj;

  	char m_encryptKey[9];
	char m_encryptIv[9];
};
#pragma pack (pop)


#endif /* SRC_TCPPACKETSERVER_H_ */
