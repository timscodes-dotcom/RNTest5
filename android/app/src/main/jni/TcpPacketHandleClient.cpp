#include <sstream>
#include <iomanip>
#include "TcpPacketServer.h"
#include "AES/AESCrypt.h"
#include "functions.h"
#include "CMongooseWebClient.h"

extern std::string g_tempDir;

std::string urlEncode(const std::string &value) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex;

    for (char c : value) {
        // 检查字符是否为字母或数字
        if (isalnum(static_cast<unsigned char>(c)) ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            // 如果是字母或数字，直接添加
            encoded << c;
        } else {
            // 否则，转换为 % 加上十六进制值
            encoded << '%' << std::setw(2) << std::uppercase << int(static_cast<unsigned char>(c));
        }
    }

    return encoded.str();
}

void CTcpPacketClient::SendData_json(Json::Value & jMsg) {
    bool needEncrypt = true;
    if (jMsg.isMember("msgType")) {
      std::string msgType = jMsg["msgType"].asString();
      if (msgType == "clientLogin" || msgType == "joinGroup") {
        needEncrypt = false;
      }
    }
    else if (jMsg.isMember("cmd")) {
      std::string cmd = jMsg["cmd"].asString();
      if (cmd == "ping") {
        needEncrypt = false;
      }
    }

    if (needEncrypt == false) {
      std::string data = jMsg.toFastString();
      SendData_string(data);
    }
    else {
      std::stringstream ss;
      ss << getTimeStampMS();
      Json::Value jEnc;
      jEnc["ek"] = msgEncrypt(m_encryptKey, m_encryptIv, ss.str().c_str(), jMsg.toFastString().c_str());
      jEnc["t"] = ss.str();
      std::string data = jEnc.toFastString();
      SendData_string(data);
    }
}
void CTcpPacketClient::SendData_string(std::string & data) {
    PACKET_HEADER header;
    header.size = htons(sizeof(PACKET_HEADER)+data.length()+1);
    SendData((const unsigned char*)(&header),sizeof(PACKET_HEADER));
    SendData((const unsigned char*)data.c_str(),data.length()+1);
}

void CTcpPacketClient::ReqMember() {
    if (m_bReadyMember) return;
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    Json::Value jReq;
    jReq["msgType"] = "reqMember";
    jReq["seq"] = grpObj->getMemberSeq();
    SendData_json(jReq);
}

void CTcpPacketClient::ReqMessage() {
  if (m_bReadyMsg) return;
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  Json::Value jReq;
  jReq["msgType"] = "reqMessage";
  jReq["seq"] = grpObj->getMsgSeq();
  SendData_json(jReq);
}

void CTcpPacketClient::onRecv_onReqMember(Json::Value & jMsg) {
  Json::Value jMembers = jMsg["members"];
  unsigned int maxSeq = jMsg["maxSeq"].asInt();
  int count = jMembers.size();
  bool bReady = (count==0);
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  int lastSeq = 0;
  int lastId = 0;
  for (int i = 0; i < count; i++) {
    Json::Value jRow = jMembers[i];
    int seq = atoi( jRow["seq"].asString().c_str() );
    int id = atoi( jRow["id"].asString().c_str() );
    std::stringstream ssql;
    ssql << "replace into t_member([id],[name],[key],[type],[status],[create_time],[seq],[attr]) values(";
        ssql << jRow["id"].asString() << ",";
        ssql << "'" << grpObj->msgEncrypt(jRow["name"].asString().c_str(), "0123456789") << "',";
        ssql << "'',";
        ssql << jRow["type"].asString() << ",";
        ssql << jRow["status"].asString() << ",";
        ssql << jRow["create_time"].asString() << ",";
        ssql << seq << ",";
        ssql << "'" << grpObj->msgEncrypt(jRow["attr"].asString().c_str(), "0123456789") << "');";
    std::string sql = ssql.str();
    std::string err = grpObj->execDB(sql.c_str());
    if (seq == maxSeq ) bReady = true;
    if (seq > lastSeq) lastSeq = seq;
    if (id > lastId) lastId = id;
  }
  
  if (bReady) {
    if (lastSeq>0) grpObj->setMemberSeq(lastSeq);
    if (lastId>0) grpObj->setMemberId(lastId);
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "updateMemberSeqs";
        jVal["grpKey"] = m_svrKey;
        jVal["id"] = grpObj->getMemberId();
        jVal["seq"] = grpObj->getMemberSeq();
        on_event(jVal.toFastString().c_str());
    }
    m_bReadyMember = true;
    if (!m_bReadyMsg) {
      ReqMessage();
    }
  }
  else {
    if (lastSeq>0) grpObj->setMemberSeq(lastSeq);
    if (lastId>0) grpObj->setMemberId(lastId);
    ReqMember();
  }
}

