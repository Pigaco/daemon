#pragma once

#include <memory>
#include <string>

#include <sol.hpp>

namespace piga
{
namespace devkit
{
class Devkit;    

/**
 * The WebUI class handles the rendering of 
 */
class WebUI
{
public:
    WebUI(Devkit *devkit);
    ~WebUI();
    
    std::string handleRequest(const std::string path, std::string *contentType, const std::string &token);
    
    bool getValidFile(const std::string &path, std::string *out, std::string *includeDir);
private:
    sol::state m_lua;
    Devkit *m_devkit;
};
}
}
