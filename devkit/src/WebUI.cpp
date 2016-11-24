#include <sol.hpp>

#include <boost/filesystem.hpp>

#include <piga/devkit/Devkit.hpp>
#include <piga/devkit/WebUI.hpp>

namespace piga
{
namespace devkit
{
WebUI::WebUI(Devkit *devkit)
    : m_devkit(devkit)
{
    m_lua.open_libraries(sol::lib::base);
    m_lua.open_libraries(sol::lib::package);
    m_lua.open_libraries(sol::lib::string);
    m_lua.open_libraries(sol::lib::os);
    m_lua.open_libraries(sol::lib::table);
    m_lua.open_libraries(sol::lib::io);

    std::string path = m_lua["package"]["path"];
    m_lua["package"]["path"] =
        path + ";" + (boost::filesystem::current_path() / boost::filesystem::path("../devkit/web/")).string() + "?.lua;"
        "/usr/share/piga/devkit/web/?.lua";
}
WebUI::~WebUI()
{

}

std::string devkit::WebUI::handleRequest(const std::string path, std::string *contentType, const std::string &token)
{

    std::string file = path;
    std::string absolutePath;
    std::string returnMessage;

    std::string includeDir;
    
    m_lua["getToken"] = [token](){
        return token;
    };

    if(path == "") {
        file = "index.lua";
    }

    try {
        if(getValidFile(file, &absolutePath, &includeDir) || getValidFile(file + ".lua", &absolutePath, &includeDir)) {
            m_lua["INCLUDEDIR"] = includeDir;

            m_lua["print"] = [&](std::string msg) {
                returnMessage += msg;
            };

            m_lua.script_file(absolutePath);

            sol::function render = m_lua["render"];
            if(render.valid()) {
                returnMessage += render();
            }

            *contentType = "text/html; charset=utf-8";
            return returnMessage;
        }
    }
    catch(sol::error &e) {
        return std::string("Lua error (501): ") + e.what();
    }
    catch(...) {
        return "ERROR 501";
    }

    return "ERROR 404";
}
bool WebUI::getValidFile(const std::string &path, std::string *out, std::string *includeDir)
{
    // Check if this is a file.
    boost::filesystem::path fullPath1(boost::filesystem::current_path() / boost::filesystem::path("../devkit/web/" + path));
    boost::filesystem::path fullPath2("/usr/share/piga/devkit/web/" + path);
    if(boost::filesystem::exists(fullPath1)) {
        *out = fullPath1.string();
        *includeDir = (boost::filesystem::current_path() / boost::filesystem::path("../devkit/web/")).string();
        return true;
    } else if(boost::filesystem::exists(fullPath2)) {
        *out = fullPath2.string();
        *includeDir = "/usr/share/piga/devkit/web/";
        return true;
    }
    return false;
}
}
}
