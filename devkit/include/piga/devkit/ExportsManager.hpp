#pragma once

#include <string>
#include <map>
#include <vector>

namespace piga
{
namespace devkit
{
class Devkit;

/**
 * @brief This class writes the exports into the specified exports file. 
 * 
 * This is useful to keep exports after a reboot.
 */
class ExportsManager
{
public:
    ExportsManager(Devkit *devkit);
    ~ExportsManager();
    
    void setExport(const std::string &path, const std::string &host);
    void removeExport(const std::string &path, const std::string &host);
    
    void readFromConfigFile();
    void updateExportsFile();
    void updateNFS();
private:
    std::string m_exportsFile = "/etc/exports.d/piga-nfs-exports.cfg";
    Devkit *m_devkit = nullptr;
    
    std::map<std::string, std::vector<std::string>> m_exports;
};
}
}
