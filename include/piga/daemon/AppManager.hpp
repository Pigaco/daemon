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
    AppManager(const std::string &directory = "/usr/lib/piga/apps/", uid_t defaultUID = 1010);
    ~AppManager();

    void reload(const std::string &directory = "/usr/lib/piga/apps/");
    void update();
private:
    std::unordered_map<std::string, std::shared_ptr<App>> m_apps;
    std::string m_directory;
    uid_t m_defaultUID;
};
}
}

#endif
