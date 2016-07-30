#include <piga/daemon/Loader.hpp>
#include <piga/daemon/Host.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace piga
{
namespace daemon
{

Loader::Loader(const std::string &soDir, int playerCount, std::shared_ptr<boost::asio::io_service> ioService, std::shared_ptr<piga_host> globalHost)
    : m_soDir(soDir), m_playerCount(playerCount), m_ioService(ioService), m_globalHost(globalHost)
{

}

Loader::~Loader()
{

}

const std::string &Loader::getSoDir()
{
    return m_soDir;
}

void Loader::setSoDir(const std::string &soDir)
{
    m_soDir = soDir;
}

int Loader::getPlayerCount()
{
    return m_playerCount;
}

void Loader::setPlayerCount(int playerCount)
{
    m_playerCount = playerCount;
}

void Loader::reload()
{
    m_hosts.clear();

    // Iterate recursively over the soDir to find hosts, which are then loaded.
    using namespace boost::filesystem;

    for (recursive_directory_iterator it{path{m_soDir}};
         it != recursive_directory_iterator{}; ++it) {
        std::unique_ptr<Host> host(new Host(it->path().string(), m_ioService, m_globalHost));

        host->init(m_playerCount);
        m_hosts.push_back(std::move(host));
    }
}

}
}
