#include "CMongooseWebServer.h"
#include "CRITICAL_SECTION.h"
#include <fstream>
#include <iostream>
#include <vector>

#include <string>
#include <cctype>
#include <sstream>
#include <iomanip>

#include <android/log.h>
#define MWS_LOG_TAG "MyNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, MWS_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MWS_LOG_TAG, __VA_ARGS__)

std::string decodeUrl(const std::string& url) {
    std::string decoded;
    std::istringstream urlStream(url);
    char ch;

    while (urlStream >> std::noskipws >> ch) {
        if (ch == '%') {
            // 读取接下来的两个字符，并将其视为十六进制值
            char hex[3] = {0};
            if (urlStream >> hex[0] >> hex[1] && std::isxdigit(hex[0]) && std::isxdigit(hex[1])) {
                // 将十六进制字符串转换为字符
                decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
            }
        } else if (ch == '+') {
            // `+` 在 URL 编码中表示空格
            decoded += ' ';
        } else {
            decoded += ch;
        }
    }

    return decoded;
}

void parseUrlParams(const std::string url, std::unordered_map<std::string, std::string>& params) {
    //std::unordered_map<std::string, std::string> params;
    std::string key, value;
    std::istringstream urlStream(url);
    
    // 以 '&' 分割每个参数对
    while (std::getline(urlStream, key, '&')) {
        std::istringstream keyValueStream(key);

        // 使用 '=' 分隔键和值
        if (std::getline(keyValueStream, key, '=') && std::getline(keyValueStream, value)) {
            params[key] = value;
        }
    }
    
    //return params;
}

bool CMongooseWebServer::authuser(struct mg_http_message* hm) {
    char user[256], pass[256];
    mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
    if (m_pEventHandler && on_event) {
        std::string apiRet = "ok";
        on_event(m_pEventHandler, "authuser", nullptr, apiRet);
    }
    if (strcmp(user, s_user.c_str()) == 0 && strcmp(pass, s_pass.c_str()) == 0) return true;
    return false;
}

struct upload_state {
    size_t expected;  // POST data length, bytes
    size_t received;  // Already received bytes
    void* fp;         // Opened uploaded file
};

size_t get_content_length(struct mg_http_message* hm) {
    struct mg_str* h = mg_http_get_header(hm, "Content-Length");
    if (h == NULL || h->len == 0) return 0;
    size_t v = 0;
    for (size_t i = 0; i < h->len; ++i) {
        char ch = h->buf[i];
        if (ch < '0' || ch > '9') return 0;
        size_t digit = (size_t)(ch - '0');
        if (v > (SIZE_MAX - digit) / 10) return 0;  // overflow
        v = v * 10 + digit;
    }
    return v;
}

std::string get_mg_str(struct mg_str& src) {
    return std::string(src.buf, src.len);
}

std::string CMongooseWebServer::formatUploadFileName(std::string query, std::string apiName, std::unordered_map<std::string, std::string>& params) {
    //std::unordered_map<std::string, std::string> params;
    //parseUrlParams(query, params);

    char randStr[11];
    randString(10, randStr);

    char fpath[MG_PATH_MAX];
    //msgId = atoi(params["msgid"].c_str());
    //snprintf(fpath, MG_PATH_MAX, "%s%cmsg%s_%s_%s", s_upld_dir.c_str(), MG_DIRSEP, params["msgid"].c_str(), randStr, apiName.c_str());
    snprintf(fpath, MG_PATH_MAX, "%s%c%s", s_upld_dir.c_str(), MG_DIRSEP, apiName.c_str());
    return std::string(fpath);
}

