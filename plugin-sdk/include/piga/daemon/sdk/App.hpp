#pragma once

namespace piga
{
namespace daemon
{
namespace sdk
{
class App
{
public:
    virtual void loadFromName(const std::string &name) = 0;
    virtual void loadFromPath(const std::string &path, bool autostart_active = true) = 0;
    virtual bool loadConfigFile(const std::string &configPath) = 0;
    virtual void executeAutostart() = 0;
    
    virtual void reload(bool start = false) = 0;
    virtual void start(bool restartIfRunning = false) = 0;
    virtual void stop() = 0; 

    virtual bool isRunning() const = 0;
    virtual bool isInstalled() const = 0;
    virtual void update() = 0;
    virtual pid_t getPid() const = 0;
    virtual bool shouldWaitForSignal() const = 0;
    virtual bool restartOnExit() const = 0;
    virtual bool restartOnCrash() const = 0;

    virtual const std::string& getPath() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getWorkingDir() const = 0;
    virtual const std::string& getExecutable() const = 0;
    virtual bool isAutostart() const = 0;
};
}
}
}
