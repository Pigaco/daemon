#ifndef PIGA_DAEMON_APP_HPP_INCLUDED
#define PIGA_DAEMON_APP_HPP_INCLUDED

#include <vector>
#include <string>
#include <memory>

namespace piga
{
namespace daemon
{
class App
{
public:
    App(const std::string &defaultAppPath = "/usr/lib/piga/apps/", uid_t defaultUID = 1010);
    ~App();

    void loadFromName(const std::string &name);
    void loadFromPath(const std::string &path, bool autostart_active = true);
    bool loadConfigFile(const std::string &configPath);
    static void generateSampleConfig(const std::string &output);
    void reload(bool start = false);
    void executeAutostart();

    void start(bool restartIfRunning = false);
    void stop();

    bool isRunning();
    bool isInstalled();
    pid_t getPid();
    void update();

    const std::string& getPath();
    const std::string& getName();
    const std::string& getWorkingDir();
    const std::string& getExecutable();
    bool isAutostart();
private:
    std::string m_appPath;
    std::string m_name = "Undefined App Name";
    std::string m_path;

    std::string m_workingDir;
    std::string m_executable;
    std::vector<std::string> m_args;

    bool m_stopped = false;
    bool m_running = false;
    bool m_installed = false;
    bool m_autostart = false;

    pid_t m_pid = 0;
    uid_t m_uid = 1010;
};
}
}

#endif
