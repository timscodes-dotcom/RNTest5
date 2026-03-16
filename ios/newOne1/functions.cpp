#include "functions.h"

//#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sstream>
#include <iomanip>

#include <thread>
#include <map>

#include "AES/AESCrypt.h"
#include "md5/md5.h"

CRITICAL_SECTION m_mapGroupLock;
std::map<std::string, CGroupObj*> m_mapGroup;

std::string g_tempDir = "";
extern std::string g_deviceID;
/*
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <cstring>

std::string GetLocalIPAddress1()
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
*/

void setTempDir(const char * dir) {
  g_tempDir = dir;
}

void Group_Thread_Func() {
	while (true) {
		m_mapGroupLock.lock();
		std::map<std::string, CGroupObj*>::iterator it;
		for (it = m_mapGroup.begin(); it != m_mapGroup.end(); it++) {
			it->second->clientReconnect();
		}
		m_mapGroupLock.unlock();
		Sleep(1000);
	}
}

void func_init() {
	m_mapGroupLock.init();
	std::thread(Group_Thread_Func).detach();
}

std::string func_joinGroup1(Json::Value & root, void (*onEvent)(const char *)) {
	//Json::Value vPath = root["path"];
	Json::Value vGrpKey = root["grpKey"];
	//Json::Value vTimeMS = root["grpTimeMS"];

	Json::Value JRet;
	std::string grpKey = vGrpKey.asString();
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(grpKey);
	if (it != m_mapGroup.end()) {
		std::string ret1 = "group already exist!";
		m_mapGroupLock.unlock();
		JRet["ret"] = ret1;
		return JRet.toFastString();
	}
	CGroupObj * grp = new CGroupObj(root);
	m_mapGroup[grpKey] = grp;
	m_mapGroupLock.unlock();

	std::string ownerKey = "";
	std::string ret = func_createGroup(root, grp, ownerKey, false);
	if (ret != "") {
		JRet["ret"] = ret;
		return JRet.toFastString();
	}
	return ownerKey;
}

std::string func_getGroupSeqs(Json::Value & root) {
	Json::Value vGrpKey = root["grpKey"];
	Json::Value jRet;
	std::string grpKey = vGrpKey.asString();
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(grpKey);
	if (it == m_mapGroup.end()) {
		std::string ret1 = "no this group";
		m_mapGroupLock.unlock();
		jRet["ret"] = ret1;
		return jRet.toFastString();
	}
	jRet["ret"] = "";
	jRet["lastMemberId"] = it->second->getMemberId();
	jRet["lastMemberSeq"] = it->second->getMemberSeq();
	jRet["lastMsgId"] = it->second->getMsgId();
	jRet["lastMsgSeq"] = it->second->getMsgSeq();
	m_mapGroupLock.unlock();
	return jRet.toFastString();
}

std::string func_createGroup1(Json::Value & root, bool bNew, void (*onEvent)(const char *)) {
	//Json::Value vPath = root["path"];
	Json::Value vGrpKey = root["grpKey"];
	//Json::Value vTimeMS = root["grpTimeMS"];

	Json::Value JRet;
	std::string grpKey = vGrpKey.asString();
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(grpKey);
	if (it != m_mapGroup.end()) {
		std::string ret1 = "group already exist!";
		if (bNew == false) {
			ret1 = it->second->load_db_config(onEvent);
		}
		m_mapGroupLock.unlock();
		JRet["ret"] = ret1;
		return JRet.toFastString();
	}
	CGroupObj * grp = new CGroupObj(root);
	m_mapGroup[grpKey] = grp;
	m_mapGroupLock.unlock();

	if (bNew == false) {
		Json::Value vLoginKey = root["loginKey"];
		int memberId = vLoginKey["id"].asInt();
		grp->setClientMemberId(memberId);
		std::string ret1 = grp->load_db_config(onEvent);
		JRet["ret"] = ret1;
		return JRet.toFastString();
	}

	std::string ownerKey = "";
	std::string ret = func_createGroup(root, grp, ownerKey);
	if (ret != "") {
		JRet["ret"] = ret;
		return JRet.toFastString();
	}
	return ownerKey;
}

