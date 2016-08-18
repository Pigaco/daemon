#ifndef PIGA_DAEMON_APPMANAGER_HPP_INCLUDED
#define PIGA_DAEMON_APPMANAGER_HPP_INCLUDED

#include <unordered_map>
#include <memory>
#include <string>
#include <piga/daemon/App.hpp>

namespace piga
{
namespace daemon
{
class AppManager
{
public:
    typedef std::unordered_map<std::string, std::shared_ptr<App>> AppMap;
    AppManager(const std::string &directory = "/usr/lib/piga/apps/", uid_t defaultUID = 1010, char **envp = nullptr);
    ~AppManager();

    void reload(const std::string &directory = "/usr/lib/piga/apps/");
    void update();
    void processApps();
private:
    AppMap m_apps;
    AppMap::iterator m_currentPos;
    bool m_continueAfterWait = false;
    std::string m_directory;
    uid_t m_defaultUID;
    char **m_envp;
};
}
}

#endif
