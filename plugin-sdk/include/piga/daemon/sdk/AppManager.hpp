#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include <piga/daemon/sdk/App.hpp>

namespace piga
{
namespace daemon
{
namespace sdk
{
class AppManager
{
public:
    typedef std::shared_ptr<App> AppPtr;
    typedef std::unordered_map<std::string, AppPtr> AppMap;
    
    virtual AppPtr operator[](const std::string &name) = 0;
    virtual AppPtr getApp(const std::string &name) = 0;
};
}
}
}