std::string func_createGroup(Json::Value & root, CGroupObj * grpObj, std::string & ownerKey, bool isServer) {
    Json::Value vPath = root["path"];
	Json::Value vGrpKey = root["grpKey"];
	Json::Value vTimeMS = root["grpTimeMS"];
  std::string path = vPath.asString();
  std::string key = vGrpKey.asString();
  std::string timeMS = vTimeMS.asString();

	char szTempName1[256];
    //sprintf(szTempName1, "cchat@%s%s&tscode", name, timeMS);
    sprintf(szTempName1, "cchat@%s&tscode", key.c_str());
    MD5 md5 = MD5(szTempName1);
    std::string md5Name = md5.hexdigest();

    std::string ret = "";

    std::string grpFolder = path;
    grpFolder += "/"+md5Name;
    int nRC = mkdir( grpFolder.c_str(), 0777 );
    if( nRC == -1 )
    {
        switch( errno )
        {
        case ENOENT:
            //parent didn't exist,
            ret = "parent folder didn't exist";
            break;
        case EEXIST:
            //Done! already exist
            break;
        default:
            ret = "fail to create folder files";
            break;
        }
        return ret;
    }

    std::string dbFile = grpFolder + "/msg.db";
    char err[1024];
#if 0
	SQLiteDB db;
	db.Open((char*)dbFile.c_str(), err);
    if (strcmp(err, "") != 0) {
        return err;
    }
#else
	grpObj->startDB();
	SQLiteDB db = grpObj->getDB();
#endif

	std::string sql = "CREATE TABLE if not exists [t_config] (\
  [id] INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, \
  [attr] TEXT NOT NULL);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	Json::Value jConfig;
	Json::Value jAES_member;
	CAESCrypt aes_member;
	char szTemp[128];
	if (isServer) {
		randString(32, szTemp);
		jAES_member["key"] = szTemp;
		aes_member.SetCryptKey(szTemp);
		grpObj->set_AES_member_key((const char*)szTemp);
		randString(16, szTemp);
		jAES_member["iv"] = szTemp;
		//aes_member.SetCryptIV(szTemp);
		grpObj->set_AES_member_iv((const char*)szTemp);
		jConfig["AES_member"] = jAES_member;
		randString(8, szTemp);
		jConfig["a1"] =  szTemp;
		grpObj->setMsgKey((const char*)szTemp);
		randString(8, szTemp);
		jConfig["a2"] =  szTemp;
		grpObj->setMsgIv((const char*)szTemp);

		sql = std::string("insert into t_config(attr) values('") + escapeSQLite(jConfig.toFastString()) + "');";
	}
	else {
		randString(8, szTemp);
		jConfig["a1"] =  szTemp;
		grpObj->setMsgKey((const char*)szTemp);
		randString(8, szTemp);
		jConfig["a2"] =  szTemp;
		grpObj->setMsgIv((const char*)szTemp);
		sql = std::string("insert into t_config(attr) values('") + escapeSQLite(jConfig.toFastString()) + "');";
	}
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	Json::Value JMemberKey;
  unsigned int maxMemberId = 0;
  unsigned int maxMemberSeq = 0;
	if (isServer) {
    maxMemberId = grpObj->getMemberId(true);
    maxMemberSeq = grpObj->getMemberSeq(true);
		unsigned long long create_time = getTimeStampMS();
		char szRand[6];
		randString(5, szRand);
		char plainKey[64], iv[64];
		sprintf(iv, "%llu%u%s", create_time, maxMemberId, jAES_member["iv"].asString().c_str());
		aes_member.SetCryptIV(iv);
		int cipherKey_len = 0;
		sprintf(plainKey, "%s.%llu.%s", MEMBER_ENCRYPT_KEY, create_time, szRand);
		unsigned char * cipherKey = aes_member.Encrypt((unsigned char *)plainKey, strlen(plainKey)+1, cipherKey_len);

		JMemberKey["key"] = aes_member.parseByte2HexStr(cipherKey, cipherKey_len);
		delete [] cipherKey;
		sprintf(szTemp, "%llu", create_time);
		JMemberKey["createTime"] = szTemp;
    JMemberKey["id"] = maxMemberId;
		JMemberKey["ret"] = "";
		JMemberKey["folder"] = grpFolder;

		ownerKey = JMemberKey.toFastString();
	}
	else {
		Json::Value vJoinCipher = root["joinCipher"];
		JMemberKey["key"] = "";
		JMemberKey["createTime"] = "";
    JMemberKey["id"] = 0;
		JMemberKey["joinCipher"] = vJoinCipher.asString().c_str();
		JMemberKey["ret"] = "";
		JMemberKey["folder"] = grpFolder;
    
    ownerKey = JMemberKey.toFastString();
	}

	sql = "CREATE TABLE if not exists [t_member] (\
  [id] INTEGER NOT NULL PRIMARY KEY, \
  [name] VARCHAR(64) NOT NULL, \
  [key] TEXT NOT NULL, \
  [type] INT NOT NULL, \
  [status] INT NOT NULL, \
  [create_time] BIGINT NOT NULL, \
  [update_time] BIGINT, \
  [seq] BIGINT NOT NULL, \
  [attr] TEXT);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	sql = "CREATE INDEX if not exists [idx_member] ON [t_member] ([id], [key]);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	sql = "CREATE INDEX if not exists [idx_member_name] ON [t_member] ([name]);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	sql = "CREATE INDEX if not exists [idx_member_seq] ON [t_member] ([seq]);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	if (isServer) {
    	Json::Value vOwner = root["owner"];
    	std::string szOwner = vOwner["name"].asString();
		
		std::stringstream ssql;
		//"insert into t_member([name],[key],[type],[status],[create_time],[seq],[attr]) values('owner','xxx',0,0,1728356343235,1,'{}');";
		ssql << "insert into t_member([id],[name],[key],[type],[status],[create_time],[seq],[attr]) values(";
		ssql << maxMemberId << ",";
		ssql << "'" << grpObj->msgEncrypt(szOwner.c_str(), "0123456789") << "',";
		ssql << "'" << JMemberKey["key"].asString() << "',";
		ssql << MEMBER_TYPE_OWNER << ",";
		ssql << MEMBER_STATUS_VALID << ",";
		ssql << JMemberKey["createTime"].asString() << ",";
		ssql << maxMemberSeq << ",";
		ssql << "'" << grpObj->msgEncrypt(vOwner.toFastString().c_str(), "0123456789") << "');";
		db.Exec(ssql.str().c_str(), err);
		if (strcmp(err, "") != 0) {
			return err;
		}

		grpObj->SetServerKey(JMemberKey["createTime"].asString().c_str(), JMemberKey["key"].asString().c_str());
    grpObj->setClientMemberId(maxMemberId);
	}

	sql = "CREATE TABLE if not exists [t_message] (\
  [id] INTEGER NOT NULL PRIMARY KEY, \
  [member_id] INTEGER NOT NULL, \
  [msg_time] BIGINT NOT NULL, \
  [msg_type] INT NOT NULL, \
  [msg] TEXT NOT NULL, \
  [status] INT NOT NULL, \
  [seq] BIGINT NOT NULL, \
  [attr] TEXT);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	sql = "CREATE INDEX if not exists [idx_msg_seq] ON [t_message] ([seq]);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	sql = "CREATE INDEX if not exists [idx_msg_time] ON [t_message] ([msg_time]);";
	db.Exec(sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }
  
  if (isServer) {
    unsigned int maxMsgId = grpObj->getMsgId(true);
    unsigned int maxMsgSeq = grpObj->getMsgSeq(true);
    std::stringstream ssql;
    ssql << "insert into t_message([id],[member_id], [msg_time],[msg_type],[msg],[status],[seq],[attr]) values(";
    ssql << maxMsgId << ",";
    ssql << 1 << ",";
    ssql << JMemberKey["createTime"].asString() << ",";
	ssql << MSG_TYPE_TEXT << ",";
    //ssql << "'@welcome@',";
	ssql << "'" << grpObj->msgEncrypt( "@welcome@", JMemberKey["createTime"].asString().c_str() ) << "',";
    ssql << MSG_STATUS_VALID << ",";
    ssql << maxMsgSeq << ",";
    ssql << "'" << "{}" << "');";
    db.Exec(ssql.str().c_str(), err);
    if (strcmp(err, "") != 0) {
      return err;
    }
  }

	grpFolder += "/msg_files";
    nRC = mkdir( grpFolder.c_str(), 0777 );
    if( nRC == -1 )
    {
        switch( errno )
        {
        case ENOENT:
            //parent didn't exist,
            ret = "parent folder didn't exist";
            break;
        case EEXIST:
            //Done! already exist
            break;
        default:
            ret = "fail to create folder files";
            break;
        }
        return ret;
    }

    return ret;
}

int remove_dir(const char *path, std::string & ret) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        //perror("Failed to open directory");
        ret = "Failed to open directory";
        return -1;
    }

    struct dirent *entry;
    char full_path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat statbuf;

        if (stat(full_path, &statbuf) == -1) {
            //perror("Failed to get file status");
            ret = "Failed to get file status";
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Recursively delete subdirectory
            if (remove_dir(full_path, ret) == -1) {
                closedir(dir);
                return -1;
            }
        } else {
            // Delete file
            if (remove(full_path) == -1) {
                //perror("Failed to remove a file");
                ret = "Failed to remove a file";
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);

    // Delete the empty directory
    if (rmdir(path) == -1) {
        perror("Failed to remove a directory");
        return -1;
    }

    return 0;
}

