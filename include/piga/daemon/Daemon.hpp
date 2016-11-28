#ifndef PIGA_DAEMON_DAEMON_DAEMON_HPP_INCLUDED
#define PIGA_DAEMON_DAEMON_DAEMON_HPP_INCLUDED

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <memory>
#include <string>
#include <piga/host.h>
#include <piga/client.h>

#include <piga/event_app_installed.h>

#include <piga/daemon/Loader.hpp>
#include <piga/daemon/AppManager.hpp>
#include <piga/daemon/LogManager.hpp>

#define PIGA_DAEMON_PIDFILE_PATH "/etc/piga/proc/daemon.pid"

namespace piga
{
namespace devkit 
{
class Devkit;
}
    
namespace daemon
{
class DBusManager;
class PluginManager;
    
class Daemon
{
public:
    Daemon(char** envp);
    ~Daemon();

    void run();
    void stop();
    void reload();

    void signalHandler(const boost::system::error_code &code, int signal_number);
    void update();

    static const char* getPidfilePath() {
        return getenv("PIGA_DAEMON_PIDFILE_PATH");
    }

private:
    std::unique_ptr<Loader> m_loader;
    std::shared_ptr<AppManager> m_appManager;
    std::shared_ptr<::piga::devkit::Devkit> m_devkit;
    std::shared_ptr<DBusManager> m_dbusManager;
    std::shared_ptr<PluginManager> m_pluginManager;

    std::shared_ptr<boost::asio::io_service> m_io_service;
    std::shared_ptr<boost::asio::io_service::work> m_work;
    std::shared_ptr<boost::asio::deadline_timer> m_hostTimer;
    boost::asio::signal_set m_signals;

    std::string m_configFilePath = "/etc/piga/daemon.cfg";
    std::string m_soPath = "/usr/lib/piga/hosts/";
    std::string m_name = "Unnamed Piga Host";
    uid_t m_defaultUID = 1010;
    std::string m_defaultAppPath = "/usr/lib/piga/apps/";
    bool m_devkitActive = false;
    uint32_t m_devkitHttpPort = 8080;

    std::shared_ptr<piga_host> m_host;
    std::shared_ptr<piga_client> m_client;
    std::shared_ptr<piga_event> m_cacheEvent;
    char m_cacheBuffer[PIGA_EVENT_APP_INSTALLED_NAME_LENGTH];
    char **m_envp;
    
    SeverityChannelLogger m_log;
};
}
}

#endif
