/*
 * TcpPacketServer.cpp
 *
 *  Created on: Oct 1, 2024
 *      Author: ytim
 */

#include <sstream>
#include "TcpPacketServer.h"
#include "./json/json/json.h"
#include "AES/AESCrypt.h"
#include "functions.h"

#define PING_MSG_HEADER "{\"cmd\":\"ping\", \"time\":\""

CRITICAL_SECTION1 m_connectLock;

int CTcpPacketAccept::SendHeartBeat() {
	std::stringstream msg;
	msg << PING_MSG_HEADER << time(NULL) << "\"}";
	std::string data = msg.str();
	SendData_string(data);
	return 0;
}

int CTcpPacketServer::updateMemberCount() {
	CGroupObj* grpObj = (CGroupObj*)m_grpObj;
	std::string memberCount = grpObj->GetOneValue("select count(1) from t_member where [status]<>3;");
	m_member_count = atoi(memberCount.c_str());
	return m_member_count;
}

int CTcpPacketAccept::OnClose() {
	printf("CTcpPacketAccept onClose\n");
	m_tReleaseTime = time(NULL) + 60;

	if (m_client_memberid != -1) {
		CGroupObj* grpObj = (CGroupObj*)m_grpObj;
		CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
		int onlineMemberCount = grpObj->updateOnlineMemberIds(m_client_memberid, false, m_tmConnect);

		Json::Value jAddOnlineMember;
		jAddOnlineMember["cmd"]  = "addOnlineMember";
		jAddOnlineMember["online"] = onlineMemberCount;
		jAddOnlineMember["memberCount"] = pServer->getMemberCount();
		jAddOnlineMember["addmemberId"] = m_client_memberid;
		jAddOnlineMember["tmConnect"] = m_tmConnect;
		jAddOnlineMember["add"] = false;
		
		pServer->broadcast(jAddOnlineMember);
	}

	if (on_event) {
		Json::Value jVal;
		jVal["action"] = "OnClose";
		jVal["grpKey"] = m_svrKey;
		jVal["side"] = "server";
		on_event(jVal.toFastString().c_str());
	}
	return 0;
}

int CTcpPacketAccept::OnRecv(const unsigned char * pBuf, int nLen) {
	//printf("OnRecv: %d\n", nLen);
	EnterCriticalSection(m_CritRecv);
	int tot = m_RecvTemp.Write((char*)pBuf, nLen);
	while (tot >=  sizeof(PACKET_HEADER))
	{
		PACKET_HEADER header;
		m_RecvTemp.Read((char*)&header, sizeof(PACKET_HEADER), false);

		int size = htons(header.size);
		if (tot >= size)
		{
			char * buf = new char[size+1];
			m_RecvTemp.Read(buf, size, true);
			//DoPacket((const void *)buf,true);
			for (int iii = 0; iii < 1; iii++)
			{
				buf[size] = 0;
				TCP_PACKET * pk = (TCP_PACKET*)buf;
				printf("recv: %s\n", pk->data);
        
				bool bSkip = false;
				Json::Value jMsg;
				Json::Reader reader;
				std::string _text = pk->data;
				if (!reader.parse(_text, jMsg)) break;

				if (jMsg.isMember("ek") && jMsg.isMember("t")) {
					std::string _cipher = jMsg["ek"].asString();
					_text = msgDecrypt(m_encryptKey, m_encryptIv, jMsg["t"].asString().c_str(), _cipher);
					if (_text == "@fail@") break;
					Json::Reader reader1;
					jMsg.clear();
					if (!reader1.parse(_text, jMsg)) break;
				}

				std::string cmd = "";
				if (jMsg.isMember("cmd")) {
					cmd = jMsg["cmd"].asString();
					if (cmd == "ping") bSkip = true;
				}
				
				if (cmd == "sendMsg") {
					OnRecv_sendMsg(jMsg);
					bSkip = true;
				}
				else {
					if (jMsg.isMember("msgType")) {
						std::string msgType = jMsg["msgType"].asString();
						if (msgType == "clientLogin") {
						OnRecv_clientLogin(jMsg);
						bSkip = true;
						}
						else if (msgType == "joinGroup") {
						OnRecv_joinGroup(jMsg);
						bSkip = true;
						}
						else if (msgType == "reqMember") {
						OnRecv_reqMember(jMsg);
						bSkip = true;
						}
						else if (msgType == "reqMessage") {
						OnRecv_reqMessage(jMsg);
						bSkip = true;
						}
						else if (msgType == "deleteMsg") {
						OnRecv_deleteMsg(jMsg);
						bSkip = true;
						}
						else if (msgType == "addEmoji") {
						OnRecv_addEmoji(jMsg);
						bSkip = true;
						}
						else if (msgType == "removeEmoji") {
						OnRecv_removeEmoji(jMsg);
						bSkip = true;
						}
					}
				}
				
				if (bSkip == false && m_bLogined) {

					if (on_event) {
						Json::Value jVal;
						jVal["action"] = "OnRecv";
						jVal["grpKey"] = m_svrKey;
						jVal["side"] = "server";
						jVal["msg"] = _text;
						on_event(jVal.toFastString().c_str());
					}
					if (strstr(_text.c_str(), PING_MSG_HEADER)==NULL && m_bLogined) {
						CTcpPacketServer * pServer = (CTcpPacketServer *)m_pServer;
						pServer->broadcast(buf, size);
					}
				}
			}

			delete [] buf;
			tot = m_RecvTemp.GetSize();
		}
		else
		{
			break;
		}
	}
	LeaveCriticalSection(m_CritRecv);
	return 0;
}