std::string func_deleteGroup1(Json::Value & root) {
	Json::Value vPath = root["path"];
	Json::Value vGrpKey = root["grpKey"];
	Json::Value vTimeMS = root["grpTimeMS"];

	std::string grpKey = vGrpKey.asString();

	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(grpKey);
	if (it == m_mapGroup.end()) {
		m_mapGroupLock.unlock();
		return "group not exist!";
	}
	CGroupObj* grp = it->second;
    if (grp) {
		grp->stop();
		//delete grp;
	}
	m_mapGroup.erase(it);
	m_mapGroupLock.unlock();

	return func_deleteGroup(vPath.asString().c_str(), grpKey.c_str(), vTimeMS.asString().c_str());
}

std::string func_deleteGroup(const char * path, const char * key, const char * timeMS) {
    char szTempName1[256];
    sprintf(szTempName1, "cchat@%s&tscode", key);
    MD5 md5 = MD5(szTempName1);
    std::string md5Name = md5.hexdigest();

    std::string ret = "";

    std::string grpFolder = path;
    grpFolder += "/"+md5Name;

    std::string dbFile = grpFolder + "/msg.db";
    struct stat stat_buf;
    int rc = stat(dbFile.c_str(), &stat_buf);
    if (rc == 0) {
        FILE * fp = fopen(dbFile.c_str(), "wb");
        if (fp) {
            char _tmp = 0;
            fwrite(&_tmp, 1, stat_buf.st_size, fp);
            fclose(fp);
        }
    }

    remove_dir(grpFolder.c_str(), ret);


    return ret;
}

std::string func_startGroup(Json::Value & root, void (*onEvent)(const char *)) {
	Json::Value vGrpKey = root["grpKey"];
	Json::Value jRet;
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
	if (it == m_mapGroup.end()) {
		m_mapGroupLock.unlock();
		jRet["ret"] = "no this group!";
		return jRet.toFastString();
	}
  std::string ret = "";
  if (!it->second->isServer()) {
    Json::Value vLoginKey = root["loginKey"];
    Json::Value vJoinCipher = vLoginKey["joinCipher"];
    std::string cipher = vJoinCipher.asString();
    if (vLoginKey.isMember("createTime") && vLoginKey.isMember("key")) {
      Json::Value vLoginKey_createTime = vLoginKey["createTime"];
      Json::Value vLoginKey_key = vLoginKey["key"];
      it->second->SetServerKey(vLoginKey_createTime.asString().c_str(), vLoginKey_key.asString().c_str());
    }
    ret = it->second->start(onEvent, root);
  }
  else {
    ret = it->second->start(onEvent, root);
  }
	
	jRet["ret"] = ret;
	if (ret == "") {
		jRet["lastMemberId"] = it->second->getMemberId();
		jRet["lastMemberSeq"] = it->second->getMemberSeq();
		jRet["lastMsgId"] = it->second->getMsgId();
		jRet["lastMsgSeq"] = it->second->getMsgSeq();
	}
	m_mapGroupLock.unlock();
	return jRet.toFastString();
}

std::string func_stopGroup(Json::Value & root, void (*onEvent)(const char *)) {
	Json::Value vGrpKey = root["grpKey"];
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
	if (it == m_mapGroup.end()) {
		m_mapGroupLock.unlock();
		return "no this group!";
	}
	it->second->stop();
	m_mapGroupLock.unlock();
	return "";
}

std::string func_downloadFile(Json::Value & root, void (*onEvent)(const char *)) {
  Json::Value vGrpKey = root["grpKey"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    return "no this group!";
  }
  std::string ret = it->second->downloadFile(root);
  m_mapGroupLock.unlock();
  return ret;
}

std::string func_uploadFile(Json::Value & root, void (*onEvent)(const char *)) {
  Json::Value vGrpKey = root["grpKey"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    return "no this group!";
  }
  it->second->uploadFile(root);
  m_mapGroupLock.unlock();
  return "";
}

std::string func_sendMsg(Json::Value & root, void (*onEvent)(const char *)) {
  Json::Value vGrpKey = root["grpKey"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    return "no this group!";
  }
  std::string ret = it->second->sendMsg(root);
  m_mapGroupLock.unlock();
  return ret;
}

std::string func_loadMsg(Json::Value & root) {
  Json::Value jRet;
  Json::Value vGrpKey = root["grpKey"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    jRet["ret"] = "no this group!";
    return jRet.toFastString();
  }
  
  std::string szType = root["type"].asString(); // "init"
  Json::Value jRows;
  char sql[512];
  if (szType == "init") {
    sprintf(sql, "select m.[id],m.[member_id],b.[name] as member_name,m.[msg_time],m.[msg_type],m.[msg],m.[status],m.[seq],m.[attr],b.[attr] as member_attr from t_message m left join t_member b on m.member_id=b.[id] order by m.[id] desc limit 100;");
  }
  else if (szType == "newSeq") { //newSeq
    int seqFrom = root["seqFrom"].asInt();
    sprintf(sql, "select m.[id],m.[member_id],b.[name] as member_name,m.[msg_time],m.[msg_type],m.[msg],m.[status],m.[seq],m.[attr],b.[attr] as member_attr from t_message m left join t_member b on m.member_id=b.[id] where m.seq>%d order by m.[id] desc limit 100;", seqFrom);
  }
  else { //reload
	int msgId = root["msgId"].asInt();
	sprintf(sql, "select m.[id],m.[member_id],b.[name] as member_name,m.[msg_time],m.[msg_type],m.[msg],m.[status],m.[seq],m.[attr],b.[attr] as member_attr from t_message m left join t_member b on m.member_id=b.[id] where m.[id]=%d;", msgId);
  }
  jRet["ret"] = it->second->selectDB(sql, jRows);
  for (int i = 0; i < jRows.size(); i++) {
	Json::Value row = jRows[i];
	jRows[i]["msg"] = it->second->msgDecrypt(row["msg"].asString().c_str(), row["msg_time"].asString().c_str());
	jRows[i]["member_name"] = it->second->msgDecrypt(row["member_name"].asString().c_str(), "0123456789");
	jRows[i]["member_attr"] = it->second->msgDecrypt(row["member_attr"].asString().c_str(), "0123456789");
  }
  jRet["rows"] = jRows;
  m_mapGroupLock.unlock();
  return jRet.toFastString();
}

