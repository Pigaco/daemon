#pragma once

#include <microhttpd.h>
#include <cstring>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <piga/devkit/Devkit.hpp>

#include <map>
#include <vector>

namespace piga 
{
namespace devkit 
{
class Devkit;

class HTTPServer 
{
public: 
    typedef rapidjson::Writer<rapidjson::StringBuffer> JsonWriter;
        
    HTTPServer(Devkit *devkit, uint32_t port = 8080);
    ~HTTPServer();
    
    std::string parseRequest(const std::string &req);
    
    void setAllowedActionsForToken(const std::string &token, const Devkit::ActionsVector &actions);
    
    void exportNFS(JsonWriter &writer, const std::string &address);
    void removeNFSExport(JsonWriter &writer, const std::string &address);
    void reboot(JsonWriter &writer);
    void restartApp(JsonWriter &writer, const std::string &appName);
    
    static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                                     const char *url,
                                     const char *method, const char *version,
                                     const char *upload_data,
                                     std::size_t *upload_data_size, void **con_cls);
private:
    struct MHD_Daemon *m_daemon = nullptr;
    uint32_t m_port = 8080;
    
    Devkit *m_devkit;
    
    std::map<std::string, Devkit::ActionsVector> m_tokens;
};
}
}
