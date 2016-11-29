#include <piga/devkit/WebUI.hpp>
#include <piga/devkit/HTTPServer.hpp>
#include <piga/devkit/ExportsManager.hpp>
#include <stdlib.h>
#include <fcntl.h>

#include <piga/daemon/sdk/LogManager.hpp>

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>

#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <boost/filesystem.hpp>

#include <sstream>
#include <algorithm>

namespace xpr = boost::xpressive;

namespace piga
{
namespace devkit 
{
struct ChunkedResponseData {
    HTTPServer *object;
    struct MHD_Connection *conn;
    std::string url;
    Devkit::DevkitAction action;
};

HTTPServer::HTTPServer(Devkit *devkit, uint32_t port)
    : m_devkit(devkit), m_port(port), m_webUI(new WebUI(devkit))
{
    m_daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, m_port, NULL, NULL,
        &HTTPServer::answer_to_connection, this, 
        
        MHD_OPTION_NOTIFY_COMPLETED, &HTTPServer::connectionEndedCb, this,
        // MHD_OPTION_THREAD_POOL_SIZE, 2, // The connection cannot be suspended, because this conflicts with boost.
        MHD_OPTION_END);
    
    if (nullptr == m_daemon) {
        m_devkit->log("The HTTP daemon could not be started!");
        exit(1);
    } 
    
