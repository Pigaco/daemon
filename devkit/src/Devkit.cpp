#include <piga/devkit/HTTPServer.hpp>
#include <piga/devkit/ExportsManager.hpp>
#include <piga/devkit/Devkit.hpp>
#include <exception>

namespace as = boost::asio;

namespace piga 
{
namespace devkit 
{
Devkit::DevkitAction Devkit::getActionFromStr(const char *str)
{
    if(strcmp(str, "Unknown") == 0)
        return Unknown;
    else if(strcmp(str, "Export") == 0)
        return Export;
    else if(strcmp(str, "RemoveExport") == 0)
        return RemoveExport;
    else if(strcmp(str, "Reboot") == 0)
        return Reboot;
    else if(strcmp(str, "RestartApp") == 0)
        return RestartApp;
    else if(strcmp(str, "GetLogBuffer") == 0)
        return GetLogBuffer;
    else if(strcmp(str, "Web") == 0)
        return Web;
    return Unknown;
}
    
Devkit::Devkit()
{
}
Devkit::~Devkit() 
{
}

void Devkit::start()
{
    log("Starting devkit.");
    
    m_exportsManager.reset(new ExportsManager(this));
    m_exportsManager->readFromConfigFile();
    
    m_httpServer.reset(new HTTPServer(this, m_httpPort));
}
void Devkit::stop()
{
    log("Stopping devkit.");
    
    m_httpServer.reset();
}

void Devkit::setHTTPPort(uint32_t port)
{
    m_httpPort = port;
}
void Devkit::setAllowedActionsForToken(const std::string &token, const ActionsVector &actions)
{
    if(m_httpServer) {
        m_httpServer->setAllowedActionsForToken(token, actions);
    }
}
}
}
