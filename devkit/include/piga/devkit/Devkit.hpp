#pragma once

#include <functional>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include <piga/daemon/sdk/Plugin.hpp>

namespace piga
{
namespace devkit 
{
class HTTPServer;
class ExportsManager;

/**
 * The devkit class provides the main entry point for this library and has to be instantiated by the daemon.
 */
class Devkit : public ::piga::daemon::sdk::Plugin
{
    friend class HTTPServer;
public: 
    enum DevkitAction {
        Unknown,
        Export,
        RemoveExport,
        Reboot,
        RestartApp,
    };
    
    static DevkitAction getActionFromStr(const char *str);
    
    typedef std::vector<DevkitAction> ActionsVector;
    
    Devkit();
    ~Devkit();
    
    void setHTTPPort(uint32_t port);
    
    void setAllowedActionsForToken(const std::string &token, const ActionsVector &actions);
    
    virtual void start() override;
    virtual void stop() override;
private:
    std::unique_ptr<HTTPServer> m_httpServer;
    std::unique_ptr<ExportsManager> m_exportsManager;
    
    uint32_t m_httpPort = 8080;
    
    FILE *m_inotifyHandle = nullptr;
};
}
}
