/*
 * functions.h
 *
 *  Created on: Oct 2, 2024
 *      Author: ytim
 */

#ifndef SRC_FUNCTIONS_H_
#define SRC_FUNCTIONS_H_

#include "./json/json/json.h"
#include "./TcpPacketServer.h"
#include "sqlite/SQLiteDB.h"
#include "CRITICAL_SECTION.h"

#include "CMongooseWebServer.h"
#include "CMongooseWebClient.h"

#include <map>

class CGroupObj {
public:
	CGroupObj(Json::Value & root);
	std::string start(void (*onEvent)(const char *), Json::Value & root);
	std::string stop();
	void clientReconnect();
  std::string sendMsg(Json::Value & root);
  std::string uploadFile(Json::Value & root);
  static void uploadFileThread(void* p, void* sz);
  void _uploadFileThread(char* szJson);
  std::string downloadFile(Json::Value & root);
  static void downloadFileThread(void* p, void* sz);
  void _downloadFileThread(char* szJson);

	std::string load_db_config(void (*onEvent)(const char *));
	void set_AES_member_key(const char * key) {
		AES_member_key = key;
		m_serverMsg.set_AES_member_key(key);
	}
	void set_AES_member_iv(const char * iv) {
		AES_member_iv = iv;
		m_serverMsg.set_AES_member_iv(iv);
	}
	void SetServerKey(const char * clientLoginCreateTime, const char * clientLoginKey) {
		m_serverMsg.SetServerKey(m_strGrpKey.c_str());
		m_clientMsg.SetServerKey(m_strGrpKey.c_str(), clientLoginCreateTime, clientLoginKey);
	}

	std::string encryptGroupAddress(const char * plainText);
  
  	bool isServer() {return m_strGrpType == "server";}

	std::string startDB();
	std::string selectDB(const char * sql, Json::Value & jRet);
  	std::string execDB(const char * sql);
	std::string GetOneValue(const char * sql);

	unsigned int getMemberId(bool bNext = false) {
		m_lock.lock();
		unsigned int id = bNext?(++m_maxMemberId):m_maxMemberId;
		m_lock.unlock();
		return id;
	}

	unsigned int getMemberId(unsigned int* nextId) {
		m_lock.lock();
		unsigned int id = m_maxMemberId;
		*nextId = ++m_maxMemberId;
		m_lock.unlock();
		return id;
	}

	void setMemberId(unsigned int id) {
		m_lock.lock();
		if (m_maxMemberId < id) m_maxMemberId = id;
		m_lock.unlock();
	}

	unsigned int getMemberSeq(bool bNext = false) {
		m_lock.lock();
		unsigned int seq = bNext?(++m_maxMemberSeq):m_maxMemberSeq;
		m_lock.unlock();
		return seq;
	}

	unsigned int getMemberSeq(unsigned int* nextSeq) {
		m_lock.lock();
		unsigned int seq = m_maxMemberSeq;
		*nextSeq = ++m_maxMemberSeq;
		m_lock.unlock();
		return seq;
	}
  
  void setMemberSeq(unsigned int seq) {
    m_lock.lock();
    if (m_maxMemberSeq < seq) m_maxMemberSeq = seq;
    m_lock.unlock();
  }

	unsigned int getMsgId(bool bNext = false) {
		m_lock.lock();
		unsigned int id = bNext?(++m_maxMsgId):m_maxMsgId;
		m_lock.unlock();
		return id;
	}
  
  void setMsgId(unsigned int id) {
    m_lock.lock();
    if (m_maxMsgId < id) m_maxMsgId = id;
    m_lock.unlock();
  }

	unsigned int getMsgSeq(bool bNext = false) {
		m_lock.lock();
		unsigned int seq = bNext?(++m_maxMsgSeq):m_maxMsgSeq;
		m_lock.unlock();
		return seq;
	}

	unsigned int getMsgSeq(unsigned int* nextSeq) {
		m_lock.lock();
		unsigned int seq = m_maxMsgSeq;
		*nextSeq = ++m_maxMsgSeq;
		m_lock.unlock();
		return seq;
	}
  
  void setMsgSeq(unsigned int seq) {
    m_lock.lock();
    if (m_maxMsgSeq < seq) m_maxMsgSeq = seq;
    m_lock.unlock();
  }
  