void CTcpPacketClient::onRecv_onReqMessage(Json::Value & jMsg) {
  Json::Value jMsgs = jMsg["msgs"];
  unsigned int maxSeq = jMsg["maxSeq"].asInt();
  int count = jMsgs.size();
  bool bReady = (count==0);
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  int lastSeq = 0;
  int lastId = 0;
  for (int i = 0; i < count; i++) {
    Json::Value jRow = jMsgs[i];
    int seq = atoi( jRow["seq"].asString().c_str() );
    int id = atoi( jRow["id"].asString().c_str() );
    std::stringstream ssql;
    ssql << "replace into t_message([id],[member_id],[msg_time],[msg_type],[msg],[status],[seq],[attr]) values(";
    ssql << jRow["id"].asString() << ",";
    ssql << jRow["member_id"].asString() << ",";
    ssql << jRow["msg_time"].asString() << ",";
    ssql << jRow["msg_type"].asString() << ",";
    ssql << "'" << grpObj->msgEncrypt(jRow["msg"].asString().c_str(), jRow["msg_time"].asString().c_str()) << "',";
    ssql << jRow["status"].asString() << ",";
    ssql << seq << ",";
    ssql << "'" << escapeSQLite(jRow["attr"].asString().c_str()) << "');";
    std::string sql = ssql.str();
    std::string err = grpObj->execDB(sql.c_str());
    if (seq == maxSeq ) bReady = true;
    if (seq > lastSeq) lastSeq = seq;
    if (id > lastId) lastId = id;
  }
  
  if (bReady) {
    if (lastSeq>0) grpObj->setMsgSeq(lastSeq);
    if (lastId>0) grpObj->setMsgId(lastId);
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "updateMsgSeqs";
        jVal["grpKey"] = m_svrKey;
        jVal["id"] = grpObj->getMsgId();
        jVal["seq"] = grpObj->getMsgSeq();
        on_event(jVal.toFastString().c_str());
    }
    m_bReadyMsg = true;
  }
  else {
    if (lastSeq>0) grpObj->setMsgSeq(lastSeq);
    if (lastId>0) grpObj->setMsgId(lastId);
    ReqMessage();
  }
}

void CTcpPacketClient::onRecv_addOnlineMember(Json::Value & jMsg) {
    int addmemberId = jMsg["addmemberId"].asInt();
    time_t tmConnect = jMsg["tmConnect"].asInt64();
    bool bAdd = jMsg["tmConnect"].asBool();
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    grpObj->updateOnlineMemberIds(addmemberId, bAdd, tmConnect);
}

void CTcpPacketClient::onRecv_OnClientLogin(Json::Value & jMsg) {
    int bVerify = jMsg["verify"].asInt();
    //          msg << "{\"cmd\":\"OnClientLogin\", \"verify\":" << (bVerify?1:0) << "}";
    m_bLogined = (bVerify==1);

    if (bVerify == 1) {
      strncpy(m_encryptKey, jMsg["ek1"].asString().c_str(), sizeof(m_encryptKey));
      strncpy(m_encryptIv, jMsg["ek2"].asString().c_str(), sizeof(m_encryptIv));
    }
    else {
      return;
    }

    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    if (grpObj->isServer()) {
        m_bReadyMember = true;
        m_bReadyMsg = true;
    }
    else {
      grpObj->setWebKey(jMsg["w1"].asString().c_str());
      grpObj->setWebIv(jMsg["w2"].asString().c_str());
      ReqMember();
    }
}

