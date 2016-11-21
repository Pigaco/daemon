#pragma once

namespace piga
{
namespace daemon 
{
namespace sdk 
{
class DBusManager
{
public:
    virtual bool Reboot() = 0;
    virtual bool ReloadUnit(const std::string &serviceName) = 0;
};
}
}
}
