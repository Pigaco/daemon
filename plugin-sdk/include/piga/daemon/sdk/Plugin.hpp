#pragma once

#include <memory>

#include <boost/asio/io_service.hpp>

#include <piga/daemon/sdk/DBusManager.hpp>
#include <piga/daemon/sdk/AppManager.hpp>

namespace piga
{
namespace daemon
{
namespace sdk
{
class Plugin
{
public:
    typedef std::function<void(const std::string&)> LogCb;
    
    void setDBusManager(std::shared_ptr<DBusManager> mgr) {
        m_dbusManager = mgr;
    }
    void setAppManager(std::shared_ptr<AppManager> mgr) {
        m_appManager = mgr;
    }
    void setIOService(std::shared_ptr<boost::asio::io_service> ioService) {
        m_ioService = ioService;
    }
    void setLoggingCallback(LogCb logger) {
        m_loggerCb = logger;
    }
    void log(const std::string& msg)
    {
        if(m_loggerCb) {
            m_loggerCb(msg);
        } else {
            throw std::runtime_error("No logging callback defined in plugin!");
        }
    }
    
    virtual void start() = 0;
    virtual void stop() = 0;
protected:
    std::shared_ptr<DBusManager> m_dbusManager;
    std::shared_ptr<AppManager> m_appManager;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    
    LogCb m_loggerCb;
};
}
}
}
