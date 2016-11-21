#include <piga/daemon/Daemon.hpp>
#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <functional>
#include <fstream>

#include <libconfig.h++>
#include <iostream>

#include <piga/host_config.h>
#include <piga/event.h>

#include <piga/event_request_restart.h>

#include <piga/devkit/Devkit.hpp>
#include <piga/daemon/DBusManager.hpp>
#include <piga/daemon/PluginManager.hpp>

using std::endl;
using std::cout;

namespace piga
{
namespace daemon
{

namespace as = ::boost::asio;

Daemon::Daemon(char **envp)
    : m_io_service(std::make_shared<as::io_service>()),
      m_dbusManager(std::make_shared<DBusManager>()),
      m_work(std::make_shared<as::io_service::work>(*m_io_service)),
      m_signals(*m_io_service, SIGINT, SIGHUP, SIGUSR1),
      m_envp(envp)
{
    m_signals.async_wait(boost::bind(&Daemon::signalHandler, this, _1, _2));

    // Try to write the pidfile.
    pid_t pid = getpid();
    std::ofstream pidfile;
    pidfile.open(PIGA_DAEMON_PIDFILE_PATH, std::ios::trunc | std::ios::out);
    if(pidfile.is_open()) {
		pidfile << pid;
		pidfile.close();
        setenv("PIGA_DAEMON_PIDFILE_PATH", PIGA_DAEMON_PIDFILE_PATH, 1);
    } else {
        std::string path = boost::filesystem::current_path().string();
		BOOST_LOG_TRIVIAL(error) << "Could not create pidfile in \"" << PIGA_DAEMON_PIDFILE_PATH << "\". Does the daemon have the neccessary access rights?";
        path += "/.pigadaemon.pid";
        BOOST_LOG_TRIVIAL(info) << "Creating local pidfile and setting he envvar PIGA_DAEMON_PIDFILE_PATH to " << path;

		pidfile.open(path, std::ios::trunc | std::ios::out);
		if(pidfile.is_open()) {
			setenv("PIGA_DAEMON_PIDFILE_PATH", path.c_str(), 1);
			pidfile << pid;
            pidfile.close();
        } else {
            BOOST_LOG_TRIVIAL(error) << "The local pidfile in \"" << path << "\" could not be opened!";
        }
    }
    
    m_pluginManager = std::make_shared<PluginManager>(
        m_dbusManager,
        m_io_service,
        nullptr
    );
}

Daemon::~Daemon()
{
    BOOST_LOG_TRIVIAL(info) << "Shutting down pigadaemon.";
    // Remove the pidfile.
    std::remove(PIGA_DAEMON_PIDFILE_PATH);
    bool removed = !std::ifstream(PIGA_DAEMON_PIDFILE_PATH);
    if(!removed) {
		BOOST_LOG_TRIVIAL(error) << "Could not remove pidfile in \"" << PIGA_DAEMON_PIDFILE_PATH << "\". Does the daemon have the neccessary access rights? - Trying local pidfile.";
		std::remove(getenv("PIGA_DAEMON_PIDFILE_PATH"));
		bool removed = !std::ifstream(getenv("PIGA_DAEMON_PIDFILE_PATH"));
        if(!removed) {
            BOOST_LOG_TRIVIAL(error) << "Could not remove pidfile in \"" << getenv("PIGA_DAEMON_PIDFILE_PATH") << "\". Was it created?";
        }
    }
    // Unset the environment variable.
    BOOST_LOG_TRIVIAL(debug) << "Clearing envvar PIGA_DAEMON_PIDFILE_PATH, which had the content \"" << getenv("PIGA_DAEMON_PIDFILE_PATH") << "\"";	
	unsetenv("PIGA_DAEMON_PIDFILE_PATH");
}

void Daemon::run()
{
    reload();

    piga_host_config *cfg = piga_host_config_default();
    piga_host_config_set_name(cfg, m_name.c_str());
    m_host = std::shared_ptr<piga_host>(piga_host_create(), piga_host_free);

    m_cacheEvent = std::shared_ptr<piga_event>(piga_event_create(), piga_event_free);

    piga_host_consume_config(m_host.get(), cfg);
    piga_status status = piga_host_startup(m_host.get());

    if(status != PIGA_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "Could not start piga_host: " << piga_status_what_copy(status);
        return;
    }
    
    m_dbusManager->init();

    m_loader = std::unique_ptr<Loader>(new Loader(m_soPath,
                                                  piga_host_config_get_player_count(cfg),
                                                  m_io_service,
                                                  m_host));

    m_loader->reload();

    m_appManager = std::make_shared<AppManager>(m_defaultAppPath, m_defaultUID, m_envp);
    m_appManager->reload(m_defaultAppPath);
    
    m_pluginManager->setAppManager(m_appManager);

    // Host loaded. Now load the client for the daemon.
    piga_client_config *client_cfg = piga_client_config_default();
    piga_client_config_set_player_count(client_cfg, piga_host_config_get_player_count(cfg));
    piga_client_config_set_shared_memory_key(client_cfg, piga_host_config_get_shared_memory_key(cfg));

    m_client = std::shared_ptr<piga_client>(piga_client_create(), piga_client_free);
    piga_client_consume_config(m_client.get(), client_cfg);
    piga_client_connect(m_client.get(), "Pigadaemon-Internal-Client", sizeof("Pigadaemon-Internal-Client"));

    m_hostTimer = std::make_shared<as::deadline_timer>(*m_io_service, boost::posix_time::milliseconds(1));

    m_hostTimer->async_wait(std::bind(&Daemon::update, this));
    
    m_io_service->run();
}

void Daemon::stop()
{
    BOOST_LOG_TRIVIAL(info) << "Stopping the pigadaemon";
    m_io_service->stop();
}

void Daemon::reload()
{
    BOOST_LOG_TRIVIAL(info) << "Loading configuration of pigadaemon.";
    // Load the configuration file.
    using namespace libconfig;

    Config cfg;

    bool warningHappened = false;

    try {
        cfg.readFile(m_configFilePath.c_str());
    }
    catch(const FileIOException &e) {
        BOOST_LOG_TRIVIAL(error) << "I/O error while reading file \"" << m_configFilePath << "\"";
        warningHappened = true;
    }
    catch(const ParseException &e) {
        BOOST_LOG_TRIVIAL(error) << "Parse error at \"" << e.getFile() << "\":" << e.getLine() << " : " << e.getError();
        warningHappened = true;
        return;
    }

    if(!warningHappened)
    {
        try {
            Setting &root = cfg.getRoot();

            if(!root.exists("piga")) {
                BOOST_LOG_TRIVIAL(warning) << "The \"piga\" config is missing! Using default values.";
                m_name = "Unnamed Piga Host";
                warningHappened = true;
            } else {
                root["piga"].lookupValue("name", m_name);
            }

            if(!root.exists("hosts")) {
                BOOST_LOG_TRIVIAL(warning) << "The \"hosts\" config is missing! Using default values.";
                m_soPath = "/usr/lib/piga/hosts/";
                warningHappened = true;
            } else {
                root["hosts"].lookupValue("so_path", m_soPath);
            }
            
            if(!root.exists("devkit")) {
                BOOST_LOG_TRIVIAL(warning) << "The \"devkit\" config is missing! Using default values.";
                m_devkitActive = false;
                m_devkitHttpPort = 8080;
                warningHappened = true;
            } else {
                root["devkit"].lookupValue("active", m_devkitActive);
                root["devkit"].lookupValue("port", m_devkitHttpPort);
                
                // Check for the devkit.
                if(m_devkitActive && !m_devkit)  {
                    m_devkit = m_pluginManager->activatePlugin<::piga::devkit::Devkit>("Devkit");
                    m_devkit->setHTTPPort(m_devkitHttpPort);
                    m_devkit->start();
                } else if(!m_devkitActive && m_devkit) {
                    m_pluginManager->removePlugin("Devkit");
                }
                
                if(root["devkit"].exists("tokens")) {
                    Setting &tokens = root["devkit"]["tokens"];
                    
                    std::string token;
                    std::vector<::piga::devkit::Devkit::DevkitAction> actions;
                    
                    for(std::size_t i = 0; i < tokens.getLength(); ++i) {
                        tokens[i].lookupValue("token", token);
                        for(std::size_t n = 0; n < tokens[i]["actions"].getLength(); ++n) {
                            actions.push_back(::piga::devkit::Devkit::getActionFromStr(
                                tokens[i]["actions"][n]));
                        }
                        
                        m_devkit->setAllowedActionsForToken(token, actions);
                    }
                } else {
                    BOOST_LOG_TRIVIAL(warning) << "The \"devkit\" config has no defined tokens! Please define them in the order \"tokens = ({token = \"TOKEN\"; actions = (\"Export\", \"...\", ...); } );";
                }
            }

            if(!root.exists("apps")) {
                BOOST_LOG_TRIVIAL(warning) << "The \"apps\" config is missing! Using default values.";
                warningHappened = true;
            } else {
                root["apps"].lookupValue("default_uid", m_defaultUID);
                root["apps"].lookupValue("app_path", m_defaultAppPath);
            }
        }
        catch(std::exception &e) {
            BOOST_LOG_TRIVIAL(warning) << "Caught an exception while parsing the config: " << e.what();
            warningHappened = true;
        }
    }

    // Sample config file generator.
    if(warningHappened) {
        Config cfg;
        Setting &root = cfg.getRoot();
        root.add("piga", Setting::TypeGroup);
        {
            Setting &piga = root["piga"];
            piga.add("name", Setting::TypeString) = m_name;
        }
        root.add("devkit", Setting::TypeGroup);
        {
            Setting &devkit= root["devkit"];
            devkit.add("active", Setting::TypeBoolean) = true;
            devkit.add("port", Setting::TypeInt) = 8080;
        }
        root.add("hosts", Setting::TypeGroup);
        {
            Setting &hosts = root["hosts"];
            hosts.add("so_path", Setting::TypeString) = m_soPath;
        }
        root.add("apps", Setting::TypeGroup);
        {
            Setting &apps = root["apps"];
            apps.add("default_uid", Setting::TypeInt) = static_cast<int>(m_defaultUID);
            apps.add("app_path", Setting::TypeString) = m_defaultAppPath;
        }

        std::string samplePath = m_configFilePath + ".sample";
        BOOST_LOG_TRIVIAL(info) << "Because warnings happened while reading the config, a sample config file was generated and placed into " << samplePath;
        try {
            cfg.writeFile(samplePath.c_str());
        }
        catch(FileIOException &e) {
            BOOST_LOG_TRIVIAL(error) << "I/O error while writing sample config file \"" << samplePath << "\"";
        }
    }

    // Also reload all hosts, if the loader is already loaded (after the first start).
    if(m_loader) {
        m_loader->setSoDir(m_soPath);

        m_loader->reload();
    }
}

void Daemon::signalHandler(const boost::system::error_code &code, int signal_number)
{
    switch(signal_number) {
        case SIGINT:
            stop();
            break;
        case SIGTERM:
            stop();
            break;
        case SIGHUP:
            reload();
            break;
        case SIGUSR1:
            // This signal means, that the app processing should continue.
            BOOST_LOG_TRIVIAL(info) << "Received a SIGUSR1, this means the daemon continues processing the apps now.";
            m_appManager->processApps();
            break;
    }
    m_signals.async_wait(boost::bind(&Daemon::signalHandler, this, _1, _2));
}
void Daemon::update()
{
    if(m_host) {
        // This is the host update function.
        piga_host_update(m_host.get());

        piga_event_queue *clientQueue = piga_client_get_in_queue(m_client.get());
        
        std::shared_ptr<sdk::App> app;
        
        piga_event_request_restart *event_restart = nullptr;
        
        while(piga_event_queue_poll(clientQueue, m_cacheEvent.get()) == PIGA_STATUS_OK) {
            switch(piga_event_get_type(m_cacheEvent.get())) {
                case PIGA_EVENT_REQUEST_KEYBOARD:       // UNHANDLED
                    break;
                case PIGA_EVENT_REQUEST_RESTART:
                    // An app should be started or restarted.
                    event_restart = piga_event_get_request_restart(m_cacheEvent.get());
                    piga_event_request_restart_get_name(event_restart, m_cacheBuffer);
                    app = (*m_appManager)[m_cacheBuffer];
                    if(app->isRunning()) {
                        app->stop();
                    }
                    app->reload();
                    app->start();
                    break;
                case PIGA_EVENT_CONSUMER_REGISTERED:    // UNHANDLED
                    break;
                case PIGA_EVENT_CONSUMER_UNREGISTERED:  // UNHANDLED
                    break;
                case PIGA_EVENT_PLAYER_JOINED:
                    break;
                case PIGA_EVENT_PLAYER_LEFT:
                    break;
                case PIGA_EVENT_TEXT_INPUT:
                    break;
                case PIGA_EVENT_GAME_INPUT:
                    break;
                case PIGA_EVENT_APP_INSTALLED:
                    break;
                case PIGA_EVENT_UNKNOWN:
                    break;
            }
        }

        m_hostTimer->expires_from_now(boost::posix_time::milliseconds(20));
        m_hostTimer->async_wait(std::bind(&Daemon::update, this));
    }
    if(m_appManager) {
        m_appManager->update();
    }
}
}
}
