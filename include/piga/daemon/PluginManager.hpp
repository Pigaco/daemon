#pragma once

#include <piga/daemon/sdk/Plugin.hpp>

#include <boost/log/trivial.hpp>

#include <memory>
#include <string>
#include <unordered_map>

#include <piga/daemon/LogManager.hpp>

namespace piga
{
namespace daemon
{
class PluginManager
{
public:
    typedef std::shared_ptr<sdk::Plugin> PluginPtr;
    struct Plugin {
        PluginPtr plugin;
        SeverityChannelLogger logger;
    };
    typedef std::unordered_map<std::string, Plugin> PluginMap;
    
    PluginManager(
        std::shared_ptr<sdk::DBusManager> dbusManager,
        std::shared_ptr<boost::asio::io_service> ioService,
        std::shared_ptr<sdk::AppManager> appManager
    );
    ~PluginManager();
    
    template<typename T>
    std::shared_ptr<T> activatePlugin(const std::string &identifier) {
        std::shared_ptr<sdk::Plugin> plugin;
        
        if(m_plugins.count(identifier) == 0) {
            plugin = std::static_pointer_cast<sdk::Plugin>(
                std::make_shared<T>());
            
            m_plugins[identifier].plugin = plugin;
        } else {
            plugin = m_plugins[identifier].plugin;
        }
        
        Plugin *entry = &m_plugins[identifier];
        entry->logger = SeverityChannelLogger(boost::log::keywords::channel = ("Plugin:" + identifier));
        
        plugin->setDBusManager(m_dbusManager);
        plugin->setAppManager(m_appManager);
        plugin->setIOService(m_ioService);
        plugin->setLoggingCallback([this,identifier,entry](const std::string &msg) {
            BOOST_LOG_SEV(entry->logger, L_INFO) << msg;
        });
        
        return std::static_pointer_cast<T>(plugin);
    }
    
    void setDBusManager(std::shared_ptr<sdk::DBusManager> dbusManager);
    void setAppManager(std::shared_ptr<sdk::AppManager> appManager);
    void setIOService(std::shared_ptr<boost::asio::io_service> ioService);
    
    bool startPlugin(const std::string &identifier);
    bool stopPlugin(const std::string &identifier);
    bool removePlugin(const std::string &identifier);
private:
    PluginMap m_plugins;
    
    std::shared_ptr<sdk::DBusManager> m_dbusManager;
    std::shared_ptr<sdk::AppManager> m_appManager;
    std::shared_ptr<boost::asio::io_service> m_ioService;
};
}
}