std::string func_loadMember(Json::Value & root) {
  Json::Value jRet;
  Json::Value vGrpKey = root["grpKey"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    jRet["ret"] = "no this group!";
    return jRet.toFastString();
  }
  
  //std::string szType = root["type"].asString(); // "init"
  Json::Value jRows;
  char sql[512];
  if (root.isMember("id")) {
	sprintf(sql, "select [id],[name],[type],[status],[create_time],[seq],[attr] from t_member where ([status]=0 or [status]=2) and [id]=%d order by [type], [name] limit 100;", root["id"].asInt());
  }
  else {
  	sprintf(sql, "select [id],[name],[type],[status],[create_time],[seq],[attr] from t_member where [status]=0 or [status]=2 order by [type], [name] limit 100;");
  }
  jRet["ret"] = it->second->selectDB(sql, jRows);
  for (int i = 0; i < jRows.size(); i++) {
	Json::Value row = jRows[i];
	jRows[i]["name"] = it->second->msgDecrypt(row["name"].asString().c_str(), "0123456789");
	jRows[i]["attr"] = it->second->msgDecrypt(row["attr"].asString().c_str(), "0123456789");
  }
  jRet["rows"] = jRows;
  m_mapGroupLock.unlock();
  return jRet.toFastString();
}

std::string func_encryptGroupAddress(Json::Value & root) {
  Json::Value jRet;
  Json::Value vGrpKey = root["grpKey"];
  Json::Value vGrpAddrInfo = root["grpAddrInfo"];
  m_mapGroupLock.lock();
  std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(vGrpKey.asString());
  if (it == m_mapGroup.end()) {
    m_mapGroupLock.unlock();
    jRet["ret"] = "no this group!";
	return jRet.toFastString();
  }

  unsigned long long tNow = getTimeStampMS();
  char plainText[64];
  sprintf(plainText,"%s.%llu",MEMBER_QRCODE_ENCRYPT_KEY,tNow);
  vGrpAddrInfo["cipher"] = it->second->encryptGroupAddress(plainText);
  vGrpAddrInfo["cmp"] = "local@Chat";
  m_mapGroupLock.unlock();

  CAESCrypt aes_member;
	aes_member.SetCryptKey((char*)QRCODE_ENCRYPT_KEY);
	aes_member.SetCryptIV((char*)QRCODE_ENCRYPT_IV);

	std::string szGrpAddrInfo = vGrpAddrInfo.toFastString();
	int cipherText_len = 0;
	unsigned char * cipherText = aes_member.Encrypt((unsigned char *)szGrpAddrInfo.c_str(), szGrpAddrInfo.length()+1, cipherText_len);

	jRet["cipher"] = aes_member.parseByte2HexStr(cipherText, cipherText_len);
	delete [] cipherText;
  
  jRet["ret"] = "";
  return jRet.toFastString();
}

std::string func_decryptGroupAddress(Json::Value & root) {
	Json::Value jRet;
	Json::Value vGrpAddrInfo = root["grpAddrInfo"];
	std::string szGrpAddrInfo = vGrpAddrInfo.asString();
	CAESCrypt aes_member;
	aes_member.SetCryptKey((char*)QRCODE_ENCRYPT_KEY);
	aes_member.SetCryptIV((char*)QRCODE_ENCRYPT_IV);
	char cmp[64];
	int plainText_len = 0;
	char * plainText = (char*)aes_member.DecryptHexStr(szGrpAddrInfo.c_str(), szGrpAddrInfo.length(), plainText_len);
	plainText[plainText_len-1] = 0;
	bool bValid = true;
	sprintf(cmp, "\"local@Chat\"");
	if (strstr(plainText, cmp) == NULL) {
		jRet["ret"] = "encrypt fail";
		delete [] plainText;
		return jRet.toFastString();
	}
	try {
		Json::Value jMsg;
		Json::Reader reader;
		reader.parse(plainText, jMsg);
		long long tExpire = atoll( jMsg["e"].asString().c_str() );
		long long tNow = getTimeStampMS();
		if (tNow > tExpire) {
			jRet["ret"] = "expire";
		}
		else {
			jRet["ret"] = "";
			jRet["plainText"] = plainText;
		}
	}
	catch(const std::exception& e)  {
		bValid = false;
		jRet["ret"] = "fail to parse json";
	}
	delete [] plainText;
	return jRet.toFastString();
}

CGroupObj::CGroupObj(Json::Value & root) {

	m_lock.init();
	m_maxMemberId = 0;
	m_maxMemberSeq = 0;
	m_maxMsgId = 0;
	m_maxMsgSeq = 0;

	Json::Value vPath = root["path"];
	m_strPath = vPath.asString();
	Json::Value vGrpKey = root["grpKey"];
	m_strGrpKey = vGrpKey.asString();
	Json::Value vTimeMS = root["grpTimeMS"];
	m_strTimeMS = vTimeMS.asString();
	Json::Value vGrpType = root["grpType"];
	m_strGrpType = vGrpType.asString();
	Json::Value vMsgPort = root["msgPort"];
	m_strMsgPort = vMsgPort.asString();
	Json::Value vFilePort = root["filePort"];
	m_strFilePort = vFilePort.asString();

	Json::Value vConnectIp = root["connectIp"];
	m_strConnectIp = vConnectIp.asString();

	if (root.isMember("loginKey")) {
		Json::Value vLoginKey = root["loginKey"];
		Json::Value vLoginKey_createTime = vLoginKey["createTime"];
		m_client_login_createTime = vLoginKey_createTime.asString();
		Json::Value vLoginKey_key = vLoginKey["key"];
		m_client_login_key = vLoginKey_key.asString();
		m_client_login_id = vLoginKey["id"].asInt();
	}
	else {
		m_client_login_createTime = "";
		m_client_login_key = "";
	}

	if (m_strGrpType == "server") {
		m_serverMsg.SetServerKey(m_strGrpKey.c_str());
		//m_serverMsg.Init();
		m_serverMsg.setGrpObj((void*)this);
    
		m_clientMsg.setEvents(NULL);
		m_clientMsg.SetServerKey(m_strGrpKey.c_str(), m_client_login_createTime.c_str(), m_client_login_key.c_str());
		m_clientMsg.setGrpObj((void*)this);
	}
	else {
		m_clientMsg.setEvents(NULL);
		m_clientMsg.SetServerKey(m_strGrpKey.c_str());
    	m_clientMsg.setGrpObj((void*)this);
	}
	m_started = false;
}

std::string CGroupObj::selectDB(const char * sql, Json::Value & jRet) {
	char err_msg[1024];
    m_db.Select((char*)sql, jRet, err_msg);
	return err_msg;
}

std::string CGroupObj::execDB(const char * sql) {
  char err_msg[1024];
    m_db.Exec((char*)sql, err_msg);
  return err_msg;
}

std::string CGroupObj::GetOneValue(const char * sql) {
	char err[1024];
	return m_db.GetOneValue((char*)sql, err);
}

bool onHttpEvent1(void* handler, const char* action, void* ptr, std::string& apiRet) {
    printf("onHttpEvent %s\n", action);
    CGroupObj* p = (CGroupObj*)handler;
    p->onHttpEvent(action, ptr, apiRet);
    return true;
}