bool CTcpPacketServer::DecryptClientLogin(Json::Value & jLoginKey) {
	std::string loginCreateTime = jLoginKey["createTime"].asString();
    std::string loginKey = jLoginKey["key"].asString();
  	int memberId = jLoginKey["id"].asInt();
	return DecryptClientLogin(loginCreateTime.c_str(), memberId, loginKey.c_str());
}
bool CTcpPacketServer::DecryptClientLogin(const char * createTime, unsigned int id, const char * key) {
	CAESCrypt aes_member;
	char iv[64], cmp[64];
	sprintf(iv, "%s%u%s", createTime, id, AES_member_iv.c_str());
	aes_member.SetCryptIV(iv);
	aes_member.SetCryptKey((char*)AES_member_key.c_str());
	int plainKey_len = 0;
	char * plainKey = (char *)aes_member.DecryptHexStr(key, strlen(key), plainKey_len);
	plainKey[plainKey_len-1] = 0;
	sprintf(cmp, "%s.%s.", MEMBER_ENCRYPT_KEY, createTime);
	if (strstr(plainKey, cmp)) {
		delete [] plainKey;
		return true;
	}
	delete [] plainKey;
	return false;
}

void CTcpPacketServer::OnAccept(int new_socket, const char * ip, int port)
{
	CTcpPacketAccept * sock = new CTcpPacketAccept(ip, port, on_event, m_svrKey.c_str(), this, m_grpObj);
	AddClientSocket(new_socket, sock);

	if (on_event) {
		Json::Value jVal;
		jVal["action"] = "OnAccept";
		jVal["grpKey"] = m_svrKey;
		on_event(jVal.toFastString().c_str());
	}
	/*
	std::string test = "test!";
	int r = sock->SendData((const unsigned char *)test.c_str(), test.length());
	printf("accet: %d\n", r);
	*/

}

void CTcpPacketServer::broadcast(std::string & data) {
  int len = sizeof(PACKET_HEADER)+data.length()+1;
  char * buf = new char[len];
  PACKET_HEADER * header = (PACKET_HEADER *)buf;
  header->size = htons(sizeof(PACKET_HEADER)+data.length()+1);
  memcpy(buf+sizeof(PACKET_HEADER), data.c_str(), data.length()+1);
  broadcast(buf,len);
  delete [] buf;
}

