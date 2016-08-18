#include <piga/daemon/AppManager.hpp>
#include <boost/filesystem.hpp>

namespace piga
{
namespace daemon
{

AppManager::AppManager(const std::string &directory, uid_t defaultUID, char **envp)
    : m_directory(directory), m_defaultUID(defaultUID), m_envp(envp)
{

}
AppManager::~AppManager()
{

}
void AppManager::reload(const std::string &directory)
{
    m_directory = directory;

    using namespace boost::filesystem;
    directory_iterator it{path{directory}};

    for (;
        it != directory_iterator{}; ++it) {
        std::shared_ptr<App> app(new App(m_directory, m_defaultUID, m_envp));

        app->loadFromPath(it->path().string(), false);

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

    // The internal map sorts the entries after their names. No additional sorting
    // has to be done
    processApps();
}
void AppManager::update()
{
    for(auto &app : m_apps) {
        app.second->update();
    }
}
void AppManager::processApps()
{
    AppMap::iterator app;
    if(m_continueAfterWait) {
        app = m_currentPos;
        m_continueAfterWait = false;
    }
    else
        app = m_apps.begin();

    for(;app != m_apps.end(); ++app) {
		app->second->executeAutostart();

        if(app->second->shouldWaitForSignal()) {
            m_continueAfterWait = true;
            m_currentPos = ++app;
            return;
        }
    }
}


}
}