void CTcpPacketClient::onRecv_onJoinGroup(Json::Value & jMsg) {
    m_bLogined = jMsg["verify"].asBool();
  if (m_bLogined) {

    strncpy(m_encryptKey, jMsg["ek1"].asString().c_str(), sizeof(m_encryptKey));
    strncpy(m_encryptIv, jMsg["ek2"].asString().c_str(), sizeof(m_encryptIv));

    m_memberid = jMsg["memberId"].asInt();
    m_memberType = jMsg["memberType"].asInt();
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    grpObj->setClientMemberId(m_memberid);

    grpObj->setWebKey(jMsg["w1"].asString().c_str());
    grpObj->setWebIv(jMsg["w2"].asString().c_str());
  }
    //ReqMember();
}

void CTcpPacketClient::onRecv_newMember(Json::Value & jMsg) {
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  if (grpObj->isServer()) return;
  
  int newMemberId = jMsg["id"].asInt();
  int prevMemberSeq = jMsg["prevSeq"].asInt();
  if (prevMemberSeq == grpObj->getMemberSeq()) {
    int seq = jMsg["seq"].asInt();

    Json::Value jMemberValues = jMsg["memberValues"];
    std::stringstream ssql;
    ssql << "(";
    ssql << jMemberValues["id"].asInt() << ",";
    ssql << "'" << grpObj->msgEncrypt(jMemberValues["name"].asString().c_str(), "0123456789") << "',";
    ssql << "'" << jMemberValues["key"].asString() << "',";
    ssql << jMemberValues["type"].asInt() << ",";
    ssql << jMemberValues["status"].asInt() << ",";
    ssql << jMemberValues["create_time"].asString() << ",";
    ssql << jMemberValues["seq"].asInt() << ",";
    ssql << "'" << grpObj->msgEncrypt(jMemberValues["attr"].asString().c_str(), "0123456789") << "');";

    std::string sql = "replace into t_member([id],[name],[key],[type],[status],[create_time],[seq],[attr]) values" + ssql.str();
    std::string err = grpObj->execDB(sql.c_str());

    grpObj->setMemberId(newMemberId);
    grpObj->setMemberSeq(seq);
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "updateMemberSeqs";
        jVal["grpKey"] = m_svrKey;
        jVal["id"] = grpObj->getMemberId();
        jVal["seq"] = grpObj->getMemberSeq();
        on_event(jVal.toFastString().c_str());
    }
  }
  else {
    if (m_bReadyMember) {
        m_bReadyMember = false;
    }
    ReqMember();
  }
}

void CTcpPacketClient::onRecv_changeMember(Json::Value & jMsg) {
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  if (grpObj->isServer()) return;
  
  int memberId = jMsg["memberId"].asInt();
  int prevMemberSeq = jMsg["prevSeq"].asInt();
  if (prevMemberSeq == grpObj->getMemberSeq()) {
    int seq = jMsg["seq"].asInt();
    if (jMsg["cmd1"].asString() == "changeMemberType") {
        int newMemberType = jMsg["newMemberType"].asInt();
        char sql[256];
        sprintf(sql, "update t_member set [type]=%d, [seq]=%d where [id]=%d;", newMemberType, seq, memberId);
        std::string err = grpObj->execDB(sql);
    }
    else if (jMsg["cmd1"].asString() == "deleteMember") {
        char sql[256];
        sprintf(sql, "update t_member set [status]=%d, [seq]=%d where [id]=%d;", MEMBER_STATUS_DELETED, seq, memberId);
        std::string err = grpObj->execDB(sql);
    }
    else if (jMsg["cmd1"].asString() == "updateProfie") {
        Json::Value jAttr;
				jAttr["name"] = jMsg["name"].asString();
				jAttr["icon"] = jMsg["icon"];
        char sql[512];
        sprintf(sql, "update t_member set [name]='%s', [attr]='%s', [seq]=%u where [id]=%d;", 
                grpObj->msgEncrypt(jMsg["name"].asString().c_str(), jMsg["create_time"].asString().c_str()).c_str(), 
                grpObj->msgEncrypt(jAttr.toFastString().c_str(), jMsg["create_time"].asString().c_str()).c_str(), 
                seq, memberId);
        std::string err = grpObj->execDB(sql);
    }

    grpObj->setMemberSeq(seq);
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "updateMemberSeqs";
        jVal["grpKey"] = m_svrKey;
        jVal["id"] = grpObj->getMemberId();
        jVal["seq"] = grpObj->getMemberSeq();
        on_event(jVal.toFastString().c_str());
    }
  }
  else {
    if (m_bReadyMember) {
        m_bReadyMember = false;
    }
    ReqMember();
  }
}