void CTcpPacketServer::broadcast(Json::Value & jMsg) {
#if 0
  std::string data = jMsg.toFastString();
  broadcast(data);
#else
  CTcpPacketAccept * sock = NULL;
	EnterCriticalSection(m_CritCloseSocket);
	std::list<CTcpAccept *>::iterator acceptIt;
	for (acceptIt = m_pAcceptSocket.begin(); acceptIt != m_pAcceptSocket.end(); acceptIt++) {
		sock = (CTcpPacketAccept *)(*acceptIt);
    if (sock == NULL) continue;
    if (sock->isLogined() == false) continue;
		sock->SendData_json(jMsg);
	}
	LeaveCriticalSection(m_CritCloseSocket);
#endif
}

void CTcpPacketServer::broadcast(char * data, int len) {
	time_t tNow = time(NULL);
	CTcpPacketAccept * sock = NULL;
	EnterCriticalSection(m_CritCloseSocket);
	std::list<CTcpAccept *>::iterator acceptIt;
	for (acceptIt = m_pAcceptSocket.begin(); acceptIt != m_pAcceptSocket.end(); acceptIt++) {
		sock = (CTcpPacketAccept *)(*acceptIt);
    if (sock == NULL) continue;
    if (sock->isLogined() == false) continue;
		sock->SendData((const unsigned char*)data, len);
	}
	LeaveCriticalSection(m_CritCloseSocket);
}

int CTcpPacketClient::OnRecv(const unsigned char * pBuf, int nLen) {
	//printf("onrecv:%d\n", nLen);
	EnterCriticalSection(m_CritRecv);
	int tot = m_RecvTemp.Write((char*)pBuf, nLen);
	while (tot >=  sizeof(PACKET_HEADER))
	{
		PACKET_HEADER header;
		m_RecvTemp.Read((char*)&header, sizeof(PACKET_HEADER), false);

		int size = htons(header.size);
		if (tot >= size)
		{
			char * buf = new char[size+1];
			m_RecvTemp.Read(buf, size, true);
			//DoPacket((const void *)buf,true);
			for (int iii = 0; iii < 1; iii++)
			{
				buf[size] = 0;
				TCP_PACKET * pk = (TCP_PACKET*)buf;
				printf("recv: %s\n", pk->data);
				//if (on_event && strstr(pk->data, PING_MSG_HEADER)==NULL) {
				if (on_event) {
					bool bSkip = false;
					Json::Value jMsg;
					Json::Reader reader;
					std::string _text = pk->data;
					if (!reader.parse(_text, jMsg)) break;
					if (jMsg.isMember("ek") && jMsg.isMember("t")) {
						std::string _cipher = jMsg["ek"].asString();
						_text = msgDecrypt(m_encryptKey, m_encryptIv, jMsg["t"].asString().c_str(), _cipher);
						if (_text == "@fail@") break;
						Json::Reader reader1;
						jMsg.clear();
						if (!reader1.parse(_text, jMsg)) break;
					}
					if (jMsg.isMember("cmd")) {
						if (jMsg["cmd"].asString() == "ping") bSkip = true;
						else if (jMsg["cmd"].asString() == "OnClientLogin") {
							onRecv_OnClientLogin(jMsg);
						}
						else if (jMsg["cmd"].asString() == "onJoinGroup") {
							onRecv_onJoinGroup(jMsg);
						}
						else if (jMsg["cmd"].asString() == "addOnlineMember") {
							onRecv_addOnlineMember(jMsg);
						}
						else if (jMsg["cmd"].asString() == "onReqMember") {
							onRecv_onReqMember(jMsg);
						}
						else if (jMsg["cmd"].asString() == "onReqMessage") {
							onRecv_onReqMessage(jMsg);
							bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "newMember") {
							onRecv_newMember(jMsg);
							bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "onSendMsg") {
							onRecv_onSendMsg(jMsg);
							bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "newMsg") {
							onRecv_newMsg(jMsg);
							//bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "updateMsg") {
							onRecv_updateMsg(jMsg);
							//bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "changeMember") {
							onRecv_changeMember(jMsg);
							bSkip = true;
						}
						else if (jMsg["cmd"].asString() == "deleteMsgFile") {
							onRecv_deleteMsgFile(jMsg);
							bSkip = true;
						}
					}
					
					if (bSkip == false) {
						Json::Value jVal;
						jVal["action"] = "OnRecv";
						jVal["grpKey"] = m_svrKey;
						jVal["side"] = "client";
						jVal["msg"] = _text;
						on_event(jVal.toFastString().c_str());
					}
				}
			}
			delete [] buf;
			tot = m_RecvTemp.GetSize();
		}
		else
		{
			break;
		}
	}
	LeaveCriticalSection(m_CritRecv);
	return 0;
}

