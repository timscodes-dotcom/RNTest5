#include <jni.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST

#include <netinet/in.h>

#include "json/json.h"
#include "md5/md5.h"
//https://blog.csdn.net/tantion/article/details/84936093?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task
//https://blog.csdn.net/ywl5320/article/details/78739211?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task

#include "key.h"
#include "TcpClient.h"
#include "functions.h"

JavaVM *g_VM = NULL;
pid_t g_main_tid = 0;
JNIEnv * g_env = NULL;
jobject g_object = NULL;
jmethodID g_javaOnTestMessageId = NULL;
jmethodID g_javaOnUdpEventId = NULL;

std::string g_ip = "";
int g_port = 0;
std::string g_dir = "";
std::string g_packageName = "";

std::string g_deviceID = "";
CMongooseWebServer* g_webServer = nullptr;

bool ensureDirExists(const std::string& path, std::string& err) {
        int nRC = mkdir(path.c_str(), 0777);
        if (nRC == -1 && errno != EEXIST) {
                err = "mkdir fail: ";
                err += path;
                return false;
        }
        return true;
}

std::string writeDefaultWebRoot(const std::string& webRoot, const std::string& uploadDir) {
        std::string err;
        if (!ensureDirExists(webRoot, err)) return err;

        std::string indexFile = webRoot + "/index.html";
        FILE* fp = fopen(indexFile.c_str(), "w");
        if (fp == NULL) {
                return "open index.html fail";
        }

        std::string html = R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>RN 上传下载</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 16px; }
        .tips { color: #666; font-size: 12px; margin-bottom: 10px; }
        button { padding: 8px 12px; }
    </style>
</head>
<body>
    <h2>文件上传</h2>
    <div class="tips">上传到 APP 目录，下载路径: /download?文件名</div>
    <input id="f" type="file" />
    <button onclick="upl()">上传</button>
    <div id="msg"></div>
    <script>
        async function upl() {
            const fi = document.getElementById('f');
            if (!fi.files || fi.files.length === 0) return;
            const file = fi.files[0];
            const data = await file.arrayBuffer();
            const bytes = new Uint8Array(data);
            let binary = '';
            for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
            const res = await fetch('/upload/' + encodeURIComponent(file.name) + '?msgid=0&idx=0', {
                method: 'POST',
                headers: {'Content-Type': 'application/octet-stream'},
                body: binary
            });
            document.getElementById('msg').innerText = await res.text();
        }
    </script>
</body>
</html>)HTML";

        fwrite(html.c_str(), 1, html.length(), fp);
        fclose(fp);
        return "";
}

std::string startGlobalWebServer(Json::Value& root) {
        Json::Value jRet;

        std::string ip = root.isMember("ip") ? root["ip"].asString() : "";
        int port = root.isMember("port") ? root["port"].asInt() : 8899;
        if (ip == "") ip = "0.0.0.0";
        if (port <= 0) port = 8899;

        if (g_dir == "") {
                jRet["ret"] = "filesDir empty, call init first";
                return jRet.toFastString();
        }

        std::string uploadDir = g_dir + "/files";
        std::string webRoot = g_dir + "/web_root";

        std::string err;
        if (!ensureDirExists(uploadDir, err)) {
                jRet["ret"] = err;
                return jRet.toFastString();
        }

        err = writeDefaultWebRoot(webRoot, uploadDir);
        if (err != "") {
                jRet["ret"] = err;
                return jRet.toFastString();
        }

        if (g_webServer) {
                g_webServer->stopWebServer();
                delete g_webServer;
                g_webServer = nullptr;
        }

        char addr[128];
        snprintf(addr, sizeof(addr), "http://%s:%d", ip.c_str(), port);
        g_webServer = new CMongooseWebServer(webRoot.c_str(), uploadDir.c_str(), addr, "", "", false, 1024 * 1024 * 200);
        g_webServer->startWebServer();

        jRet["ret"] = "ok";
        jRet["url"] = addr;
        jRet["rootDir"] = webRoot;
        jRet["uploadDir"] = uploadDir;
        return jRet.toFastString();
}

std::string stopGlobalWebServer() {
        Json::Value jRet;
        if (g_webServer) {
                g_webServer->stopWebServer();
                delete g_webServer;
                g_webServer = nullptr;
        }
        jRet["ret"] = "ok";
        return jRet.toFastString();
}

