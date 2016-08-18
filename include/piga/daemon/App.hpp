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
    App(const std::string &defaultAppPath = "/usr/lib/piga/apps/", uid_t defaultUID = 1010, char **envp = nullptr);
    ~App();

    void loadFromName(const std::string &name);
    void loadFromPath(const std::string &path, bool autostart_active = true);
    bool loadConfigFile(const std::string &configPath);
    static void generateSampleConfig(const std::string &output);
    void reload(bool start = false);
    void executeAutostart();

    void start(bool restartIfRunning = false);
    void stop();

    bool isRunning() const;
    bool isInstalled() const;
    pid_t getPid() const;
    void update();
    bool shouldWaitForSignal() const;
    bool restartOnExit() const;
    bool restartOnCrash() const;

    const std::string& getPath() const;
    const std::string& getName() const;
    const std::string& getWorkingDir() const;
    const std::string& getExecutable() const;
    bool isAutostart() const;
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
    bool m_runAsRoot = false;
    bool m_waitForSignal = false;
    bool m_restartOnCrash = false;
    bool m_restartOnExit = false;

    pid_t m_pid = 0;
    uid_t m_uid = 1010;
    char **m_envp;
    int m_waitpid_counter = 0;
};
}
}

#endif