  void setClientMemberId(unsigned int id) {
    m_clientMsg.setMemberId(id);
	m_client_login_id = id;
  }

  unsigned int getClientLoginId() { return m_client_login_id; }

	void onHttpEvent(const char* action, void* ptr, std::string& apiRet);

	int queryMemberType(int id, char * err);

    std::string getGrpFolder() { return m_grpFolder; }
	std::string getGroupKey() { return m_strGrpKey; }

	int updateOnlineMemberIds(int memberId, bool bAdd, time_t tmConnect);
	void getOnlineMemberIds(Json::Value & jIds);

	std::string getMsgKey() { return m_msgKey; }
	std::string getMsgIv() { return m_msgIv; }

	std::string msgEncrypt(const char * msg, const char * tm);
	std::string msgDecrypt(const char * msg, const char * tm);
	void setMsgKey(const char * key) { m_msgKey = key; }
	void setMsgIv(const char * iv) { m_msgIv = iv; }

	SQLiteDB & getDB() { return m_db; }

	void setWebKey(const char * key) { strncpy(m_webKey, key, sizeof(m_webKey)); }
	void setWebIv(const char * iv) { strncpy(m_webIv, iv, sizeof(m_webIv)); }
	const char * getWebKey() { return m_webKey; }
	const char * getWebIv() { return m_webIv; }

private:
	std::string m_strPath;
	std::string m_strGrpKey;
	std::string m_strTimeMS;
	std::string m_strGrpType;
	std::string m_strMsgPort;
	std::string m_strFilePort;
	std::string m_strConnectIp;
	std::string m_grpFolder;

	CTcpPacketServer m_serverMsg;
	CMongooseWebServer m_fileServer;
	CTcpPacketClient m_clientMsg;

	bool m_started;
	time_t m_timeConnect;

	std::string AES_member_key;
	std::string AES_member_iv;

	std::string m_client_login_createTime;
	std::string m_client_login_key;
	unsigned int m_client_login_id;

	unsigned int m_maxMemberId;
	unsigned int m_maxMemberSeq;

	unsigned int m_maxMsgId;
	unsigned int m_maxMsgSeq;

	SQLiteDB m_db;

	std::map<int, time_t> m_online_memberIds;

	CRITICAL_SECTION m_lock;
	void (*on_event)(const char *);

	std::string m_msgKey;
	std::string m_msgIv;

	char m_webKey[9];
	char m_webIv[9];
};

void func_init();
std::string func_createGroup(Json::Value & root, CGroupObj * grpObj, std::string & ownerKey, bool isServer = true);
std::string func_createGroup1(Json::Value & root, bool bNew, void (*onEvent)(const char *));
std::string func_deleteGroup(const char * path, const char * key, const char * timeMS);
std::string func_deleteGroup1(Json::Value & root);
std::string func_startGroup(Json::Value & root, void (*onEvent)(const char *));
std::string func_stopGroup(Json::Value & root, void (*onEvent)(const char *));
std::string func_sendMsg(Json::Value & root, void (*onEvent)(const char *));
std::string func_encryptGroupAddress(Json::Value & root);
std::string func_decryptGroupAddress(Json::Value & root);
std::string func_joinGroup1(Json::Value & root, void (*onEvent)(const char *));
std::string func_getGroupSeqs(Json::Value & root);
std::string func_loadMsg(Json::Value & root);
std::string func_loadMember(Json::Value & root);
std::string func_uploadFile(Json::Value & root, void (*onEvent)(const char *));
std::string func_downloadFile(Json::Value & root, void (*onEvent)(const char *));
std::string func_md5Encrypt(Json::Value & root);
std::string func_md5Verify(Json::Value & root);
std::string func_webEncrypt(Json::Value & root);

void setTempDir(const char * dir);

std::string func_aes_encrypt_hex(std::string & key, std::string & iv, std::string & text);
std::string func_aes_decrypt_hex(std::string & key, std::string & iv, std::string & cipher);
std::string msgEncrypt(const char * _key, const char * _iv, const char * tm, const char * msg);
std::string msgDecrypt(const char * _key, const char * _iv, const char * tm, std::string & cipher);

#endif /* SRC_FUNCTIONS_H_ */
