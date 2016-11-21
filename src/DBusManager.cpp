#include <piga/daemon/DBusManager.hpp>
#include <boost/log/trivial.hpp>
#include <systemd/sd-bus.h>

namespace piga 
{
namespace daemon 
{
DBusManager::DBusManager()
{
}
DBusManager::~DBusManager()
{
    deinit();
}
void DBusManager::init()
{
    deinit();
    
    int r = 0;
    
    r = sd_bus_open_system(&m_bus);
    if(r < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to connect to system bus! Error: " << strerror(-r);
        return;
    }
    else {
        BOOST_LOG_TRIVIAL(info) << "Sucessfully opened system bus.";
    }
}
void DBusManager::deinit()
{
    if(m_bus) {
        sd_bus_unref(m_bus);
    }
}
bool DBusManager::Reboot()
{
    int r = sd_bus_call_method(m_bus,
                           "org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           "Reboot",
                           nullptr,
                           nullptr,
                           "b",
                           "0");
    
    if(r < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to issue \"Reboot\"! Error: " << strerror(-r);
        return false;
    }
    
    return true;
}
bool DBusManager::ReloadUnit(const std::string& serviceName)
{
    int r = sd_bus_call_method(m_bus,
                           "org.freedesktop.systemd1",
                           "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager",
                           "ReloadOrRestartUnit",
                           nullptr,
                           nullptr,
                           "ss",
                           serviceName.c_str(),
                           "replace"
    );
    
    if(r < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to issue \"ReloadUnit\"! Error: " << strerror(-r);
        return false;
    }
    
    return true;
}
}
}