    /* 
    else {
        // The daemon was created successfully. Now, sockets have to be assigned to 
        // the io_service of the main application.
        int r = 0;
        
        fd_set readFdSet;
        fd_set writeFdSet;
        fd_set exceptFdSet;
        int maxFd = 0;
        
        r = MHD_get_fdset(m_daemon, &readFdSet, &writeFdSet, &exceptFdSet, &maxFd);
        if(r != MHD_YES) {
            m_devkit->log("fd_sets could not be received from MHD!");
        } else {
            m_devkit->log("fd_sets received!");
            
            m_writeFdSet.reset(new boost::asio::detail::posix_fd_set_adapter());
            m_readFdSet.reset(new boost::asio::detail::posix_fd_set_adapter());
            
            *(*m_readFdSet) = readFdSet;
            *(*m_writeFdSet) = writeFdSet;
            
            boost::asio::async_read(m_readFdSet, )
        }
    }
    */
}
HTTPServer::~HTTPServer()
{
    MHD_stop_daemon(m_daemon);
}
std::string HTTPServer::parseRequest(const std::string &req, std::string *contentType, Devkit::DevkitAction *return_action) 
{
    using namespace boost::xpressive;
  
    std::string token;
    std::string params[3];
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    
    writer.StartObject();
    
    Devkit::DevkitAction action = Devkit::Unknown;
    
    sregex expr = 
            "/devkit/" >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::Export] >> "/export/" >> *(*(+_w) | *(set='.',':'))
        |
            "/devkit/" >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::RemoveExport] >> "/removeExport/" >> (+_w)[xpr::ref(params)[0] = _]
        |
            "/devkit/" >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::Reboot] >> "/reboot"
        |
            "/devkit/" >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::RestartApp] >> "/restartApp/" >> (+_w)[xpr::ref(params)[0] = _]
        |  
            "/devkit/" >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::GetLogBuffer] >> "/log/"
        |  
            "/web/"    >> (+_w)[xpr::ref(token) = _, xpr::ref(action) = Devkit::Web] >> "/" >> *(*(+_w) | *(set='.',':','/','-'))
        ;
    regex_match(req, expr);
    
    bool tokenValid = false;
    
    if(m_tokens.count(token) == 1) {
        std::vector<Devkit::DevkitAction> allowedActions = m_tokens[token];
        if(std::find(allowedActions.begin(), allowedActions.end(), action) != allowedActions.end()) {
            tokenValid = true;
        }
    }
    
    if(tokenValid) 
    {
        *return_action = action;
        switch(action) 
        {
            case Devkit::Unknown:
                writer.Key("status");
                writer.Bool(false);
                writer.Key("error");
                writer.String("Unknown action or invalid URL pattern.");
                break;
            case Devkit::Export:
                params[0] = req.substr(req.find_last_of("/") + 1);
                exportNFS(writer, params[0]);
                break;
            case Devkit::RemoveExport:
                removeNFSExport(writer, params[0]);
                break;
            case Devkit::Reboot:
                reboot(writer);
                break;
            case Devkit::GetLogBuffer:
                *contentType = "text";
                return "\n";
                break;
            case Devkit::RestartApp:
                restartApp(writer, params[0]);
                break;
            case Devkit::Web:
                // The right parameter is everything after the / of the token.
                params[0] = req.substr(req.find_first_of("/", req.find_first_of("/", 6)) + 1); 
                return web(params[0], contentType, token);
                break;
        }
    }
    else {
        writer.Key("status");
        writer.Bool(false);
        writer.Key("error");
        writer.String("Action not allowed with the used token.");
    }
    
    writer.EndObject();
    
    return buffer.GetString();
}
void HTTPServer::setAllowedActionsForToken(const std::string &token, const Devkit::ActionsVector &actions)
{
    m_tokens[token] = actions;
}
void HTTPServer::exportNFS(JsonWriter &writer, const std::string &address)
{
    m_devkit->m_exportsManager->setExport("/", address);
    m_devkit->m_exportsManager->updateExportsFile();
    
}
void HTTPServer::removeNFSExport(JsonWriter &writer, const std::string &address)
{
    m_devkit->m_exportsManager->removeExport("/", address);
    m_devkit->m_exportsManager->updateExportsFile();
}
void HTTPServer::reboot(JsonWriter &writer)
{
    m_devkit->m_dbusManager->Reboot();
}
void HTTPServer::connectionEndedCb(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode code)
{
    HTTPServer *server = static_cast<HTTPServer*>(cls);
    server->connectionEnded(connection, con_cls, code);
}
void HTTPServer::connectionEnded(struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode code)
{
    Devkit::DevkitAction action = getConnectionAction(connection);
    if(action == Devkit::GetLogBuffer) {
        removeLogReader();
        {
            std::lock_guard<std::mutex> logLock(m_logAccessMutex);
            if(m_logReaderPos.count(connection) > 0) {
                m_logReaderPos.erase(connection);
            }
        }
    }
    eraseConnectionFromActions(connection);
}
void HTTPServer::addLogReader()
{
    std::lock_guard<std::mutex> logLock(m_logAccessMutex);
    ++m_logAccessorCount;
    if(m_logAccessorCount == 1) {
        m_logReaderConnection = 
            ::piga::daemon::sdk::LogManager::get()->getOutBuffer().bufferFlush.connect(
                boost::bind(&HTTPServer::logReaderCallback, this, _1)
            );
    }
}
void HTTPServer::removeLogReader()
{
    std::lock_guard<std::mutex> logLock(m_logAccessMutex);
    --m_logAccessorCount;
    
    if(m_logAccessorCount == 0) {
        m_logReaderConnection.disconnect();
    }
}
void HTTPServer::logReaderCallback(const std::string &msg) 
{
    {
        std::lock_guard<std::mutex> logLock(m_logAccessMutex);
        m_logBuffer.push_back(msg);
    }
    
    // We have to resume all connections with GetLogBuffer actions.
    std::vector<struct MHD_Connection*> conns = getConnectionsOfAction(Devkit::GetLogBuffer);
    for(auto conn : conns) {
        // MHD_resume_connection(conn); //TODO implement boost integration to make libmicrohttpd and boost asio compatible.
    }
}
std::string HTTPServer::getLogMsgForConnection(struct MHD_Connection *conn)
{
    std::string msg;
    std::lock_guard<std::mutex> logLock(m_logAccessMutex);
    if(m_logReaderPos.count(conn) == 0) {
        m_logReaderPos[conn] = 0;
    } 
    std::size_t &logPos = m_logReaderPos[conn];
    if(m_logBuffer.size() > 0) {
        msg = m_logBuffer[logPos];
        ++logPos;
    } else {
        // There are no logs currently! Suspend the connection
        // The connection will be resumed when there are new log messages.
        // MHD_suspend_connection(conn); // Suspend cannot be used with one thread per connection
        // TODO implement boost integration to do select() calling.
        return "";
    }
    // Check if we can shrink the buffer. This occurs, when all connections read the
    // next message.
    bool shrinkByOne = true;
    for(auto it : m_logReaderPos) {
        if(it.first != conn && it.second < 1) {
            shrinkByOne = false;
        }
    }
    if(shrinkByOne && logPos > 0) {
        for(auto it : m_logReaderPos) {
            m_logReaderPos[conn] = it.second - 1;
        }
        m_logBuffer.erase(m_logBuffer.begin());
    }
    
    return msg;
}
std::string HTTPServer::web(const std::string &path, std::string *contentType, const std::string &token) {
    // Check if this is a file.
    boost::filesystem::path fullPath1(boost::filesystem::current_path() / boost::filesystem::path("../devkit/web/" + path));
    boost::filesystem::path fullPath2("/usr/share/piga/devkit/web/" + path);
    if(boost::filesystem::exists(fullPath1)) {
        return fullPath1.string();
    } else if(boost::filesystem::exists(fullPath2)) {
        return fullPath2.string();
    }
    
    // The file doesn't exist, go to the webUI handler.
    return m_webUI->handleRequest(path, contentType, token);
}
void HTTPServer::restartApp(JsonWriter &writer, const std::string &app)
{
    std::shared_ptr<::piga::daemon::sdk::App> appPtr = 
        (*m_devkit->m_appManager)[app];
        
    if(appPtr) {
        if(appPtr->isRunning()) {
            appPtr->stop();
        }
        appPtr->reload();
        appPtr->start();
        
        writer.Key("status");
        writer.Bool(true);
    } 
    else {
        writer.Key("status");
        writer.Bool(false);
        writer.Key("error");
        writer.String(("The app \"" + app + "\" was not found in the app manager.").c_str());
    }
}

