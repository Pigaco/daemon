#include <piga/daemon/AppManager.hpp>
#include <boost/filesystem.hpp>

namespace piga
{
namespace daemon
{

AppManager::AppManager(const std::string &directory, uid_t defaultUID)
    : m_directory(directory), m_defaultUID(defaultUID)
{

}
AppManager::~AppManager()
{

}
void AppManager::reload(const std::string &directory)
{
    m_directory = directory;

    using namespace boost::filesystem;

    for (directory_iterator it{path{directory}};
        it != directory_iterator{}; ++it) {
        std::shared_ptr<App> app(new App(m_directory, m_defaultUID));

        app->loadFromPath(it->path().string(), false);
        app->executeAutostart();

        if(app->isInstalled()) {
            // Only if the parsing was good enough, the app is installed. Then it can be added to the internal map.

            if(m_apps.count(app->getName()) > 0) {
                // Reloading makes the currently running app reload its configuration.
                m_apps[app->getName()]->reload(false);
            } else {
                m_apps[app->getName()] = app;
            }
        }
    }
}
void AppManager::update()
{
    for(auto &app : m_apps) {
        app.second->update();
    }
}


}
}