#ifndef _NEW_MD5_LIB
void CountMD5(unsigned char* strin,unsigned int ilen,char* strout) //strout的长度必须大于32
{
	MD5_CTX m_md5;

	MD5Init(&m_md5);
	MD5Update(&m_md5,strin,ilen);
	MD5Final(&m_md5);

	char* pchar = strout;

	// Get each chunk of the hash and append it to the output
	for ( int i = 0 ; i < 16 ; i++ )
	{
	/*
		//char tmp[3];
		itoa(m_md5.digest[i], pchar , 16);
		//sprintf(pchar, "%x", m_md5.digest[i]);

		if (strlen(pchar) == 1)
		{
			pchar[1] = pchar[0];
			pchar[0] = '0';
			pchar[2] = '\0';
		}
	*/
	    unsigned char v1 = m_md5.digest[i] / 16;
	    unsigned char v2 = m_md5.digest[i] % 16;
	    pchar[0] = v1 > 9 ? (v1-10+'a') : (v1+'0');
	    pchar[1] = v2 > 9 ? (v2-10+'a') : (v2+'0');
        pchar[2] = '\0';
		pchar = pchar+2;
	}

	*pchar = 0;
}
#endif

void MD5Pwd1(const char * _in, char * pwd)
{
	char tmp[128];
	//sprintf(tmp,"qeWS?%s@34fFN", _in);
    //sprintf(tmp,"987p:Dw%sM#125", _in);
    sprintf(tmp,MD5_LOGIN_KEY, _in);
#ifndef _NEW_MD5_LIB
	CountMD5((unsigned char *)tmp, strlen(tmp), pwd);
#else
    MD5 md5 = MD5(tmp);
    std::string md5Result = md5.hexdigest();
    strcpy(pwd, md5Result.c_str());
#endif
}


extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniMD5Pwd1(
        JNIEnv *env,
        jobject /* this */ obj,
        jstring _in)
{
    const char* temp1 = env->GetStringUTFChars(_in, NULL);

	char _pwd[128];
    MD5Pwd1(temp1, _pwd);
    return env->NewStringUTF(_pwd);

}

void onTestMessage(const char * msg/*, int len*/)
{
/*
    //printf(">>> %s\n", msg);
    if (g_VM && g_object && g_javaOnTestMessageId) {
        JNIEnv *env;
        g_VM->AttachCurrentThread(&env, 0);

        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(g_object, g_javaOnTestMessageId, 0, jmsg);
        env->DeleteLocalRef(jmsg);

        g_VM->DetachCurrentThread();
    }
*/
	pid_t tid = gettid();

    if (g_main_tid == tid) {
        if (g_env && g_object && g_javaOnTestMessageId) {
            jstring jmsg = g_env->NewStringUTF(msg);
            g_env->CallVoidMethod(g_object, g_javaOnTestMessageId,0,jmsg);
            g_env->DeleteLocalRef(jmsg);
        }
    }
    else {
        if (g_VM && g_object && g_javaOnTestMessageId) {
            JNIEnv *env;
            g_VM->AttachCurrentThread(&env, 0);

            jstring jmsg = env->NewStringUTF(msg);
            env->CallVoidMethod(g_object, g_javaOnTestMessageId, 0, jmsg);
            env->DeleteLocalRef(jmsg);

            g_VM->DetachCurrentThread();
        }
    }
}

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniOnLoad(
        JNIEnv* env,
        jobject /* this */) 
{
    //if(!g_VM)(*env).GetJavaVM(&g_VM);

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());

}

/*
extern "C"  JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,void* reserved)
{
    JNIEnv *env;
    g_VM = vm;
    if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK)
    {
        //LOG("#### JNI_OnLoad fail\n");
        return -1;
    }
    //LOGD("#### JNI_OnLoad succ\n");
    return JNI_VERSION_1_6;
}
*/

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    //WebSocket * socket = new WebSocket();
    //delete socket;

    Json::Value user;
    user["id"] = 0;
    user["name"] = "1";

    std::string hello = "Hello from C++";

    std::string test1 = "stringFromJNI: onTestMessage";
    onTestMessage(test1.c_str()/*, test1.length()+1*/);
    return env->NewStringUTF(hello.c_str());
}