int CGroupObj::queryMemberType(int id, char * err) {
	char sql[256];
	strcpy(err, "");
	sprintf(sql, "select [type] from t_member where [id]=%d and [status]=%d;", id, MEMBER_STATUS_VALID);
	std::string szValue = m_db.GetOneValue(sql, err);
	return atoi(szValue.c_str());
}

int CGroupObj::updateOnlineMemberIds(int memberId, bool bAdd, time_t tmConnect) {
	m_lock.lock();
	std::map<int, time_t>::iterator itr = m_online_memberIds.find(memberId);
	if (bAdd) {
		//if (itr == m_online_memberIds.end()) {
			m_online_memberIds[memberId] = tmConnect;
		//}
	}
	else {
		if (itr != m_online_memberIds.end()) {
			if (tmConnect <= itr->second) {
                m_online_memberIds.erase(itr);
            }
		}
	}
	int count = m_online_memberIds.size();
	m_lock.unlock();
	return count;
}

void CGroupObj::getOnlineMemberIds(Json::Value & jIds) {
	m_lock.lock();
	for (std::map<int, time_t>::iterator itr = m_online_memberIds.begin(); itr != m_online_memberIds.end(); itr++) {
		Json::Value item;
		item["id"] = itr->first;
		jIds.append(item);
	}
	m_lock.unlock();
}

void CGroupObj::onHttpEvent(const char* action, void* ptr, std::string& apiRet) {
	if (strcmp(action, "download") == 0 || strcmp(action, "uploadName") == 0) {
		struct mg_http_message* hm = (struct mg_http_message*)ptr;
		std::string szCmd;
        if (strcmp(action, "download") == 0) szCmd = decodeUrl(std::string(hm->query.buf, hm->query.len));
        if (strcmp(action, "uploadName") == 0) szCmd = decodeUrl(std::string(hm->uri.buf + 8, hm->uri.len - 8));
		Json::Reader reader;
		Json::Value jCmd;
		if (!reader.parse(szCmd.c_str(), jCmd)) {
			apiRet = "";
			return;
		}
		if (jCmd.isMember("e") && jCmd.isMember("t")) {
			long long tNow = getTimeStampMS();
			long long t = atoll(jCmd["t"].asString().c_str());
			if (t-5000 > tNow || t+5000 < tNow) {
				apiRet = "";
				return;
			}
			std::string _cipher = jCmd["e"].asString();
			apiRet = ::msgDecrypt(m_webKey, m_webIv, jCmd["t"].asString().c_str(), _cipher);
			if (apiRet == "@fail@") {
				apiRet = "";
				return;
			};
		}
		else {
			apiRet = "";
			return;
		}
	}
	else if (strcmp(action, "api") == 0) {
		struct mg_http_message* hm = (struct mg_http_message*)ptr;
		Json::Value jHttpReq;
		jHttpReq["uri"] = std::string(hm->uri.buf, hm->uri.len);
		jHttpReq["method"] = std::string(hm->method.buf, hm->method.len);
		jHttpReq["query"] = std::string(hm->query.buf, hm->query.len);
		jHttpReq["proto"] = std::string(hm->proto.buf, hm->proto.len);
		jHttpReq["body"] = std::string(hm->body.buf, hm->body.len);
		if (on_event) on_event(jHttpReq.toFastString().c_str());

		std::string szCmd = std::string(hm->body.buf, hm->body.len);
		Json::Reader reader;
		Json::Value jCmd;
		if (!reader.parse(szCmd.c_str(), jCmd)) {
			apiRet = "Invalid Post Data";
			return;
		}
		if (jCmd.isMember("e") && jCmd.isMember("t")) {
			long long tNow = getTimeStampMS();
			long long t = atoll(jCmd["t"].asString().c_str());
			if (t-5000 > tNow || t+5000 < tNow) {
				apiRet = "Timeout";
				return;
			}
			std::string _cipher = jCmd["e"].asString();
			szCmd = ::msgDecrypt(m_webKey, m_webIv, jCmd["t"].asString().c_str(), _cipher);
			if (szCmd == "@fail@") {
				apiRet = "Invalid Post Data1";
				return;
			};
			Json::Reader reader1;
			jCmd.clear();
			if (!reader1.parse(szCmd, jCmd)) {
				apiRet = "Invalid Post Data2";
				return;
			}
		}

		if (jCmd["cmd"].asString() == "changeMember") {
			Json::Value loginKey = jCmd["loginKey"];
			if (!m_serverMsg.DecryptClientLogin(loginKey)) {
				apiRet = "Fail to Verify ID";
				return;
			}
			char sql[512], err[1024];
			
			int accesserType = queryMemberType( loginKey["id"].asInt(), err );
			if (strcmp(err, "") != 0) {
				apiRet = err;
				return;
			}

			int memberId = 0;
			int memberType = 2;
			if (jCmd.isMember("memberId")) {
				memberId = jCmd["memberId"].asInt();

				memberType = queryMemberType( memberId, err );
				if (strcmp(err, "") != 0) {
					apiRet = err;
					return;
				}
			}

			if (jCmd["cmd1"].asString() == "changeMemberType") {
				int newMemberType = jCmd["newMemberType"].asInt();
				if (memberType <= accesserType || newMemberType <= accesserType) {
					apiRet = "Permission Deny";
					return;
				}

				unsigned int seq;
				unsigned int prevSeq = this->getMemberSeq(&seq);
				sprintf(sql, "update t_member set [type]=%d, [seq]=%u where [id]=%d;", newMemberType, seq, memberId);
				this->m_db.Exec(sql, err);
				if (strcmp(err,"") != 0) {
					apiRet = err;
				}
				else {
					jCmd["prevSeq"] = prevSeq;
					jCmd["seq"] = seq;
					this->m_serverMsg.broadcast(jCmd);
				}
			}
			else if (jCmd["cmd1"].asString() == "deleteMember") {
				if (memberType <= accesserType) {
					apiRet = "Permission Deny";
					return;
				}
				
				unsigned int seq;
				unsigned int prevSeq = this->getMemberSeq(&seq);
				sprintf(sql, "update t_member set [status]=%d, [seq]=%u where [id]=%d;", MEMBER_STATUS_DELETED, seq, memberId);
				this->m_db.Exec(sql, err);
				if (strcmp(err,"") != 0) {
					apiRet = err;
				}
				else {
					jCmd["prevSeq"] = prevSeq;
					jCmd["seq"] = seq;
					this->m_serverMsg.broadcast(jCmd);
				}
			}
			else if (jCmd["cmd1"].asString() == "updateProfie") {
				if (memberId != loginKey["id"].asInt()) {
					apiRet = "Invalid Member ID";
					return;
				}

				Json::Value jAttr;
				jAttr["name"] = jCmd["name"].asString();
				jAttr["icon"] = jCmd["icon"];
				unsigned int seq;
				unsigned int prevSeq = this->getMemberSeq(&seq);
				sprintf(sql, "update t_member set [name]='%s', [attr]='%s', [seq]=%u where [id]=%d;", 
							this->msgEncrypt(jCmd["name"].asString().c_str(), "0123456789").c_str(), 
							this->msgEncrypt(jAttr.toFastString().c_str(), "0123456789").c_str(), 
							seq, memberId);
				this->m_db.Exec(sql, err);
				if (strcmp(err,"") != 0) {
					apiRet = err;
				}
				else {
					jCmd["prevSeq"] = prevSeq;
					jCmd["seq"] = seq;
					this->m_serverMsg.broadcast(jCmd);
				}
			}
		}
	}
    else if (strcmp(action, "upload") == 0) {
        struct upload_file_info* uploading = (struct upload_file_info*)ptr;
        char sql[1024], err[1024];
        sprintf(sql, "select * from t_message where id=%d;", uploading->msgId);
        Json::Value jRec;
        m_db.Select(sql, jRec, err);
        if (strcmp(err, "") == 0) {
            if (jRec.size() == 1) {
                Json::Value jRow = jRec[0];
                std::string msg = this->msgDecrypt( jRow["msg"].asString().c_str(), jRow["msg_time"].asString().c_str() );
                Json::Reader reader;
                Json::Value jMsg;
                if (reader.parse(msg.c_str(), jMsg)) {
                    Json::Value jFileList = jMsg["fileList"];
                    Json::Value jFile = jFileList[uploading->idx];
                    if (jFile["name"].asString() == uploading->oriFileName) {
                        jMsg["fileList"][uploading->idx]["uploaded"] = true;
                        jMsg["fileList"][uploading->idx]["uploadedFile"] = uploading->fileName;
                        unsigned int seq;
                        unsigned int prevSeq = this->getMsgSeq(&seq);
                        //sprintf(sql, "update t_message set msg='%s',[seq]=%u where id=%d;", escapeSQLite(jMsg.toFastString()).c_str(), seq, uploading->msgId);
						std::stringstream ssql;
						ssql << "update t_message set msg='" << this->msgEncrypt(jMsg.toFastString().c_str(), jRow["msg_time"].asString().c_str())
							<< "', [seq]=" << seq << " where id=" << uploading->msgId << ";";
                        m_db.Exec(ssql.str().c_str(),err);
                        if (strcmp(err,"") != 0) {
                            apiRet = err;
                        }
                        else {
                            this->setMsgSeq(seq);
                            Json::Value jCmd;
                            jCmd["cmd"] = "updateMsg";
                            jCmd["prevSeq"] = prevSeq;
                            jCmd["seq"] = seq;
                            this->m_serverMsg.broadcast(jCmd);
                        }
                    }
                }
            }
        }
        /*
         * std::string szCmd = std::string(hm->body.buf, hm->body.len);
		Json::Reader reader;
		Json::Value jCmd;
		if (!reader.parse(szCmd.c_str(), jCmd)) {
			apiRet = "Invalid Post Data";
			return;
		}
         */
    }
}