void CTcpPacketClient::onRecv_onSendMsg(Json::Value & jMsg) {
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "onSendMsg";
        on_event(jVal.toFastString().c_str());
    }
}

void CTcpPacketClient::onRecv_updateMsg(Json::Value & jMsg) {
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    if (grpObj->isServer()) {
        if (on_event) {
            Json::Value jVal;
            jVal["action"] = "updateMsgSeqs";
            jVal["grpKey"] = m_svrKey;
            jVal["id"] = grpObj->getMsgId();
            jVal["seq"] = grpObj->getMsgSeq();
            on_event(jVal.toFastString().c_str());
        }
    }
    else {
        if (m_bReadyMsg) {
            m_bReadyMsg = false;
        }
        ReqMessage();
    }
}

void CTcpPacketClient::onRecv_newMsg(Json::Value & jMsg) {
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
      
      int newMsgId = jMsg["id"].asInt();
      int prevMsgSeq = jMsg["prevSeq"].asInt();

      if (grpObj->isServer()) {
        if (on_event) {
              Json::Value jVal;
              jVal["action"] = "updateMsgSeqs";
              jVal["grpKey"] = m_svrKey;
              jVal["id"] = newMsgId;
              jVal["seq"] = jMsg["seq"].asInt();
              on_event(jVal.toFastString().c_str());
          }
        return;
      }
    if (prevMsgSeq == grpObj->getMsgSeq()) {
        int seq = jMsg["seq"].asInt();
        //std::string sqlValues = jMsg["sqlValues"].asString();

        Json::Value jMsgValues = jMsg["msgValues"];
        std::stringstream ssql;
        ssql << "(";
        ssql << jMsgValues["id"].asInt() << ",";
        ssql << jMsgValues["member_id"].asInt() << ",";
        ssql << jMsgValues["msg_time"].asString() << ",";
        ssql << jMsgValues["msg_type"].asInt() << ",";
        ssql << "'" << grpObj->msgEncrypt(jMsgValues["msg"].asString().c_str(), jMsgValues["msg_time"].asString().c_str()) << "',";
        ssql << jMsgValues["status"].asInt() << ",";
        ssql << jMsgValues["seq"].asInt() << ",";
        ssql << "'" << jMsgValues["attr"].asString() << "');";

        std::string sql = "replace into t_message([id],[member_id], [msg_time],[msg_type],[msg],[status],[seq],[attr]) values" + ssql.str();
        std::string err = grpObj->execDB(sql.c_str());

        grpObj->setMsgId(newMsgId);
        grpObj->setMsgSeq(seq);
        if (on_event) {
            Json::Value jVal;
            jVal["action"] = "updateMsgSeqs";
            jVal["grpKey"] = m_svrKey;
            jVal["id"] = grpObj->getMsgId();
            jVal["seq"] = grpObj->getMsgSeq();
            on_event(jVal.toFastString().c_str());
        }
    }
    else {
        if (m_bReadyMsg) {
            m_bReadyMsg = false;
        }
        ReqMessage();
    }
/*
    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "newMsg";
        on_event(jVal.toFastString().c_str());
    }
    */
}

