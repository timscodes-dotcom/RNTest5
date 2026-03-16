#include <sstream>
#include "TcpPacketServer.h"
#include "AES/AESCrypt.h"
#include "functions.h"

void CTcpPacketAccept::SendData_json(Json::Value & jMsg) {
    bool needEncrypt = true;
    if (jMsg.isMember("msgType")) {
        /*
      std::string msgType = jMsg["msgType"].asString();
      if (msgType == "clientLogin" || msgType == "joinGroup") {
        needEncrypt = false;
      }
      */
    }
    else if (jMsg.isMember("cmd")) {
      std::string cmd = jMsg["cmd"].asString();
      if (cmd == "ping" || cmd == "OnClientLogin" || cmd == "onJoinGroup") {
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
void CTcpPacketAccept::SendData_string(std::string & data) {
    PACKET_HEADER header;
    header.size = htons(sizeof(PACKET_HEADER)+data.length()+1);
    SendData((const unsigned char*)(&header),sizeof(PACKET_HEADER));
    SendData((const unsigned char*)data.c_str(),data.length()+1);
}

void CTcpPacketAccept::OnRecv_reqMember(Json::Value & jMsg) {
    unsigned long seqFrom = jMsg["seq"].asInt();
    char sql[256];
    sprintf(sql, "select [id],[name],[type],[status],[create_time],[seq],[attr] from t_member where seq>%lu order by seq limit 100;", seqFrom);
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    Json::Value jRec, jArray, jRsp;
    std::string err = grpObj->selectDB((const char*)sql,jRec);
    for (int i = 0; i < jRec.size(); i++) {
        Json::Value row = jRec[i];
        jRec[i]["name"] = grpObj->msgDecrypt(row["name"].asString().c_str(), "0123456789");
        jRec[i]["attr"] = grpObj->msgDecrypt(row["attr"].asString().c_str(), "0123456789");
    }
    jRsp["cmd"] = "onReqMember";
    jRsp["members"] = jRec;
    jRsp["maxSeq"] = grpObj->getMemberSeq();
    //std::string data = jRsp.toFastString();
    SendData_json(jRsp);
}

void CTcpPacketAccept::OnRecv_reqMessage(Json::Value & jMsg) {
    unsigned long seqFrom = jMsg["seq"].asInt();
    char sql[256];
    sprintf(sql, "select [id],[member_id],[msg_time],[msg_type],[msg],[status],[seq],[attr] from t_message where seq>%lu order by seq limit 100;", seqFrom);
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    Json::Value jRec, jArray, jRsp;
    std::string err = grpObj->selectDB((const char*)sql,jRec);
    for (int i = 0; i < jRec.size(); i++) {
        Json::Value row = jRec[i];
        jRec[i]["msg"] = grpObj->msgDecrypt(row["msg"].asString().c_str(), row["msg_time"].asString().c_str());
    }
    jRsp["cmd"] = "onReqMessage";
    jRsp["msgs"] = jRec;
    jRsp["maxSeq"] = grpObj->getMsgSeq();
    //std::string data = jRsp.toFastString();
    SendData_json(jRsp);
}

void CTcpPacketAccept::OnRecv_clientLogin(Json::Value & jMsg) {
    std::string loginCreateTime = jMsg["createTime"].asString();
    std::string loginKey = jMsg["key"].asString();
    int memberId = jMsg["memberId"].asInt();
    CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
    bool bVerify = pServer->DecryptClientLogin(loginCreateTime.c_str(), memberId, loginKey.c_str());
    m_bLogined = bVerify;

    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    int onlineMemberCount = bVerify ? grpObj->updateOnlineMemberIds(memberId, true, m_tmConnect) : 0;
    m_client_memberid = memberId;
    
    {
        Json::Value jRsp;
        jRsp["cmd"]  = "OnClientLogin";
        jRsp["verify"]  = bVerify?1:0;
        if (bVerify) {
            //CGroupObj* grpObj = (CGroupObj*)m_grpObj;
            jRsp["memberSeq"] = grpObj->getMemberSeq();
            jRsp["msgSeq"] = grpObj->getMsgSeq();

            randString(8, m_encryptKey);
            jRsp["ek1"] = m_encryptKey;
            randString(8, m_encryptIv);
            jRsp["ek2"] = m_encryptIv;

            jRsp["w1"] = grpObj->getWebKey();
            jRsp["w2"] = grpObj->getWebIv();
        }
        SendData_json(jRsp);
    }

    if (bVerify) {
        Json::Value jAddOnlineMember;
        jAddOnlineMember["cmd"]  = "addOnlineMember";
        jAddOnlineMember["online"] = onlineMemberCount;
        jAddOnlineMember["memberCount"] = pServer->getMemberCount();
        jAddOnlineMember["addmemberId"] = memberId;
        jAddOnlineMember["tmConnect"] = m_tmConnect;
        jAddOnlineMember["add"] = true;
        pServer->broadcast(jAddOnlineMember);
    }

    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "OnClientLogin";
        jVal["grpKey"] = grpObj->getGroupKey();
        jVal["verify"] = bVerify;
        jVal["side"] = "server";
        on_event(jVal.toFastString().c_str());
    }
}

void CTcpPacketAccept::OnRecv_joinGroup(Json::Value & jMsg) {
    std::string cipher = jMsg["cipher"].asString();
    CAESCrypt aes_member;
    CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
    aes_member.SetCryptKey((char*)pServer->get_AES_member_key().c_str());
    aes_member.SetCryptIV((char*)pServer->get_AES_member_iv().c_str());
    char cmp[128];
    int plainText_len = 0;
    char * plainText = (char *)aes_member.DecryptHexStr(cipher.c_str(), cipher.length(), plainText_len);
    plainText[plainText_len-1]=0;
    sprintf(cmp, "%s.", MEMBER_QRCODE_ENCRYPT_KEY);
    bool bVerify = (strstr(plainText, cmp) != NULL);
    delete [] plainText;

    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    int onlineMemberCount = 0;
    unsigned int nextMemberId = 0;

    {
        Json::Value jRsp, jNewMember;
        jRsp["cmd"]  = "onJoinGroup";
        jRsp["verify"]  = bVerify;
        if (bVerify) {
            randString(8, m_encryptKey);
            jRsp["ek1"] = m_encryptKey;
            randString(8, m_encryptIv);
            jRsp["ek2"] = m_encryptIv;
            jRsp["w1"] = grpObj->getWebKey();
            jRsp["w2"] = grpObj->getWebIv();
            //CGroupObj* grpObj = (CGroupObj*)m_grpObj;
            nextMemberId = grpObj->getMemberId(true);
            unsigned int prevMemberSeq = grpObj->getMemberSeq();
            unsigned int nextMemberSeq = grpObj->getMemberSeq(true);
          
            unsigned long long create_time = getTimeStampMS();
            char szRand[6];
            randString(5, szRand);
            CAESCrypt aes_member_en;
            aes_member_en.SetCryptKey((char*)pServer->get_AES_member_key().c_str());
            char plainKey[64], iv[64];
            sprintf(iv, "%llu%u%s", create_time, nextMemberId, pServer->get_AES_member_iv().c_str());
            aes_member_en.SetCryptIV(iv);
            sprintf(plainKey, "%s.%llu.%s", MEMBER_ENCRYPT_KEY, create_time, szRand);
            int cipherKey_len = 0;
            unsigned char * cipherKey = aes_member_en.Encrypt((unsigned char *)plainKey, strlen(plainKey)+1, cipherKey_len);
            jRsp["key"] = aes_member_en.parseByte2HexStr(cipherKey, cipherKey_len);
            delete [] cipherKey;
            sprintf(plainKey, "%llu", create_time);
            jRsp["createTime"] = plainKey;
            
            Json::Value jAttr;
            jAttr["name"] = jMsg["name"].asString();
            jAttr["icon"] = jMsg["icon"];

            std::stringstream ssql;
            ssql << "(";
            ssql << nextMemberId << ",";
            ssql << "'" << grpObj->msgEncrypt(jMsg["name"].asString().c_str(), "0123456789") << "',";
            ssql << "'" << jRsp["key"].asString() << "',";
            ssql << MEMBER_TYPE_MEMBER << ",";
            ssql << MEMBER_STATUS_VALID << ",";
            ssql << jRsp["createTime"].asString() << ",";
          
            ssql << nextMemberSeq << ",";
            ssql << "'" << grpObj->msgEncrypt(jAttr.toFastString().c_str(), "0123456789") << "');";
          
            jRsp["memberId"] = nextMemberId;
            jRsp["memberType"] = MEMBER_TYPE_MEMBER;
            
            Json::Value jMemberValues;
            jMemberValues["id"] = nextMemberId;
            jMemberValues["name"] = jMsg["name"].asString();
            jMemberValues["key"] = jRsp["key"].asString();
            jMemberValues["type"] = MEMBER_TYPE_MEMBER;
            jMemberValues["status"] = MEMBER_STATUS_VALID;
            jMemberValues["create_time"] = jRsp["createTime"].asString();
            jMemberValues["seq"] = nextMemberSeq;
            jMemberValues["attr"] = jAttr.toFastString();

            jNewMember["cmd"] = "newMember";
            jNewMember["id"] = nextMemberId;
            //jNewMember["memberType"] = MEMBER_TYPE_MEMBER;
            jNewMember["prevSeq"] = prevMemberSeq;
            jNewMember["seq"] = nextMemberSeq;
            jNewMember["memberValues"] = jMemberValues;
            
            std::string sql = "replace into t_member([id],[name],[key],[type],[status],[create_time],[seq],[attr]) values" + ssql.str();
            std::string err = grpObj->execDB(sql.c_str());
            m_bLogined = bVerify;

            pServer->updateMemberCount();
            onlineMemberCount = grpObj->updateOnlineMemberIds(nextMemberId, true, m_tmConnect);
            m_client_memberid = nextMemberId;
        }

        SendData_json(jRsp);
      if (bVerify) {
        pServer->broadcast(jNewMember);

        Json::Value jAddOnlineMember;
        jAddOnlineMember["cmd"]  = "addOnlineMember";
        jAddOnlineMember["online"] = onlineMemberCount;
        jAddOnlineMember["memberCount"] = pServer->getMemberCount();
        jAddOnlineMember["addmemberId"] = nextMemberId;
        jAddOnlineMember["tmConnect"] = m_tmConnect;
        jAddOnlineMember["add"] = true;
        pServer->broadcast(jAddOnlineMember);
      }
    }

    if (on_event) {
        Json::Value jVal;
        jVal["action"] = "onJoinGroup";
        jVal["verify"] = bVerify;
        jVal["grpKey"] = grpObj->getGroupKey();
        //jVal["plainText"]  = plainText;
        //jVal["cipher"]  = cipher;
        jVal["side"] = "server";
        on_event(jVal.toFastString().c_str());
    }
}

void CTcpPacketAccept::OnRecv_addEmoji(Json::Value & jMsg) {
    unsigned long msgId = jMsg["msgId"].asInt();
    int memberId = jMsg["memberId"].asInt();
    std::string emoji = jMsg["emoji"].asString();
    char sql[1024];
    sprintf(sql, "select [id],[member_id],[msg_time],[msg_type],[msg],[status],[seq],[attr] from t_message where [id]=%lu;", msgId);
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    Json::Value jRec, jArray, jRsp;
    std::string err = grpObj->selectDB((const char*)sql,jRec);
    if (err == "") {
        if (jRec.size() == 1) {
            Json::Value jRow = jRec[0];
            std::string msg = grpObj->msgDecrypt( jRow["msg"].asString().c_str(), jRow["msg_time"].asString().c_str() );
            Json::Reader reader;
            Json::Value jOriMsg;
            if (reader.parse(msg.c_str(), jOriMsg)) {
                Json::Value jEmojis;
                if (jOriMsg.isMember("emojis")) {
                    jEmojis = jOriMsg["emojis"];
                }
                else {
                    Json::Reader reader1;
                    reader1.parse("[]", jEmojis);
                }
                bool hasMemberId = false;
                for (int i = 0; i < jEmojis.size(); i++) {
                    if (jEmojis[i]["memberId"].asInt() == memberId) {
                        jEmojis[i]["emoji"] = emoji;
                        hasMemberId = true;
                        break;
                    }
                }
                if (hasMemberId == false) {
                    Json::Value jItem;
                    jItem["memberId"] = memberId;
                    jItem["emoji"] = emoji;
                    jEmojis.append(jItem);
                }
                jOriMsg["emojis"] = jEmojis;

                unsigned int maxMsgSeq;
                unsigned int prevMsgSeq = grpObj->getMsgSeq(&maxMsgSeq);
                //sprintf(sql, "update t_message set msg='%s',[seq]=%u where id=%lu;", escapeSQLite(jOriMsg.toFastString()).c_str(), maxMsgSeq, msgId);
                std::stringstream ssql;
                ssql << "update t_message set msg='" << grpObj->msgEncrypt(jOriMsg.toFastString().c_str(), jRow["msg_time"].asString().c_str())
							<< "', [seq]=" << maxMsgSeq << " where id=" << msgId << ";";
                err = grpObj->execDB(ssql.str().c_str());
                if (err == "") {
                    Json::Value jCmd;
                    jCmd["cmd"] = "updateMsg";
                    jCmd["prevSeq"] = prevMsgSeq;
                    jCmd["seq"] = maxMsgSeq;
                    CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
                    pServer->broadcast(jCmd);
                }
            }
        }
    }
}

void CTcpPacketAccept::OnRecv_removeEmoji(Json::Value & jMsg) {
    unsigned long msgId = jMsg["msgId"].asInt();
    int memberId = jMsg["memberId"].asInt();
    std::string emoji = jMsg["emoji"].asString();
    char sql[1024];
    sprintf(sql, "select [id],[member_id],[msg_time],[msg_type],[msg],[status],[seq],[attr] from t_message where [id]=%lu;", msgId);
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    Json::Value jRec, jArray, jRsp;
    std::string err = grpObj->selectDB((const char*)sql,jRec);
    if (err == "") {
        if (jRec.size() == 1) {
            Json::Value jRow = jRec[0];
            std::string msg = grpObj->msgDecrypt( jRow["msg"].asString().c_str(), jRow["msg_time"].asString().c_str() );;
            Json::Reader reader;
            Json::Value jOriMsg;
            if (reader.parse(msg.c_str(), jOriMsg)) {
                Json::Value jEmojis, jEmojisNew;
                if (jOriMsg.isMember("emojis")) {
                    jEmojis = jOriMsg["emojis"];
                }
                else {
                    Json::Reader reader1;
                    reader1.parse("[]", jEmojis);
                }
                bool hasMemberId = false;
                Json::Reader reader2;
                reader2.parse("[]", jEmojisNew);
                for (int i = 0; i < jEmojis.size(); i++) {
                    if (jEmojis[i]["memberId"].asInt() == memberId) {
                        hasMemberId = true;
                    }
                    else {
                        jEmojisNew.append(jEmojis[i]);
                    }
                }
                if (hasMemberId == false) {
                    return;
                }
                jOriMsg["emojis"] = jEmojisNew;

                unsigned int maxMsgSeq;
                unsigned int prevMsgSeq = grpObj->getMsgSeq(&maxMsgSeq);
                //sprintf(sql, "update t_message set msg='%s',[seq]=%u where id=%lu;", escapeSQLite(jOriMsg.toFastString()).c_str(), maxMsgSeq, msgId);
                std::stringstream ssql;
                ssql << "update t_message set msg='" << grpObj->msgEncrypt(jOriMsg.toFastString().c_str(), jRow["msg_time"].asString().c_str())
							<< "', [seq]=" << maxMsgSeq << " where id=" << msgId << ";";
                err = grpObj->execDB(ssql.str().c_str());
                if (err == "") {
                    Json::Value jCmd;
                    jCmd["cmd"] = "updateMsg";
                    jCmd["prevSeq"] = prevMsgSeq;
                    jCmd["seq"] = maxMsgSeq;
                    CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
                    pServer->broadcast(jCmd);
                }
            }
        }
    }
}

void CTcpPacketAccept::OnRecv_deleteMsg(Json::Value & jMsg) {
    int msgId = jMsg["msgId"].asInt();
    int msg_type = jMsg["msg_type"].asInt();
    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    unsigned int maxMsgSeq;
    unsigned int prevMsgSeq = grpObj->getMsgSeq(&maxMsgSeq);
    char sql[256];
    sprintf(sql, "update t_message set [seq]=%u, [status]=%d where [id]=%d;", maxMsgSeq, MSG_STATUS_DELETED, msgId);
    std::string err = grpObj->execDB((const char*)sql);
    if (err == "") {
        Json::Value jCmd;
        CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
        jCmd["cmd"] = "updateMsg";
        jCmd["prevSeq"] = prevMsgSeq;
        jCmd["seq"] = maxMsgSeq;
        pServer->broadcast(jCmd);
        if (msg_type == MSG_TYPE_FILE) {
            jCmd.clear();
            jCmd["cmd"] = "deleteMsgFile";
            jCmd["msgId"] = msgId;
            pServer->broadcast(jCmd);
        }
    }
}

void CTcpPacketAccept::OnRecv_sendMsg(Json::Value & jMsg) {
    Json::Value jRsp, jNewMsg;
    Json::Value JMsgTime = jMsg["msgTime"]; // 'ms'
    Json::Value JMsgType = jMsg["msgType"]; // 1
    Json::Value JMemberId = jMsg["memberId"]; // 1
    Json::Value JMsbObj = jMsg["msg"]; // 

    CGroupObj* grpObj = (CGroupObj*)m_grpObj;
    unsigned int maxMsgId = grpObj->getMsgId(true);
    unsigned int prevMsgSeq = grpObj->getMsgSeq();
    unsigned int maxMsgSeq = grpObj->getMsgSeq(true);
    std::stringstream ssql;
    ssql << "(";
    ssql << maxMsgId << ",";
    ssql << JMemberId.asInt() << ",";
    ssql << JMsgTime.asString() << ",";
    ssql << JMsgType.asInt() << ",";
    ssql << "'" << grpObj->msgEncrypt( JMsbObj.asString().c_str(), JMsgTime.asString().c_str() ) << "',";
    ssql << MSG_STATUS_VALID << ",";
    ssql << maxMsgSeq << ",";
    ssql << "'" << "{}" << "');";

    Json::Value jMsgValues;
    jMsgValues["id"] = maxMsgId;
    jMsgValues["member_id"] = JMemberId.asInt();
    jMsgValues["msg_time"] = JMsgTime.asString();
    jMsgValues["msg_type"] = JMsgType.asInt();
    jMsgValues["msg"] = JMsbObj.asString();
    jMsgValues["status"] = MSG_STATUS_VALID;
    jMsgValues["seq"] = maxMsgSeq;
    jMsgValues["attr"] = "{}";


    std::string sql = "replace into t_message([id],[member_id], [msg_time],[msg_type],[msg],[status],[seq],[attr]) values" + ssql.str();
    std::string err = grpObj->execDB(sql.c_str());

    jRsp["cmd"]  = "onSendMsg";
    SendData_json(jRsp);
    
    jNewMsg["cmd"] = "newMsg";
    jNewMsg["id"] = maxMsgId;
    jNewMsg["msgTime"] = JMsgTime.asString();
    jNewMsg["prevSeq"] = prevMsgSeq;
    jNewMsg["seq"] = maxMsgSeq;
    //jNewMsg["sqlValues"] = ssql.str();
    jNewMsg["msgValues"] = jMsgValues;
    CTcpPacketServer * pServer = (CTcpPacketServer*)m_pServer;
    pServer->broadcast(jNewMsg);
}
