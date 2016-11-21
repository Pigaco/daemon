#pragma once

#include <string>

#include <piga/daemon/sdk/DBusManager.hpp>

typedef struct sd_bus sd_bus;

namespace piga
{
namespace daemon 
{
/**
 * This class connects to the Systemd instance of the running system and manages
 * all low level tasks (restarting the daemon itself, NFS shares for the devkit, ...).
 * 
 * All this is done using the sd-bus library from systemd, to keep dependencies
 * minimal. Here is a blog about that: http://0pointer.de/blog/the-new-sd-bus-api-of-systemd.html
 */
class DBusManager : public sdk::DBusManager
{
public:
    DBusManager();
    ~DBusManager();
    
    void init();
    void deinit();
    
    // The following functions will be called on the according DBus objects. 
    virtual bool Reboot() override;
    virtual bool ReloadUnit(const std::string &serviceName) override;
private:
    sd_bus *m_bus = nullptr;
};
}
}