std::string func_md5Encrypt(Json::Value & root) {
	Json::Value vText = root["text"];
	char szTempName1[256];
	sprintf(szTempName1, "A2d0@eW%s&p*9Oo&", vText.asString().c_str());
	MD5 md5 = MD5(szTempName1);
	return md5.hexdigest();
}

std::string func_md5Verify(Json::Value & root) {
	std::string cipher = func_md5Encrypt(root);
	Json::Value vCipher = root["cipher"];
	if (vCipher.asString() == cipher) return "ok";
	return "fail";
}

std::string CGroupObj::start(void (*onEvent)(const char *), Json::Value & root ) {
	if (m_started == true) return "already started";
	std::string err = startDB();
	if (err != "") return err;

	on_event = onEvent;
	
	if (isServer()) {
		m_serverMsg.setEvents(onEvent);
		m_serverMsg.updateMemberCount();
		m_serverMsg.Start(atoi(m_strMsgPort.c_str()), 2, err);
		if (err != "") return err;

		Json::Value vPath = root["path"];
		//Json::Value vTimeMS = root["grpTimeMS"];
		Json::Value vGrpKey = root["grpKey"];
		std::string path = vPath.asString();
		std::string key = vGrpKey.asString();
		//std::string timeMS = vTimeMS.asString();

		char szTempName1[256];
		//sprintf(szTempName1, "cchat@%s%s&tscode", name, timeMS);
		sprintf(szTempName1, "cchat@%s&tscode", key.c_str());
		MD5 md5 = MD5(szTempName1);
		std::string md5Name = md5.hexdigest();

		/*
		if (on_event) {
			Json::Value jVal;
			jVal["action"] = "grpFolder";
			jVal["folder"] = path + "/" + md5Name;
			on_event(jVal.toFastString().c_str());
		}
		*/

		randString(8, m_webKey);
		randString(8, m_webIv);

		m_fileServer.setEvents((void*)this, onHttpEvent1);
		m_grpFolder = path.c_str();
		std::string grpFolder = path;
		grpFolder += "/"+md5Name+"/msg_files";
		m_fileServer.init(
			grpFolder.c_str(),
			grpFolder.c_str(),
			(std::string("http://")+root["connectIp"].asString()+":"+m_strFilePort).c_str(),
			"user",
			"pass",
			false,
			1024*1024*100
		);
		err = m_fileServer.startWebServer();
		if (err != "") return err;
    /*
    Json::Value jRet;
    char err_msg[1024];
    m_db.Select("select * from t_member order by seq", jRet, err_msg);
     */
	}
	else {
		Json::Value vLoginKey = root["loginKey"];
		Json::Value vJoinCipher = vLoginKey["joinCipher"];
		std::string joinCipher = vJoinCipher.asString();
		Json::Value vJoiner = root["joiner"];

		if (joinCipher!="") m_clientMsg.SetJoinCipher(joinCipher.c_str(), vJoiner);
	}

	m_clientMsg.setEvents(onEvent);
  m_clientMsg.setFilePort(atoi(m_strFilePort.c_str()));

	//m_clientMsg.Connect(m_strConnectIp.c_str(), atoi(m_strMsgPort.c_str()), err);
	m_clientMsg.asyncConnet(m_strConnectIp.c_str(), atoi(m_strMsgPort.c_str()));
	m_timeConnect = time(NULL);

	m_started = true;

	return "";
}

std::string CGroupObj::stop() {
	if (m_started == false) return "already stopped";
	m_started = false;
	m_clientMsg.stop();
	if (m_strGrpType == "server") {
		m_serverMsg.Stop();
	}

    m_fileServer.stopWebServer();
  
  m_db.Close();

	return "";
}

