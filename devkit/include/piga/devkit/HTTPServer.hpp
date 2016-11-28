#pragma once

#include <boost/signals2/signal.hpp>
#include <boost/asio/detail/posix_fd_set_adapter.hpp>

#include <microhttpd.h>
#include <cstring>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <piga/devkit/Devkit.hpp>

#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace piga 
{
namespace devkit 
{
class Devkit;
class WebUI;

class HTTPServer 
{
public: 
    typedef rapidjson::Writer<rapidjson::StringBuffer> JsonWriter;
        
    HTTPServer(Devkit *devkit, uint32_t port = 8080);
    ~HTTPServer();
    
    std::string parseRequest(const std::string &req, std::string *contentType, Devkit::DevkitAction *return_action);
    
    void setAllowedActionsForToken(const std::string &token, const Devkit::ActionsVector &actions);
    
    void exportNFS(JsonWriter &writer, const std::string &address);
    void removeNFSExport(JsonWriter &writer, const std::string &address);
    void reboot(JsonWriter &writer);
    void restartApp(JsonWriter &writer, const std::string &appName);
    
    void connectionEnded(struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode code);
    
    std::string web(const std::string &path, std::string *contentType, const std::string &token);
    
    int64_t chunkedResponseCallback(void *cls, uint64_t pos, char *buf, size_t max);
    
    static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                                     const char *url,
                                     const char *method, const char *version,
                                     const char *upload_data,
                                     std::size_t *upload_data_size, void **con_cls);
    
    // Pushes a message on the internal log buffer, which can then be read. 
    void logReaderCallback(const std::string &msg);
private:
    struct MHD_Daemon *m_daemon = nullptr;
    uint32_t m_port = 8080;
    
    std::unique_ptr<boost::asio::detail::posix_fd_set_adapter> m_readFdSet;
    std::unique_ptr<boost::asio::detail::posix_fd_set_adapter> m_writeFdSet;
    
    Devkit *m_devkit;
    
    std::map<std::string, Devkit::ActionsVector> m_tokens;
    
    std::unique_ptr<WebUI> m_webUI;
    
    static void connectionEndedCb(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode code);
    
    void setConnectionAction(struct MHD_Connection *conn, Devkit::DevkitAction action);
    Devkit::DevkitAction getConnectionAction(struct MHD_Connection *conn);
    void eraseConnectionFromActions(struct MHD_Connection *conn);
    std::vector<struct MHD_Connection*> getConnectionsOfAction(Devkit::DevkitAction action);
    
    std::mutex m_actionMapMutex;
    std::unordered_map<struct MHD_Connection*, Devkit::DevkitAction> m_actionMap;
    
    void removeLogReader();
    void addLogReader();
    std::string getLogMsgForConnection(struct MHD_Connection *conn);
    std::mutex m_logAccessMutex;
    std::size_t m_logAccessorCount = 0;
    std::vector<std::string> m_logBuffer;
    std::unordered_map<struct MHD_Connection*, std::size_t> m_logReaderPos;
    boost::signals2::connection m_logReaderConnection;
};
}
}
