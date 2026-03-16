#pragma once

#include "mongoose/mongoose.h"
#include "CRITICAL_SECTION.h"
#include <string>
#include <thread>
#include <unordered_map>

/*
static int s_debug_level = MG_LL_INFO;
static int s_max_size = 10000;
static const char* s_root_dir = "web_root";
static const char* s_upld_dir = "upload";
static const char* s_listening_address = "http://0.0.0.0:8090";
static const char* s_user = "user";
static const char* s_pass = "pass";

static int s_signo;
*/


#ifdef TLS_TWOWAY
static const char* s_tls_ca =
"-----BEGIN CERTIFICATE-----\n"
"MIIBFTCBvAIJAMNTFtpfcq8NMAoGCCqGSM49BAMCMBMxETAPBgNVBAMMCE1vbmdv\n"
"b3NlMB4XDTI0MDUwNzE0MzczNloXDTM0MDUwNTE0MzczNlowEzERMA8GA1UEAwwI\n"
"TW9uZ29vc2UwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASuP+86T/rOWnGpEVhl\n"
"fxYZ+pjMbCmDZ+vdnP0rjoxudwRMRQCv5slRlDK7Lxue761sdvqxWr0Ma6TFGTNg\n"
"epsRMAoGCCqGSM49BAMCA0gAMEUCIQCwb2CxuAKm51s81S6BIoy1IcandXSohnqs\n"
"us64BAA7QgIgGGtUrpkgFSS0oPBlCUG6YPHFVw42vTfpTC0ySwAS0M4=\n"
"-----END CERTIFICATE-----\n";
#endif
static const char* s_tls_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIIBMTCB2aADAgECAgkAluqkgeuV/zUwCgYIKoZIzj0EAwIwEzERMA8GA1UEAwwI\n"
"TW9uZ29vc2UwHhcNMjQwNTA3MTQzNzM2WhcNMzQwNTA1MTQzNzM2WjARMQ8wDQYD\n"
"VQQDDAZzZXJ2ZXIwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASo3oEiG+BuTt5y\n"
"ZRyfwNr0C+SP+4M0RG2pYkb2v+ivbpfi72NHkmXiF/kbHXtgmSrn/PeTqiA8M+mg\n"
"BhYjDX+zoxgwFjAUBgNVHREEDTALgglsb2NhbGhvc3QwCgYIKoZIzj0EAwIDRwAw\n"
"RAIgTXW9MITQSwzqbNTxUUdt9DcB+8pPUTbWZpiXcA26GMYCIBiYw+DSFMLHmkHF\n"
"+5U3NXW3gVCLN9ntD5DAx8LTG8sB\n"
"-----END CERTIFICATE-----\n";

static const char* s_tls_key =
"-----BEGIN EC PRIVATE KEY-----\n"
"MHcCAQEEIAVdo8UAScxG7jiuNY2UZESNX/KPH8qJ0u0gOMMsAzYWoAoGCCqGSM49\n"
"AwEHoUQDQgAEqN6BIhvgbk7ecmUcn8Da9Avkj/uDNERtqWJG9r/or26X4u9jR5Jl\n"
"4hf5Gx17YJkq5/z3k6ogPDPpoAYWIw1/sw==\n"
"-----END EC PRIVATE KEY-----\n";


struct upload_file_info{
    char oriFileName[MG_PATH_MAX];
    char fileName[MG_PATH_MAX];
    int msgId;
    int idx;
};

class CMongooseWebServer
{
public:
	CMongooseWebServer() {
		on_event = nullptr;
		m_pEventHandler = nullptr;
	}
	CMongooseWebServer(const char * root_dir, const char * uplod_dir, const char * listening_addr, const char * user, const char * pass, bool is_https = false, int max_size=10000) {
		s_debug_level = MG_LL_INFO;
		m_is_https = is_https;
		s_max_size = max_size;
		s_root_dir = root_dir;
		s_upld_dir = uplod_dir;
		s_listening_address = listening_addr;
		s_user = user;
		s_pass = pass;
		s_signo = 0;
		on_event = nullptr;
		m_pEventHandler = nullptr;
	}

	void init(const char* root_dir, const char* uplod_dir, const char* listening_addr, const char* user, const char* pass, bool is_https = false, int max_size = 10000) {
		s_debug_level = MG_LL_INFO;
		m_is_https = is_https;
		s_max_size = max_size;
		s_root_dir = root_dir;
		s_upld_dir = uplod_dir;
		s_listening_address = listening_addr;
		s_user = user;
		s_pass = pass;
		s_signo = 0;
	}

	~CMongooseWebServer() {
		stopWebServer();
	}
	std::string startWebServer();
	std::string stopWebServer();

	const char* get_root_dir() { return s_root_dir.c_str(); }
	bool isHttps() { return m_is_https; }
	void setEvents(void* handler, bool (*onEvent)(void*, const char*, void*, std::string&))
	{
		m_pEventHandler = handler;
		on_event = onEvent;
	}

	std::string formatUploadFileName(std::string query, std::string apiName, std::unordered_map<std::string, std::string>& params);

private:
	void threadFunc();
	static void cb(struct mg_connection* c, int ev, void* ev_data);
	void handle_uploads(struct mg_connection* c, int ev, void* ev_data);
	void handle_download(struct mg_connection* c, int ev, void* ev_data);
	void handle_api(struct mg_http_message* hm, std::string& apiRet);
	bool authuser(struct mg_http_message* hm);
private:
	std::thread m_thread;
	bool m_is_https;

	int s_debug_level;
	int s_max_size;
	std::string s_root_dir;
	std::string s_upld_dir;
	std::string s_listening_address;
	std::string s_user;
	std::string s_pass;

	int s_signo;

	bool (*on_event)(void*, const char*, void*, std::string&);
	void* m_pEventHandler;

    CRITICAL_SECTION1 m_lock;
    std::unordered_map<uint64_t, struct upload_file_info*> m_uploading;
};

std::string decodeUrl(const std::string& url);