void CGroupObj::clientReconnect() {
	if (m_started == true) {
		if (m_clientMsg.GetSocket() == INVALID_SOCKET) {
			time_t tNow = time(NULL);
			if (m_timeConnect + 5 <= tNow) {
				//std::string err = "";
				//m_clientMsg.Connect(m_strConnectIp.c_str(), atoi(m_strMsgPort.c_str()), err);
				m_clientMsg.asyncConnet();
				m_timeConnect = time(NULL);
			}
		}
	}
}

std::string CGroupObj::sendMsg(Json::Value & root) {
  if (m_started == false) return "grp connection is stopped";
  if (m_clientMsg.GetSocket() == INVALID_SOCKET) return "grp client socket is invalid";
  
  Json::Value vMsg = root["msg"];
  std::string msg = vMsg.toFastString();
  m_clientMsg.sendMsg(msg.c_str(), msg.length()+1);
  return "";
}

void CGroupObj::uploadFileThread(void* p, void* sz) {
	CGroupObj * pThis = (CGroupObj*)p;
	char * szJson = (char* )sz;
	pThis->_uploadFileThread(szJson);
	delete [] szJson;
}

void CGroupObj::_uploadFileThread(char* szJson) {
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(szJson, root)) {
		Json::Value vFile = root["file"];
		int msgId = root["msgId"].asInt();
		std::string ret = m_clientMsg.uploadFile(vFile, msgId);

		if (on_event) {
			Json::Value jVal;
			jVal["action"] = "uploadFile";
			jVal["ret"] = ret;
			jVal["grpKey"] = m_strGrpKey;
			jVal["msgId"] = msgId;
			jVal["idx"] = root["idx"].asInt();
			//jVal["fileName"] = downloadFileName;
			on_event(jVal.toFastString().c_str());
		}
	}
}

std::string CGroupObj::uploadFile(Json::Value & root) {
	if (this->isServer()) return "";
  if (m_started == false) return "grp connection is stopped";
  if (m_clientMsg.GetSocket() == INVALID_SOCKET) return "grp client socket is invalid";
  
#if 1
	std::string szRoot = root.toFastString();
	char * p = new char[szRoot.length()+1];
	strcpy(p, szRoot.c_str());
	std::thread(CGroupObj::uploadFileThread, this, p).detach();
	return "";
#else
  Json::Value vFile = root["file"];
  int msgId = root["msgId"].asInt();
  m_clientMsg.uploadFile(vFile, msgId);
#endif

  return "";
}

void CGroupObj::downloadFileThread(void* p, void* sz) {
	CGroupObj * pThis = (CGroupObj*)p;
	char * szJson = (char* )sz;
	pThis->_downloadFileThread(szJson);
	delete [] szJson;
}

void CGroupObj::_downloadFileThread(char* szJson) {
	Json::Reader reader;
	Json::Value root;
	std::string ret = "";
	if (reader.parse(szJson, root)) {
		Json::Value vFile = root["file"];
		int msgId = root["msgId"].asInt();
		int idx = root["idx"].asInt();
		char downloadFileName[256];
		ret = m_clientMsg.downloadFile(vFile, msgId, idx, downloadFileName);
		if (ret == "") {
			char sql[1024], err[1024];
			sprintf(sql, "select * from t_message where id=%d;", msgId);
			Json::Value jRec;
			m_db.Select(sql, jRec, err);
			if (strcmp(err, "") == 0) {
				if (jRec.size() == 1) {
					Json::Value jRow = jRec[0];
					std::string attr = jRow["attr"].asString();
					Json::Reader reader;
					Json::Value jAttr;
					if (reader.parse(attr.c_str(), jAttr)) {
						char _key[32];
						sprintf(_key,"downloadFile%d", idx);
						jAttr[_key] = downloadFileName;
						sprintf(sql, "update t_message set attr='%s' where id=%d;", escapeSQLite(jAttr.toFastString()).c_str(), msgId);
						m_db.Exec(sql,err);
						if (strcmp(err,"") != 0) {
							ret = err;
						}
						else {
							if (on_event) {
								Json::Value jVal;
								jVal["action"] = "downloadFile";
								jVal["ret"] = ret;
								jVal["grpKey"] = m_strGrpKey;
								jVal["msgId"] = msgId;
								jVal["idx"] = idx;
								jVal["attr"] = jAttr;
								on_event(jVal.toFastString().c_str());
							}
						}
					}
					else {
						ret = "fail to parse attr";
					}
				}
			}
			else {
				ret = err;
			}
		}

		if (ret != "") {
			if (on_event) {
				Json::Value jVal;
				jVal["action"] = "downloadFile";
				jVal["ret"] = ret;
				jVal["grpKey"] = m_strGrpKey;
				jVal["msgId"] = msgId;
				jVal["idx"] = idx;
				on_event(jVal.toFastString().c_str());
			}
		}
	}
}

std::string CGroupObj::downloadFile(Json::Value & root) {
	if (this->isServer()) return "";
  if (m_started == false) return "grp connection is stopped";
  if (m_clientMsg.GetSocket() == INVALID_SOCKET) return "grp client socket is invalid";
  
#if 1
	std::string szRoot = root.toFastString();
	char * p = new char[szRoot.length()+1];
	strcpy(p, szRoot.c_str());
	std::thread(CGroupObj::downloadFileThread, this, p).detach();
	return "";
#else
  
#endif
}

std::string CGroupObj::load_db_config(void (*onEvent)(const char *)) {
	char szTempName1[256];
    //sprintf(szTempName1, "cchat@%s%s&tscode", name, timeMS);
    sprintf(szTempName1, "cchat@%s&tscode", m_strGrpKey.c_str());
    MD5 md5 = MD5(szTempName1);
    std::string md5Name = md5.hexdigest();

    std::string ret = "";

    m_grpFolder = m_strPath.c_str();
    m_grpFolder += "/"+md5Name;

	on_event = onEvent;
	/*
	if (on_event) {
		Json::Value jVal;
		jVal["action"] = "grpFolder";
		jVal["folder"] = m_grpFolder;
		on_event(jVal.toFastString().c_str());
	}
	*/

	std::string dbFile = m_grpFolder + "/msg.db";
    char err[1024];
	m_db.Open((char*)dbFile.c_str(), err);
    if (strcmp(err, "") != 0) {
        return err;
    }

	std::string sql = "select attr from t_config order by id limit 1;";
	std::string szConfig = m_db.GetOneValue((char*)sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }

	Json::Reader reader;
	Json::Value jConfig;
	reader.parse(szConfig.c_str(), jConfig);
	if (isServer()) {
		if (!jConfig.isMember("AES_member")) return "no AES_member";
		Json::Value jAES_member = jConfig["AES_member"];
		if (!jAES_member.isMember("key")) return "no AES_member.key";
		this->set_AES_member_key(jAES_member["key"].asString().c_str());
		if (!jAES_member.isMember("iv")) return "no AES_member.iv";
		this->set_AES_member_iv(jAES_member["iv"].asString().c_str());

		m_serverMsg.updateMemberCount();
	}
	if (jConfig.isMember("a1")) m_msgKey = jConfig["a1"].asString();
	if (jConfig.isMember("a2")) m_msgIv = jConfig["a2"].asString();

	sql = "select IFNULL(max(id),0) as mx from t_member;";
	szConfig = m_db.GetOneValue((char*)sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }
	m_maxMemberId = atol(szConfig.c_str());
	sql = "select IFNULL(max(seq),0) as mx from t_member;";
	szConfig = m_db.GetOneValue((char*)sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }
	m_maxMemberSeq = atol(szConfig.c_str());

	sql = "select IFNULL(max(id),0) as mx from t_message;";
	szConfig = m_db.GetOneValue((char*)sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }
	m_maxMsgId = atol(szConfig.c_str());
	sql = "select IFNULL(max(seq),0) as mx from t_message;";
	szConfig = m_db.GetOneValue((char*)sql.c_str(), err);
	if (strcmp(err, "") != 0) {
        return err;
    }
	m_maxMsgSeq = atol(szConfig.c_str());

	return "";
}