long long __jniHton64i(long long i)
{
	long long iRet;
	int* pSrc = (int*)&i;
	int * pDes= (int*)&iRet;

	pDes[0] = htonl(pSrc[1]);
	pDes[1] = htonl(pSrc[0]);

	return iRet;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniHton64i(
        JNIEnv* env,
        jobject /* this */ obj,
		jstring value) {
	const char* temp = env->GetStringUTFChars(value, NULL);
	long long v = atoll(temp);
	long long v1 = __jniHton64i(v);
	char ret[32];
	sprintf(ret,"%lld", v1);
	return env->NewStringUTF(ret);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniTestWriteFile(
        JNIEnv* env,
        jobject /* this */ obj,
		jstring dir) {

	char buffer[256];
	if (true){
		const char* temp = env->GetStringUTFChars(dir, NULL);
		sprintf(buffer, "%s/test.txt", temp);

		FILE * fp = fopen(buffer, "w");
		if (fp == NULL) {
			strcpy(buffer, "###fopen fail");
		}
		else {
			fclose(fp);
		}
		
		onTestMessage("test msg"/*, 8*/);
	}
    return env->NewStringUTF((const char *)buffer);
}

std::string initFolders(const char * path)
{
	std::string file = path;
  file += "/test.txt";
  FILE * fp = fopen(file.c_str(), "w");
  std::string ret = "";//"write file ok";
  if (fp == NULL) {
    ret = "write file fail";
  }
  else {
    fclose(fp);
  }
  
  if (ret != "") return ret;
  
  //create files folder
  file = path;
  file += "/files";
  int nRC = mkdir( file.c_str(), 0777 );
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
  }
  //else
  //    bSuccess = true;
  
  if (ret != "") return ret;
  
  //test create file again
  file = path;
  file += "/files/test.txt";
  fp = fopen(file.c_str(), "w");
  if (fp == NULL) {
    ret = "write file in files folder fail";
  }
  else {
    fclose(fp);
  }
  
  return ret;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniSetCallback(
        JNIEnv *env,
        jobject thiz,
        jstring packageName,
        jstring dir)
{
    //LOG("NDKhelp_setOnMessage");

    const char* temp1 = env->GetStringUTFChars(packageName, NULL);
    g_packageName = temp1;
    const char* temp2 = env->GetStringUTFChars(dir, NULL);
    g_dir = temp2;

    //JavaVM是虚拟机在JNI中的表示，等下再其他线程回调java层需要用到
    //if(!g_VM)(*env).GetJavaVM(&g_VM);
	if (!g_env) g_env = env;
	if (!g_object) g_object = env->NewGlobalRef(thiz);

    //生成一个全局引用，回调的时候findclass才不会为null
    //jobject callback = (*env).NewGlobalRef(jcallback);

    // 把接口传进去，或者保存在一个结构体里面的属性， 进行传递也可以
    // pthread_create(xxx,xxx, download,callback);

    func_init();

    jclass javaClass = env->GetObjectClass(thiz);
    if (javaClass == 0) {
        //LOG("Unable to find class");
        //(*g_VM)->DetachCurrentThread(g_VM);
        return env->NewStringUTF("### GetObjectClass fail");

    }

    //获取要回调的方法ID
	g_javaOnTestMessageId = env->GetMethodID(javaClass, "onTestMessage", "(ILjava/lang/String;)V");
    if (g_javaOnTestMessageId == NULL) {
        //LOGD("Unable to find method:OnTestMessage");
        return env->NewStringUTF("### GetMethodID OnTestMessage fail");
    }
    /*
    g_javaOnUdpEventId = env->GetMethodID(javaClass, "onUdpEvent", "(ILjava/lang/String;)V");
    if (g_javaOnUdpEventId == NULL) {
        //LOGD("Unable to find method:OnTestMessage");
        return env->NewStringUTF("### GetMethodID onUdpEvent fail");
    }
    */
	/*
    g_javaOnMessageId = env->GetMethodID(javaClass, "onMessage", "(ILjava/lang/String;)V");
    if (g_javaOnMessageId == NULL) {
        //LOGD("Unable to find method:onMessage");
        return env->NewStringUTF("### GetMethodID onMessage fail");
    }
    g_javaOnConnectId = env->GetMethodID(javaClass, "onConnect", "(ILjava/lang/String;)V");
    if (g_javaOnConnectId == NULL) {
        //LOGD("Unable to find method:onMessage");
        return env->NewStringUTF("### GetMethodID onConnect fail");
    }
    g_javaOnCloseId = env->GetMethodID(javaClass, "onClose", "(ILjava/lang/String;)V");
    if (g_javaOnCloseId == NULL) {
        //LOGD("Unable to find method:onMessage");
        return env->NewStringUTF("### GetMethodID onClose fail");
    }
    g_javaOnErrorId = env->GetMethodID(javaClass, "onError", "(ILjava/lang/String;)V");
    if (g_javaOnErrorId == NULL) {
        //LOGD("Unable to find method:onMessage");
        return env->NewStringUTF("### GetMethodID onError fail");
    }
	*/

    //执行回调
    //env->CallVoidMethod(thiz, g_javaOnMessageId,0,env->NewStringUTF("call back succ"));
    //env->CallStaticVoidMethod(env, javaClass, mid_callback_static);

    g_main_tid = gettid();
    /*g_conn.setEvents(on_connect,on_close,on_error,onMessage, send_heart_beat);
    char err[1024];
    g_conn.connectUrl("ws://demos.kaazing.com/echo", err);
	*/

	std::string folders = initFolders(g_dir.c_str());
	
    return env->NewStringUTF(folders.c_str());

}

extern "C" JNIEXPORT jstring JNICALL Java_com_timscodes_fileshare_MyNativeModule_jniJsonCmd(
        JNIEnv* env,
        jobject /* this */ obj,
		jstring jsonStr) {

	const char* temp = env->GetStringUTFChars(jsonStr, NULL);
	
	Json::Reader reader;
	Json::Value root;
	reader.parse(temp, root);

	Json::Value vCmd = root["cmd"];
	if (vCmd.asString() == "GetLocalIPAddress") {
		return env->NewStringUTF(GetLocalIPAddress().c_str());
	}
    else if (vCmd.asString() == "createGroup") {
       return env->NewStringUTF(func_createGroup1(root, true, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "loadGroup") {
        return env->NewStringUTF(func_createGroup1(root, false, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "deleteGroup") {
       return env->NewStringUTF(func_deleteGroup1(root).c_str());
    }
    else if (vCmd.asString() == "startGroup") {
        return env->NewStringUTF(func_startGroup(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "stopGroup") {
        return env->NewStringUTF(func_stopGroup(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "sendMsg") {
        return env->NewStringUTF(func_sendMsg(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "encryptGroupAddress") {
        return env->NewStringUTF(func_encryptGroupAddress(root).c_str());
    }
    else if (vCmd.asString() == "decryptGroupAddress") {
        return env->NewStringUTF(func_decryptGroupAddress(root).c_str());
    }
    else if (vCmd.asString() == "joinGroup") {
        return env->NewStringUTF(func_joinGroup1(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "getGroupSeqs") {
        return env->NewStringUTF(func_getGroupSeqs(root).c_str());
    }
    else if (vCmd.asString() == "loadMsg") {
        return env->NewStringUTF(func_loadMsg(root).c_str());
    }
    else if (vCmd.asString() == "loadMmember") {
        return env->NewStringUTF(func_loadMember(root).c_str());
    }
    else if (vCmd.asString() == "uploadFile") {
        return env->NewStringUTF(func_uploadFile(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "downloadFile") {
        return env->NewStringUTF(func_downloadFile(root, onTestMessage).c_str());
    }
    else if (vCmd.asString() == "md5Encrypt") {
        return env->NewStringUTF(func_md5Encrypt(root).c_str());
    }
    else if (vCmd.asString() == "md5Verify") {
        return env->NewStringUTF(func_md5Verify(root).c_str());
    }
    else if (vCmd.asString() == "webEncrypt") {
        return env->NewStringUTF(func_webEncrypt(root).c_str());
    }
    else if (vCmd.asString() == "setDeviceID") {
        g_deviceID = root["deviceID"].asString();
        return env->NewStringUTF("");
    }
    else if (vCmd.asString() == "startWebServer") {
        return env->NewStringUTF(startGlobalWebServer(root).c_str());
    }
    else if (vCmd.asString() == "stopWebServer") {
        return env->NewStringUTF(stopGlobalWebServer().c_str());
    }
    else if (vCmd.asString() == "testing") {
        CRITICAL_SECTION1 lock;
        lock.lock();
        lock.unlock();
        return env->NewStringUTF("OK");
    }
    /*
    else if (vCmd.asString() == "createTcpServer") {
        g_tcpServer = new CTcpChatServer();
        g_tcpServer->setEvents(onTcpServerEvent);
        std::string err = "";
        g_tcpServer->Start(5001, 3, err);
        return [NSString stringWithFormat:@"%s", err.c_str()];
    }
    else if (vCmd.asString() == "connectTcpServer") {
        g_tcpClient = new CTcpClient();
        std::string err = "";
        Json::Value vIp = root["ip"];
        Json::Value vPort = root["port"];
        g_tcpClient->Connect(vIp.asString().c_str(), vPort.asInt(), err);
        return [NSString stringWithFormat:@"%s", err.c_str()];
    }
    */

    std::string err = "unknownCmd";
    return env->NewStringUTF(err.c_str());
}