void CTcpPacketClient::sendMsg(const char * msg, int len) {
  PACKET_HEADER header;
  header.size = htons(sizeof(PACKET_HEADER)+len);
  SendData((const unsigned char*)(&header),sizeof(PACKET_HEADER));
  SendData((const unsigned char*)msg,len);
}

void CTcpPacketClient::SendHeartBeat() {
	std::stringstream msg;
	msg << PING_MSG_HEADER << time(NULL) << "\"}";

	std::string data = msg.str();
	SendData_string(data);
	//return 0;
}

void CTcpPacketClient::OnClose() {
	printf("CTcpPacketClient onClose\n");
	if (on_event) {
		Json::Value jVal;
		jVal["action"] = "OnClose";
		jVal["grpKey"] = m_svrKey;
		jVal["side"] = "client";
		on_event(jVal.toFastString().c_str());
	}
}

void CTcpPacketClient::OnConnect() {
  m_bReadyMember = false;
  m_bReadyMsg = false;
	if (on_event) {
		Json::Value jVal;
		jVal["action"] = "OnConnect";
		jVal["grpKey"] = m_svrKey;
		jVal["side"] = "client";
		on_event(jVal.toFastString().c_str());
	}

	Json::Value jMsg;
	if (m_client_login_createTime != "" && m_client_login_key != "") {
		jMsg["msgType"] = "clientLogin";
		jMsg["createTime"] = m_client_login_createTime;
    jMsg["memberId"] = m_memberid;
		jMsg["key"] = m_client_login_key;
	}
	else {
		jMsg["msgType"] = "joinGroup";
		jMsg["cipher"] = m_client_joinCipher;
		jMsg["name"] = m_szJoiner;

		Json::Value jIcon;
        Json::Reader reader;
        reader.parse(m_szIcon.c_str(), jIcon);
		jMsg["icon"] = jIcon;
	}
	std::string msg = jMsg.toFastString();
  	sendMsg(msg.c_str(), msg.length()+1);
}

void CTcpPacketClient::connectThread() {
	std::string err = "";
	this->Connect(m_server1.c_str(), m_port1, err);
	if (err != "" && on_event) {
		//
	}

	m_connectLock.lock();
	m_bConnecting1 = false;
	m_connectLock.unlock();
}

void CTcpPacketClient::asyncConnet(const char * domain_name, int port) {
	m_connectLock.init();
	m_connectLock.lock();
	m_server1 = domain_name;
	m_port1 = port;
	m_bConnecting1 = true;
	m_bStopped1 = false;

	m_connectThread = std::thread(&CTcpPacketClient::connectThread, this);
	m_connectLock.unlock();
}

void CTcpPacketClient::asyncConnet() {
	m_connectLock.lock();
	if (m_bConnecting1 || m_bStopped1) {
		m_connectLock.unlock();
		return;
	}

	if (m_connectThread.joinable()) m_connectThread.join();

	m_bConnecting1 = true;
	m_connectThread = std::thread(&CTcpPacketClient::connectThread, this);
	m_connectLock.unlock();
}

void CTcpPacketClient::stop() {
	m_connectLock.lock();
	m_bStopped1 = true;
	if (m_connectThread.joinable()) m_connectThread.join();
	this->CloseSocket();
	m_connectLock.unlock();
}