void CMongooseWebServer::handle_uploads(struct mg_connection* c, int ev, void* ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;

    struct mg_fs* fs = &mg_fs_posix;
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    if (!mg_match(hm->uri, mg_str("/upload/#"), NULL)) return;

    size_t contentLen = hm->body.len;
    LOGI("upload parameters: %s, body len: %zu", get_mg_str(hm->query).c_str(), contentLen);

    if (!authuser(hm)) {
        mg_http_reply(c, 403, "", "Denied\n");
        c->is_draining = 1;
        return;
    }
    if (contentLen == 0) {
        mg_http_reply(c, 400, "", "Empty body\n");
        c->is_draining = 1;
        return;
    }
    if (contentLen > (size_t)s_max_size) {
        mg_http_reply(c, 400, "", "Too long\n");
        c->is_draining = 1;
        return;
    }

    std::unordered_map<std::string, std::string> params;
    parseUrlParams(decodeUrl(get_mg_str(hm->query)), params);

    long long offset = 0;
    bool hasOffset = false;
    if (params.find("offset") != params.end()) {
        hasOffset = true;
        offset = atoll(params["offset"].c_str());
        if (offset < 0) {
            mg_http_reply(c, 400, "", "Invalid offset\n");
            c->is_draining = 1;
            return;
        }
    }

    long long totalSize = 0;
    bool hasTotal = false;
    if (params.find("total") != params.end()) {
        hasTotal = true;
        totalSize = atoll(params["total"].c_str());
        if (totalSize <= 0) {
            mg_http_reply(c, 400, "", "Invalid total\n");
            c->is_draining = 1;
            return;
        }
        if ((size_t) totalSize > (size_t) s_max_size) {
            mg_http_reply(c, 400, "", "Too long\n");
            c->is_draining = 1;
            return;
        }
    }

    std::string oriFileName = decodeUrl(std::string(hm->uri.buf + 8, hm->uri.len - 8));
    if (oriFileName.empty()) {
        mg_http_reply(c, 400, "", "Name required\n");
        c->is_draining = 1;
        return;
    }
    if (oriFileName.find("{") != std::string::npos && m_pEventHandler && on_event) {
        on_event(m_pEventHandler, "uploadName", (void*)hm, oriFileName);
        if (oriFileName.empty()) {
            mg_http_reply(c, 400, "", "Name invalid\n");
            c->is_draining = 1;
            return;
        }
    }

    std::string fpath = formatUploadFileName(decodeUrl(get_mg_str(hm->query)), oriFileName, params);
    if (!mg_path_is_sane(mg_str(fpath.c_str()))) {
        mg_http_reply(c, 400, "", "Invalid path\n");
        c->is_draining = 1;
        return;
    }

    if (hasOffset) {
        size_t currentSize = 0;
        fs->st(fpath.c_str(), &currentSize, NULL);
        if (offset == 0) {
            fs->rm(fpath.c_str());
        } else if ((size_t) offset != currentSize) {
            mg_http_reply(c, 409, "", "offset mismatch: expect %lu got %lld\n", (unsigned long) currentSize, offset);
            c->is_draining = 1;
            return;
        }
    } else {
        fs->rm(fpath.c_str());
    }

    void* fd = fs->op(fpath.c_str(), MG_FS_WRITE);
    if (fd == NULL) {
        mg_http_reply(c, 400, "", "open failed: %d\n", errno);
        c->is_draining = 1;
        return;
    }

    size_t written = fs->wr(fd, hm->body.buf, hm->body.len);
    fs->cl(fd);
    if (written != hm->body.len) {
        mg_http_reply(c, 500, "", "write failed\n");
        c->is_draining = 1;
        return;
    }

    LOGI("Uploaded %lu bytes to %s", (unsigned long) written, fpath.c_str());
    size_t nextOffset = hasOffset ? ((size_t) offset + written) : written;
    mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%lu", (unsigned long) nextOffset);
    c->is_draining = 1;

    bool completed = true;
    if (hasTotal) {
        completed = (long long) nextOffset >= totalSize;
    }

    if (completed && m_pEventHandler && on_event) {
        struct upload_file_info uploading;
        snprintf(uploading.oriFileName, MG_PATH_MAX, "%s", oriFileName.c_str());
        snprintf(uploading.fileName, MG_PATH_MAX, "%s", fpath.c_str());
        uploading.msgId = atoi(params["msgid"].c_str());
        uploading.idx = atoi(params["idx"].c_str());
        std::string apiRet = "ok";
        on_event(m_pEventHandler, "upload", &uploading, apiRet);
    }
}

void CMongooseWebServer::handle_download(struct mg_connection* c, int ev, void* ev_data) {
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    char fpath[MG_PATH_MAX];
    std::string fileName = decodeUrl(std::string(hm->query.buf, hm->query.len));
    
    if (fileName.find("{") != std::string::npos) {
        if (m_pEventHandler && on_event) {
            on_event(m_pEventHandler, "download", (void*)hm, fileName);
            if (fileName == "") {
                mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "File not found");
                return;
            }
        }
    }
    snprintf(fpath, MG_PATH_MAX, "%s%c%s", s_upld_dir.c_str(), MG_DIRSEP, fileName.c_str());
    if (!mg_path_is_sane(mg_str(fpath))) {
        mg_http_reply(c, 400, "", "Invalid path\n");
        c->is_draining = 1;  // Tell mongoose to close this connection
    }
    else {
        std::ifstream file(fpath, std::ios::binary | std::ios::ate);
        if (!file) {
            mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "File not found");
            return;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> fileContent(fileSize);

        if (!file.read(fileContent.data(), fileSize)) {
            mg_http_reply(c, 500, "Content-Type: text/plain\r\n", "Failed to read file");
            return;
        }

        file.close();

        mg_printf(c,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Disposition: attachment; filename=\"%s\"\r\n"
            "Content-Length: %d\r\n\r\n",
            std::string(hm->query.buf, hm->query.len).c_str(),
            static_cast<int>(fileContent.size()));

        mg_send(c, fileContent.data(), fileContent.size());
        if (m_pEventHandler && on_event) {
            std::string apiRet = "ok";
            //on_event(m_pEventHandler, "download", nullptr, apiRet);
        }
    }
}

