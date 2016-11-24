#include <piga/devkit/WebUI.hpp>
#include <piga/devkit/HTTPServer.hpp>
#include <piga/devkit/ExportsManager.hpp>
#include <stdlib.h>
#include <fcntl.h>

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
HTTPServer::HTTPServer(Devkit *devkit, uint32_t port)
    : m_devkit(devkit), m_port(port), m_webUI(new WebUI(devkit))
{
    m_daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, m_port, NULL, NULL,
        &HTTPServer::answer_to_connection, this, MHD_OPTION_END);
    
    if (nullptr == m_daemon) 
        exit(1);
}
HTTPServer::~HTTPServer()
{
    MHD_stop_daemon(m_daemon);
}
std::string HTTPServer::parseRequest(const std::string &req, std::string *contentType) 
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
            case Devkit::RestartApp:
                restartApp(writer, params[0]);
                break;
            case Devkit::Web:
                // The right parameter is everything after the / of the token.
                params[0] = req.substr(req.find_first_of("/", req.find_first_of("/", 6)) + 1); 
                return web(params[0], contentType, token);
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
int HTTPServer::answer_to_connection(void* cls, struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* upload_data, std::size_t* upload_data_size, void ** con_cls)
{
    HTTPServer *instance = static_cast<HTTPServer*>(cls);
    
    struct MHD_Response *response = nullptr;
    
    int ret;
    
    std::string contentType = "application/json";
    std::string answer = instance->parseRequest(url, &contentType);
    
    
    if(answer.length() > 0) {
        if(answer[0] == '/') {
            // A file should be transmitted.
            int fd = open(answer.c_str(), O_RDONLY);
            off_t fsize;
            fsize = lseek(fd, 0, SEEK_END);
            response = MHD_create_response_from_fd(fsize, fd);
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
