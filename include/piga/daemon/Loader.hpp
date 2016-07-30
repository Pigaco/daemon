#ifndef PIGA_DAEMON_LOADER_HPP_INCLUDED
#define PIGA_DAEMON_LOADER_HPP_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <boost/asio/io_service.hpp>

#include <piga/host.h>

namespace piga
{
namespace daemon
{
class Host;

class Loader
{
public:
    Loader(const std::string &soDir, int playerCount, std::shared_ptr<boost::asio::io_service> ioService, std::shared_ptr<piga_host> globalHost);
    ~Loader();

    const std::string& getSoDir();
    void setSoDir(const std::string &soDir);

    int getPlayerCount();
    void setPlayerCount(int playerCount);

    /**
     * @brief Reloads all loaded files.
     *
     * Including:
     *   * All loaded hosts in the m_soDir.
     */
    void reload();
private:
    std::vector<std::shared_ptr<Host>> m_hosts;
    std::string m_soDir;
    int m_playerCount;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    std::shared_ptr<piga_host> m_globalHost;
};
}
}

#endif