#if MHD_VERSION < 0x00095102
int // These defines are needed because of version discrepancies in MHD between debian and arch.
#else
int64_t
#endif
HTTPServer::chunkedResponseCallback(void *cls, uint64_t pos, char *buf, size_t max) 
{
    ChunkedResponseData *data = static_cast<ChunkedResponseData*>(cls);
    
    if(data->action == Devkit::GetLogBuffer) {
        std::string msg = getLogMsgForConnection(data->conn);
        if(msg.length() < max) {
            std::memcpy(buf, msg.data(), msg.length());
            return msg.length();
        } else {
            std::memcpy(buf, msg.data(), max);
            return max;
        }
    }
    else {
        // All other actions are not handled as chunked callbacks.
        return -1;
    }
}
void HTTPServer::setConnectionAction(struct MHD_Connection *conn, Devkit::DevkitAction action)
{
    std::lock_guard<std::mutex> connectionActionMutexLock(m_actionMapMutex);
    m_actionMap[conn] = action;
}
Devkit::DevkitAction HTTPServer::getConnectionAction(struct MHD_Connection *conn)
{
    std::lock_guard<std::mutex> connectionActionMutexLock(m_actionMapMutex);
    if(m_actionMap.count(conn) > 0) {
        return m_actionMap[conn];
    }
    return Devkit::Unknown;
}
void HTTPServer::eraseConnectionFromActions(struct MHD_Connection *conn)
{
    std::lock_guard<std::mutex> connectionActionMutexLock(m_actionMapMutex);
    if(m_actionMap.count(conn) > 0) {
        m_actionMap.erase(conn);
    }
}
std::vector<struct MHD_Connection*> HTTPServer::getConnectionsOfAction(Devkit::DevkitAction action)
{
    std::lock_guard<std::mutex> connectionActionMutexLock(m_actionMapMutex);
    
    std::vector<struct MHD_Connection*> connections;
    
    for(auto it : m_actionMap) {
        if(it.second == action) {
            connections.push_back(it.first);
        }
    }
    
    return connections;
}
int HTTPServer::answer_to_connection(void* cls, struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* upload_data, std::size_t* upload_data_size, void ** con_cls)
{
    HTTPServer *instance = static_cast<HTTPServer*>(cls);
    
    struct MHD_Response *response = nullptr;
    
    int ret;
    Devkit::DevkitAction action = Devkit::Unknown;
    
    std::string contentType = "application/json";
    std::string answer = instance->parseRequest(url, &contentType, &action);
    
    // Set the connection action for cleanup purposes.
    instance->setConnectionAction(connection, action);
    
    if(answer.length() > 0) {
        if(answer[0] == '/') {
            // A file should be transmitted.
            int fd = open(answer.c_str(), O_RDONLY);
            off_t fsize;
            fsize = lseek(fd, 0, SEEK_END);
            response = MHD_create_response_from_fd(fsize, fd);
        }
        else if(answer[0] == '\n') {
            // This is a chunked response and should use a callback.
            
            if(action == Devkit::GetLogBuffer) {
                instance->addLogReader();
            }
            
            ChunkedResponseData *data = new ChunkedResponseData();
            data->object = instance;
            data->url = url;
            data->action = action;
            data->conn = connection;
            
            response = MHD_create_response_from_callback(-1, 
                4000,
#if MHD_VERSION < 0x00095102
                [](void *cls, uint64_t pos, char *buf, uint32_t max) {
#else 
                [](void *cls, uint64_t pos, char *buf, size_t max) {
#endif
                    ChunkedResponseData *data = static_cast<ChunkedResponseData*>(cls);
                    return data->object->chunkedResponseCallback(cls, pos, buf, max);
                },
                data,
                [](void *cls) {
                    ChunkedResponseData *data = static_cast<ChunkedResponseData*>(cls);
                    delete data;
                }
            );
            //MHD_add_connection(respone, "Transfer-Encoding", "chunked");
            
        }
        else {
            // An answer should be transmitted.
            response = MHD_create_response_from_buffer(
                answer.length(),
                reinterpret_cast<void*>(const_cast<char*>(answer.data())), // This cast is safe, because the data is copied.
                MHD_RESPMEM_MUST_COPY
            );
            MHD_add_response_header(response, "Content-Type", contentType.c_str());
        }
    }
    
    if(response == nullptr) {
        response = MHD_create_response_from_buffer(
            sizeof("501 ERROR"),
            reinterpret_cast<void*>(const_cast<char*>("501 ERROR")), 
            MHD_RESPMEM_PERSISTENT
        );
    }
    
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    
    return ret;
}
}
}
