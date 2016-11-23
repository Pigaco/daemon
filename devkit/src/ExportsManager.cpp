#include <piga/devkit/ExportsManager.hpp>
#include <algorithm>
#include <fstream>

#include <piga/devkit/Devkit.hpp>

namespace piga
{
namespace devkit
{
ExportsManager::ExportsManager(Devkit *devkit)
    : m_devkit(devkit)
{
    
}
ExportsManager::~ExportsManager()
{
    
}
void ExportsManager::setExport(const std::string &path, const std::string &host)
{
    std::vector<std::string> &hosts = m_exports[path];
    if(std::find(hosts.begin(), hosts.end(), host) == hosts.end()) {
        hosts.push_back(host);
    }
    
    exportPathToHost(path, host);
}
void ExportsManager::removeExport(const std::string& path, const std::string& host)
{
    std::vector<std::string> &hosts = m_exports[path];
    
    auto found = std::find(hosts.begin(), hosts.end(), host);
    
    if(found != hosts.end()) {
        hosts.erase(found);
    }
    
    if(hosts.size() == 0) {
        m_exports.erase(path);
    }
    
    unexportPathToHost(path, host);
}
void ExportsManager::readFromConfigFile()
{
    std::ifstream exportsFile;
    exportsFile.open(m_exportsFile);
    
    std::string line;
    std::string path;
    
    std::string hosts;
    while(std::getline(exportsFile, line)) {
        if(line.find_first_of("\t") == std::string::npos) {
            continue;
        }
        
        hosts = line.substr(line.find_first_of("\t") + 1);
        path = line.substr(1, line.find_first_of("\t") - 2);
        
        std::string host;
        for(std::size_t i = 0; true; i = hosts.find_first_of(',', i) + 1) {
            host = hosts.substr(i, hosts.find_first_of('(', i) - i);
            setExport(path, host);
            if(hosts.find_first_of(',', i) == std::string::npos) {
                break;
            }
            
            exportPathToHost(path, host);
        }
    }
}

void ExportsManager::updateExportsFile()
{
    std::ofstream exportsFile;
    exportsFile.open(m_exportsFile, std::ios::out | std::ios::trunc);
    
    exportsFile << "# THIS FILE IS AUTOMATICALLY GENERATED BY THE PIGACO DEVKIT!" << std::endl;
    exportsFile << "# Modifications will not be permanent. Use the devkit API instead." << std::endl;
    
    for(auto &exportEntry : m_exports) {
        exportsFile << "\"" << exportEntry.first << "\"" << "\t";
        for(auto host = exportEntry.second.begin(); host != exportEntry.second.end(); ++host) {
            exportsFile << *host << "(rw)";
            if(host + 1 != exportEntry.second.end()) {
                exportsFile << ",";
            }
            else {
                exportsFile << std::endl;
            }
        }
    }
    
    exportsFile << std::endl;
    exportsFile.close();
}
void ExportsManager::updateNFS()
{
    // TODO Ugly hack to reload exports.
    system("exportfs -ra");
}
void ExportsManager::exportPathToHost(const std::string& path, const std::string& host)
{
    system(("exportfs -o " + host + ":" + path).c_str());
}
void ExportsManager::unexportPathToHost(const std::string& path, const std::string& host)
{
    system(("exportfs -u " + host + ":" + path).c_str());
}

}
}