void CMongooseWebServer::handle_api(struct mg_http_message* hm, std::string& apiRet) {
    if (m_pEventHandler && on_event) {
        on_event(m_pEventHandler, "api", (void*)hm, apiRet);
    }
}

void CMongooseWebServer::cb(struct mg_connection* c, int ev, void* ev_data) {
    CMongooseWebServer* server = static_cast<CMongooseWebServer*>(c->fn_data);

    if (ev == MG_EV_ACCEPT && server->isHttps()) {
        struct mg_tls_opts opts;
        memset(&opts, 0, sizeof(opts));
#ifdef TLS_TWOWAY
        opts.ca = mg_str(s_tls_ca);
#endif
        opts.cert = mg_str(s_tls_cert);
        opts.key = mg_str(s_tls_key);
        mg_tls_init(c, &opts);
    }

    if (ev == MG_EV_HTTP_MSG) {
        // Non-upload requests, we serve normally
        // NOTE: handle_uploads() may delete request and reset c->pfn
        /*
        if (c->pfn != NULL) {
            struct mg_http_serve_opts opts = { 0 };
            opts.root_dir = server->get_root_dir();
            mg_http_serve_dir(c, hm, &opts);
        }
        */

        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        std::string uri(hm->uri.buf, hm->uri.len);
        // Log request
        MG_INFO(("######### method: %s\n", get_mg_str(hm->method).c_str()));
        MG_INFO(("######### uri: %s\n", uri.c_str()));
        MG_INFO(("######### query: %s\n", get_mg_str(hm->query).c_str()));
        MG_INFO(("######### proto: %s\n", get_mg_str(hm->proto).c_str()));
        MG_INFO(("######### body len: %lu\n", (unsigned long) hm->body.len));
        //MG_INFO(("######### head: %s\n", get_mg_str(hm->head).c_str()));
        //MG_INFO(("######### message: %s\n", get_mg_str(hm->message).c_str()));
        if (mg_match(hm->uri, mg_str("/upload/#"), NULL)) {
            server->handle_uploads(c, ev, ev_data);
        }
        else if (mg_match(hm->uri, mg_str("/api"), NULL))
        {
            std::string apiRet = "ok";
            server->handle_api(hm, apiRet);
            mg_http_reply(c, 200, "Content-Type: text/plain\r\n", apiRet.c_str());
            c->is_draining = 1;          // Close connection when response gets sent
        }
        else if (mg_match(hm->uri, mg_str("/download"), NULL)) { //http://localhost:8090/download?2024.jpg
            server->handle_download(c, ev, ev_data);
        }
        else {
            if (c->pfn != NULL) {
                struct mg_http_serve_opts opts = { 0 };
                opts.root_dir = server->get_root_dir();
                mg_http_serve_dir(c, hm, &opts);
            }
        }

    }
}

void CMongooseWebServer::threadFunc() {
    struct mg_mgr mgr;
    mg_log_set(s_debug_level);
    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, s_listening_address.c_str(), CMongooseWebServer::cb, this) == NULL) {
        MG_ERROR(("Cannot listen on %s.", s_listening_address.c_str()));
        //exit(EXIT_FAILURE);
        return;
    }

    // Start infinite event loop
    MG_INFO(("Mongoose version : v%s", MG_VERSION));
    MG_INFO(("Listening on     : %s", s_listening_address.c_str()));
    MG_INFO(("Web root         : [%s]", s_root_dir.c_str()));
    MG_INFO(("Uploading to     : [%s]", s_upld_dir.c_str()));
    s_signo = 0;
    while (s_signo == 0) mg_mgr_poll(&mgr, 10);
    mg_mgr_free(&mgr);
    MG_INFO(("Exiting on signal %d", s_signo));
}

std::string CMongooseWebServer::startWebServer() {
    m_thread = std::thread(&CMongooseWebServer::threadFunc, this);
    return "";
}

std::string CMongooseWebServer::stopWebServer() {
    s_signo = 1;
    if (m_thread.joinable()) m_thread.join();
    return "";
}