std::string CGroupObj::encryptGroupAddress(const char * plainText) {
	CAESCrypt aes_member;
	aes_member.SetCryptKey((char*)AES_member_key.c_str());
	aes_member.SetCryptIV((char*)AES_member_iv.c_str());

	int cipherText_len = 0;
	unsigned char * cipherText = aes_member.Encrypt((unsigned char *)plainText, strlen(plainText)+1, cipherText_len);

	std::string strHex = aes_member.parseByte2HexStr(cipherText, cipherText_len);
	delete [] cipherText;
	return strHex;
}

std::string CGroupObj::startDB() {
	char szTempName1[256];
    //sprintf(szTempName1, "cchat@%s%s&tscode", name, timeMS);
    sprintf(szTempName1, "cchat@%s&tscode", m_strGrpKey.c_str());
    MD5 md5 = MD5(szTempName1);
    std::string md5Name = md5.hexdigest();

	m_grpFolder = m_strPath;
    m_grpFolder += "/"+md5Name;

	std::string dbFile = m_grpFolder + "/msg.db";
    char err[1024];
	m_db.Open((char*)dbFile.c_str(), err);
    if (strcmp(err, "") != 0) {
        return err;
    }
	return "";
}

std::string func_aes_encrypt_hex(std::string & key, std::string & iv, std::string & text) {
	CAESCrypt aes_member;
	aes_member.SetCryptKey((char*)key.c_str());
	aes_member.SetCryptIV((char*)iv.c_str());

	int cipherText_len = 0;
	unsigned char * cipherText = aes_member.Encrypt((unsigned char *)text.c_str(), text.length()+1, cipherText_len);

	std::string strHex = aes_member.parseByte2HexStr(cipherText, cipherText_len);
	delete [] cipherText;
	return strHex;
}

std::string func_aes_decrypt_hex(std::string & key, std::string & iv, std::string & cipher) {
	CAESCrypt aes_member;
	aes_member.SetCryptKey((char*)key.c_str());
	aes_member.SetCryptIV((char*)iv.c_str());
	int plainText_len = 0;
	char * plainText = (char*)aes_member.DecryptHexStr(cipher.c_str(), cipher.length(), plainText_len);
	plainText[plainText_len-1] = 0;
	std::string str = plainText;
	delete [] plainText;
	return str;
}

std::string msgEncrypt(const char * _key, const char * _iv, const char * tm, const char * msg) {
	std::string padding = "0000000000000000000000000000000000000000000000000000000000000000";
	std::string key = std::string(tm) + _key + "4Ko*" + padding;
	std::string iv = std::string(tm) + "9aW#" + _iv + padding;
	std::string text = std::string("6Jm%") + msg + std::string("3cW$");
	return func_aes_encrypt_hex(key, iv, text);
}

std::string msgDecrypt(const char * _key, const char * _iv, const char * tm, std::string & cipher) {
	std::string padding = "0000000000000000000000000000000000000000000000000000000000000000";
	std::string key = std::string(tm) + _key + "4Ko*" + padding;
	std::string iv = std::string(tm) + "9aW#" + _iv + padding;
	std::string text = func_aes_decrypt_hex(key, iv, cipher);
	int text_len = text.length();
	if (text.find("6Jm%") != 0 || text_len < 8) return "@fail@";
	if (!strstr(text.c_str()+text_len-4, "3cW$")) return "@fail@";
	return text.substr(4, text_len-8);
}

std::string CGroupObj::msgEncrypt(const char * msg, const char * tm) {
	std::string padding = "0000000000000000000000000000000000000000000000000000000000000000";
	std::string key = std::string(tm) + m_msgKey + g_deviceID + padding;
	std::string iv = std::string(tm) + g_deviceID + m_msgIv + padding;
	std::string text = std::string("9Ia@") + msg + std::string("2*Kz");
	return func_aes_encrypt_hex(key, iv, text);
}

std::string CGroupObj::msgDecrypt(const char * msg, const char * tm) {
	std::string padding = "0000000000000000000000000000000000000000000000000000000000000000";
	std::string key = std::string(tm) + m_msgKey + g_deviceID + padding;
	std::string iv = std::string(tm) + g_deviceID + m_msgIv + padding;
	std::string cipher = msg;
	std::string text = func_aes_decrypt_hex(key, iv, cipher);
	int text_len = text.length();
    if (text.find("9Ia@") != 0 || text_len < 8) return "{\"ret\":\"Decrypt Error\"}";
	if (!strstr(text.c_str()+text_len-4, "2*Kz")) return "{\"ret\":\"Decrypt Error\"}";
	return text.substr(4, text_len-8);
}

std::string func_webEncrypt(Json::Value & root) {
	Json::Value vGrpKey = root["grpKey"];
	Json::Value jRet;
	std::string grpKey = vGrpKey.asString();
	m_mapGroupLock.lock();
	std::map<std::string, CGroupObj*>::iterator it = m_mapGroup.find(grpKey);
	if (it == m_mapGroup.end()) {
		std::string ret1 = "no this group";
		m_mapGroupLock.unlock();
		jRet["ret"] = ret1;
		return jRet.toFastString();
	}

	Json::Value data = root["data"];
	std::stringstream ss;
    ss << getTimeStampMS();
	jRet["ret"] = "";
	jRet["e"] = msgEncrypt(it->second->getWebKey(), it->second->getWebIv(), ss.str().c_str(), data.toFastString().c_str());
	jRet["t"] = ss.str();
	m_mapGroupLock.unlock();
	return jRet.toFastString();
}