#include "CMongooseWebClient.h"


#include <string>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <unordered_map>

std::string decodeUrl1(const std::string url) {
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

// Print HTTP response and signal that we're done
void CMongooseWebClient::fn(struct mg_connection* c, int ev, void* ev_data) {
    CMongooseWebClient* client = static_cast<CMongooseWebClient*>(c->fn_data);
    client->handle_fn(c, ev, ev_data);
}

void CMongooseWebClient::handle_fn(struct mg_connection* c, int ev, void* ev_data) {
    if (ev == MG_EV_OPEN) {
        // Connection created. Store connect expiration time in c->data
        *(uint64_t*)c->data = mg_millis() + s_timeout_ms;
    }
    else if (ev == MG_EV_POLL) {
        if (mg_millis() > *(uint64_t*)c->data &&
            (c->is_connecting || c->is_resolving)) {
            mg_error(c, "Connect timeout");
        }
    }
    else if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(s_url.c_str());
        // Send request
        MG_DEBUG(("Connected, send request"));
        mg_printf(c,
            "POST %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "Content-Type: octet-stream\r\n"
            "Content-Length: %d\r\n",
            mg_url_uri(s_url.c_str()), (int)host.len, host.buf, fsize);
        mg_http_bauth(c, s_user.c_str(), s_pass.c_str());  // Add Basic auth header
        mg_printf(c, "%s", "\r\n");        // End HTTP headers
    }
    else if (ev == MG_EV_WRITE && c->send.len < MG_IO_SIZE) {
        if (done == false) {
            uint8_t *buf = (uint8_t *) alloca(MG_IO_SIZE);
            size_t len = MG_IO_SIZE - c->send.len;
            len = fsize < len ? fsize : len;
            fd->fs->rd(fd->fd, buf, len);
            mg_send(c, buf, len);
            fsize -= len;
            free(buf);
            MG_DEBUG(("sent %u bytes", len));
        }
    }
    else if (ev == MG_EV_HTTP_MSG) {
        MG_DEBUG(("MSG"));
        // Response is received. Print it
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        printf("client recv: %d, %s\n", (int)hm->body.len, hm->body.buf);
        if (!strstr(hm->body.buf, " ok\n")) {
            uploadRet << hm->body.buf;
        }
        c->is_draining = 1;  // Tell mongoose to close this connection
        mg_fs_close(fd);
        done = true;  // Tell event loop to stop
    }
    else if (ev == MG_EV_ERROR) {
        MG_DEBUG(("ERROR"));
        if (done == false) {
            mg_fs_close(fd);
            uploadRet << "ERROR";
            done = true;  // Error, tell event loop to stop
        }
    }
}

std::string CMongooseWebClient::uploadFile(const char* s_fname, const char * url) {
    struct mg_mgr mgr;  // Event manager
    done = false;  // Event handler flips it to true
    time_t mtime;

    std::string fname = decodeUrl1(std::string(s_fname));
    const char * pF = fname.c_str();

    mg_fs_posix.st(pF, &fsize, &mtime);
    if (fsize == 0 ||
        (fd = mg_fs_open(&mg_fs_posix, pF, MG_FS_READ)) == NULL) {
        MG_ERROR(("open failed: %d", errno));
        //exit(EXIT_FAILURE);
        uploadRet << "open failed: " << errno;
        return uploadRet.str();
    }
    s_url = url;

    mg_log_set(s_debug_level);
    uploadRet.clear();
    mg_mgr_init(&mgr);                        // Initialise event manager
    mg_http_connect(&mgr, s_url.c_str(), fn, this);  // Create client connection
    while (!done) mg_mgr_poll(&mgr, 50);      // Event manager loops until 'done'
    mg_mgr_free(&mgr);
    printf("client upload done\n");
    return uploadRet.str();
}

void CMongooseWebClient::fn1(struct mg_connection* c, int ev, void* ev_data) {
    CMongooseWebClient* client = static_cast<CMongooseWebClient*>(c->fn_data);
    client->handle_fn1(c, ev, ev_data);
}

void CMongooseWebClient::handle_fn1(struct mg_connection* c, int ev, void* ev_data) {
    if (ev == MG_EV_CONNECT) {
        /*
        int status = *static_cast<int*>(ev_data);
        if (status != 0) {
            std::cerr << "Connection error: " << status << std::endl;
        }
        else {
            std::cout << "Connected to " << s_url << std::endl;
        }
        */
        /*
        mg_printf(c,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: MongooseDownloadClient/1.0\r\n",
            s_url.c_str(),
            mg_url_host(s_url.c_str()));
        mg_http_bauth(c, s_user.c_str(), s_pass.c_str());  // Add Basic auth header
        mg_printf(c, "%s", "\r\n");        // End HTTP headers
        */
        mg_printf(c, "GET /download?%s HTTP/1.0\r\nHost: localhost\r\n\r\n", download_file_name.c_str());
    }
    else if (ev == MG_EV_HTTP_MSG) {
        mg_http_message* msg = static_cast<mg_http_message*>(ev_data);

        if (mg_http_status(msg) == 200) {
            fileStream_.write(msg->body.buf, msg->body.len);
            std::cout << "File content received and written to " << s_url << std::endl;
        }
        else {
            std::cerr << "Failed to download file. HTTP status: " << mg_http_status(msg) << std::endl;
            downloadRet << "Failed to download file. HTTP status: " << mg_http_status(msg);
        }
        // 
        c->is_closing = 1;
        done = true;
    }
    else if (ev == MG_EV_CLOSE) {
        std::cout << "Connection closed" << std::endl;
    }
}

std::string CMongooseWebClient::downloadFile(const char* url, const char* filename, const char* s_fname) {
    struct mg_mgr mgr;  // Event manager
    done = false;
    s_url = url;
    download_file_name = filename;

    fileStream_.open(s_fname, std::ios::binary);

    mg_log_set(s_debug_level);
    downloadRet.clear();
    mg_mgr_init(&mgr);                        // Initialise event manager
    mg_http_connect(&mgr, s_url.c_str(), fn1, this);  // Create client connection
    while (!done) mg_mgr_poll(&mgr, 50);      // Event manager loops until 'done'
    mg_mgr_free(&mgr);
    fileStream_.close();
    printf("client download done\n");
    std::string ret = downloadRet.str();
    if (ret != "") remove(s_fname);
    return ret;
}
