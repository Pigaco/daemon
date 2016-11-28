#include <piga/daemon/PluginManager.hpp>

namespace piga
{
namespace daemon 
{
PluginManager::PluginManager(
        std::shared_ptr<sdk::DBusManager> dbusManager,
        std::shared_ptr<boost::asio::io_service> ioService,
        std::shared_ptr<sdk::AppManager> appManager
    )
    : m_dbusManager(dbusManager),
      m_ioService(ioService),
      m_appManager(appManager)
{
    
}
PluginManager::~PluginManager()
{
    
}

void PluginManager::setDBusManager(std::shared_ptr<sdk::DBusManager> dbusManager)
{
    m_dbusManager = dbusManager;
    for(auto &plugin : m_plugins) {
        plugin.second.plugin->setDBusManager(dbusManager);
    }
}
void PluginManager::setIOService(std::shared_ptr<boost::asio::io_service> ioService)
{
    m_ioService = ioService;
    for(auto &plugin : m_plugins) {
        plugin.second.plugin->setIOService(ioService);
    }
}
void PluginManager::setAppManager(std::shared_ptr<sdk::AppManager> appManager)
{
    m_appManager = appManager;
    for(auto &plugin : m_plugins) {
        plugin.second.plugin->setAppManager(appManager);
    }
}

bool PluginManager::startPlugin(const std::string &identifier) 
{
    if(m_plugins.count(identifier) > 0) {
        m_plugins[identifier].plugin->start();
        return true;
    } 
    return false;
}
bool PluginManager::stopPlugin(const std::string &identifier) 
{
    if(m_plugins.count(identifier) > 0) {
        m_plugins[identifier].plugin->stop();
        return true;
    } 
    return false;
}
bool PluginManager::removePlugin(const std::string &identifier)
{
    if(stopPlugin(identifier)) {
        m_plugins.erase(identifier);
        return true;
    }
    return false;
}

}
}
