#ifndef PIGA_DAEMON_APP_HPP_INCLUDED
#define PIGA_DAEMON_APP_HPP_INCLUDED

#include <vector>
#include <string>
#include <memory>

#include <piga/daemon/sdk/App.hpp>
#include <piga/daemon/LogManager.hpp>

namespace piga
{
namespace daemon
{
class App : public sdk::App
{
public:
    App(const std::string &defaultAppPath = "/usr/lib/piga/apps/", uid_t defaultUID = 1010, char **envp = nullptr);
    ~App();

    virtual void loadFromName(const std::string &name) override;
    virtual void loadFromPath(const std::string &path, bool autostart_active = true) override;
    virtual bool loadConfigFile(const std::string &configPath) override;
    static void generateSampleConfig(const std::string &output);
    virtual void reload(bool start = false) override;
    virtual void executeAutostart() override;

    virtual void start(bool restartIfRunning = false) override;
    virtual void stop() override;

    virtual bool isRunning() const override;
    virtual bool isInstalled() const override;
    virtual void update() override;
    virtual pid_t getPid() const override;
    virtual bool shouldWaitForSignal() const override;
    virtual bool restartOnExit() const override;
    virtual bool restartOnCrash() const override;

    virtual const std::string& getPath() const override;
    virtual const std::string& getName() const override;
    virtual const std::string& getWorkingDir() const override;
    virtual const std::string& getExecutable() const override;
    virtual bool isAutostart() const override;
private:
    std::string m_appPath;
    std::string m_name = "Undefined App Name";
    std::string m_path;

    std::string m_workingDir;
    std::string m_executable;
    std::vector<std::string> m_args;
    std::vector<std::string> m_envvars;

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
    
    SeverityChannelLogger m_log;
    SeverityChannelLogger m_appLog;
};
}
}

#endif