void CTcpPacketClient::onRecv_deleteMsgFile(Json::Value & jMsg) {
  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  unsigned long msgId = jMsg["msgId"].asInt();
  char sql[1024];
  sprintf(sql, "select [id],[member_id],[msg_time],[msg_type],[msg],[status],[seq],[attr] from t_message where [id]=%lu;", msgId);
  Json::Value jRec;
  std::string err = grpObj->selectDB((const char*)sql,jRec);
  if (err == "") {
    for (int ii = 0; ii < 1; ii++) {
      if (jRec.size() != 1) continue;
      Json::Value jRow = jRec[0];
      std::string msg = grpObj->msgDecrypt( jRow["msg"].asString().c_str(), jRow["msg_time"].asString().c_str() );
      std::string attr = jRow["attr"].asString();
      Json::Reader reader, reader1;
      Json::Value jOriMsg, jAttr;
      if (!reader.parse(msg.c_str(), jOriMsg)) continue;
      if (!reader1.parse(attr.c_str(), jAttr)) continue;
      if (!jOriMsg.isMember("fileList")) continue;
      Json::Value jFileList = jOriMsg["fileList"];
      if (!jFileList.isArray()) continue;
      for (int i = 0; i < jFileList.size(); i++) {
        if (jFileList[i].isMember("uploaded") && jFileList[i]["uploaded"].asBool() && jFileList[i].isMember("uploadedFile")) {
          std::string fileName = jFileList[i]["uploadedFile"].asString();
          remove(fileName.c_str());
        }
        char downloadFile[256];
        sprintf(downloadFile, "downloadFile%d", i);
        if (jAttr.isMember(downloadFile)) {
          std::string fileName = jAttr[downloadFile].asString();
          remove(fileName.c_str());
        }
      }
    }
  }
}

std::string CTcpPacketClient::downloadFile(Json::Value & jFile, int msgId, int idx, char * downloadFileName) {
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    std::string strUploadedFile = jFile["uploadedFile"].asString();
    const char * p = strrchr(strUploadedFile.c_str(), '/');
    if (!p) return "Fail to get uploadedFile name!";
    std::string fileName = p+1;
    char url[256];//, downloadFileName[256];
    sprintf(downloadFileName, "%s/msg_files/%s", grpObj->getGrpFolder().c_str(), jFile["name"].asString().c_str());

    FILE * fp = fopen(downloadFileName, "rb");
    if (fp) {
        fclose(fp);
        return "";
    }

    sprintf(url, "http://%s:%d/download", m_server1.c_str(), m_filePort);

    std::stringstream ss;
    ss << getTimeStampMS();
    Json::Value jEnc;
    jEnc["e"] = ::msgEncrypt(grpObj->getWebKey(), grpObj->getWebIv(), ss.str().c_str(), fileName.c_str());
    jEnc["t"] = ss.str();

    CMongooseWebClient client("user","pass");
    //return client.downloadFile(url, urlEncode(fileName).c_str(), downloadFileName);
    return client.downloadFile(url, urlEncode(jEnc.toFastString()).c_str(), downloadFileName);
}

std::string CTcpPacketClient::uploadFile(Json::Value & jFile, int msgId) {

  CGroupObj* grpObj = (CGroupObj*)m_grpObj;
  
  std::string file = jFile["uri"].asString();
  int idx = jFile["idx"].asInt();

  std::string fileName = jFile["name"].asString();

  std::stringstream ss;
  ss << getTimeStampMS();
  Json::Value jEnc;
  jEnc["e"] = ::msgEncrypt(grpObj->getWebKey(), grpObj->getWebIv(), ss.str().c_str(), fileName.c_str());
  jEnc["t"] = ss.str();

  char url[1024];
  //sprintf(url, "http://%s:%d/upload/%s?msgid=%d&idx=%d", m_server1.c_str(), m_filePort, urlEncode(fileName).c_str(), msgId, idx);
  sprintf(url, "http://%s:%d/upload/%s?msgid=%d&idx=%d", m_server1.c_str(), m_filePort, urlEncode(jEnc.toFastString()).c_str(), msgId, idx);

  CMongooseWebClient client("user","pass");
  if (strstr(file.c_str(), "content:/")) {
    return client.uploadFile(file.c_str()+9, url);
  }
  else if (strstr(file.c_str(), "file://")) {
    return client.uploadFile(file.c_str()+7, url);
  }
  else {
    return client.uploadFile(file.c_str(), url);
  }
  return "";
}
