#pragma once

#include "mongoose/mongoose.h"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

//#include <thread>

/*
static int s_debug_level = MG_LL_INFO;
static const char *s_user = "user";
static const char *s_pass = "pass";
static const char *s_fname = NULL;
static struct mg_fd *fd;  // file descriptor
static size_t fsize;
static const char *s_url = "http://localhost:8090/upload/foo.txt";
static const uint64_t s_timeout_ms = 1500;  // Connect timeout in milliseconds

*/

class CMongooseWebClient
{
public:
	CMongooseWebClient() {}
	CMongooseWebClient(const char* user, const char* pass, uint64_t timeout_ms = 1500, int max_size = 10000) {
		s_debug_level = MG_LL_INFO;
		s_max_size = max_size;
		//s_root_dir = root_dir;
		//s_upld_dir = uplod_dir;
		//s_listening_address = listening_addr;
		s_user = user;
		s_pass = pass;
		//s_signo = 0;
		s_timeout_ms = 1500;

		fd = nullptr;
		fsize = 0;
		done = false;
	}

	void init(const char* user, const char* pass, uint64_t timeout_ms = 1500, int max_size = 10000) {
		s_debug_level = MG_LL_INFO;
		s_max_size = max_size;
		//s_root_dir = root_dir;
		//s_upld_dir = uplod_dir;
		//s_listening_address = listening_addr;
		s_user = user;
		s_pass = pass;
		//s_signo = 0;
		s_timeout_ms = 1500;

		fd = nullptr;
		fsize = 0;
		done = false;
	}

	std::string uploadFile(const char* s_fname, const char* url);
	std::string downloadFile(const char* url, const char* filename, const char* s_fname);
private:
	//void threadFunc();

	//upload
	static void fn(struct mg_connection* c, int ev, void* ev_data);
	void handle_fn(struct mg_connection* c, int ev, void* ev_data);

	//download
	static void fn1(struct mg_connection* c, int ev, void* ev_data);
	void handle_fn1(struct mg_connection* c, int ev, void* ev_data);
private:
	//std::thread m_thread;
	int s_debug_level;
	int s_max_size;
	std::string s_user;
	std::string s_pass;
	uint64_t s_timeout_ms;
	size_t fsize;
	struct mg_fd* fd;  // file descriptor
	std::string s_url;
	bool done;
	std::stringstream downloadRet;
	std::stringstream uploadRet;

	std::ofstream fileStream_; // save the downloaded file
	std::string download_file_name;
};

