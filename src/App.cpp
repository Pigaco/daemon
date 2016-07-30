#include <piga/daemon/App.hpp>
#include <libconfig.h++>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

namespace piga
{
namespace daemon
{
App::App(const std::string &defaultAppPath, uid_t defaultUID)
    : m_appPath(defaultAppPath), m_uid(defaultUID)
{
    m_args.resize(1);
}
App::~App()
{
    if(isRunning()) {
        stop();
    }
}
void App::loadFromName(const std::string &name)
{
    m_name = name;
    loadFromPath(m_appPath + name, true);
}
void App::loadFromPath(const std::string &path, bool autostart_active)
{
    m_path = path;
    m_workingDir = path;
    if(loadConfigFile(std::string(path + "/app_config.cfg"))) {
        BOOST_LOG_TRIVIAL(info) << "App stub \"" << m_name << "\" successfully loaded into the internal database from \"" << path << "\".";
        m_installed = true;

        if(autostart_active && m_autostart) {
            BOOST_LOG_TRIVIAL(info) << "Autostart of app \"" << m_name << "\" is active. It will now be started.";
            start();
        }
    }
    else {
        BOOST_LOG_TRIVIAL(error) << "App stub could not be loaded into the internal database from \"" << path << "\".";
        m_installed = false;
    }
}
bool App::loadConfigFile(const std::string &configPath)
{
    using namespace libconfig;
    Config cfg;
    try {
        cfg.readFile(configPath.c_str());
    }
    catch(const FileIOException &e) {
        BOOST_LOG_TRIVIAL(error) << "File IO error while trying to read config file \"" << configPath << "\"";
        return false;
    }
    catch(const ParseException &e) {
        BOOST_LOG_TRIVIAL(error) << "Parse error in \"" << e.getFile() << "\":" << e.getLine() << " : " << e.getError();
        return false;
    }

    Setting &root = cfg.getRoot();

    if(!root.lookupValue("name", m_name)) {
        BOOST_LOG_TRIVIAL(error) << "App config file \"" << configPath << "\" doesn't define a name!";
        m_name = "Undefined Name";
        return false;
    }

    if(!root.lookupValue("autostart", m_autostart)) {
        BOOST_LOG_TRIVIAL(error) << "App config file \"" << configPath << "\" doesn't define autostart!";
        m_autostart = false;
    }

    if(!root.exists("execution")) {
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't have an execution group. It can therefore not be executed.";
    } else {
        Setting &execution = root["execution"];

        if(!execution.lookupValue("executable", m_executable))
            BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't have an executable. It can therefore not be executed.";

        try {
            Setting &args = execution.lookup("arguments");
            m_args.resize(args.getLength() + 1);
            std::size_t i = 1;
            for(auto &arg : args) {
                m_args[i] = *arg;
                ++i;
            }
        }
        catch (const SettingNotFoundException &e) {
            BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't specify any arguments. This doesn't compede with execution.";
        }

        if(!execution.lookupValue("working_directory", m_workingDir))
            BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't specify an working directory. It will be executed in it's root folder.";

        if(!execution.lookupValue("uid", m_uid))
            BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't specify an uid. It will be executed with the default uid for apps: " << m_uid;
    }
    return true;
}
void App::generateSampleConfig(const std::string &output)
{
    using namespace libconfig;
    Config cfg;

    Setting &root = cfg.getRoot();
    root.add("name", Setting::TypeString) = "Undefined Name";
    root.add("autostart", Setting::TypeBoolean) = false;
    Setting & exec = root.add("execution", Setting::TypeGroup);
    exec.add("executable", Setting::TypeString) = "executable_relative_to_directory_path";
    exec.add("arguments", Setting::TypeArray);
    exec.add("working_directory", Setting::TypeString) = "working directory - absolute (/) or relative (./ or sth/).";
    exec.add("uid", Setting::TypeInt) = 1010;

    try {
        cfg.writeFile(output.c_str());
    }
    catch(const FileIOException &e) {
        BOOST_LOG_TRIVIAL(error) << "File I/O error while writing file \"" << output << "\".";
    }
}
void App::reload(bool start)
{
    loadFromPath(m_path, start);
}
void App::executeAutostart()
{
    if(m_autostart)
        start();
}
void App::start(bool restartIfRunning)
{
    if(isRunning()) {
        if(restartIfRunning) {
            BOOST_LOG_TRIVIAL(info) << "App \"" << m_name << "\" with executable \"" << m_path << "/" << m_executable << "\" is already running and will be restarted.";
            stop();
        } else {
            BOOST_LOG_TRIVIAL(info) << "App \"" << m_name << "\" with executable \"" << m_path << "/" << m_executable << "\" is already running and will not be restarted!";
            return;
        }
    }

    BOOST_LOG_TRIVIAL(info) << "Starting app \"" << m_name << "\" with executable \"" << m_path << "/" << m_executable << "\".";
    pid_t pid = fork();
    if(pid == 0) {
        // Child Process

        // Set the user ID to the specified user.
        setuid(m_uid);

        // Set the working directory.
        if(m_workingDir[0] != '/') {
            // The working directory is relative.
            boost::filesystem::current_path(m_path + "/" + m_workingDir);
        } else {
            // The working directory is absolute.
            boost::filesystem::current_path(m_workingDir);
        }

        m_args[0] = (m_path + "/" + m_executable).c_str();
        char **args = new char*[m_args.size() + 1];
        for(std::size_t i = 0; i < m_args.size(); ++i) {
            args[i] = new char[m_args[i].length()];
            std::memcpy(args[i], m_args[i].c_str(), m_args[i].length());
        }
        args[m_args.size()] = nullptr;

        execv((m_path + "/" + m_executable).c_str(), args);

        BOOST_LOG_TRIVIAL(error) << "Error while trying to run \"" << m_executable << "\". : " << strerror(errno);

        for(std::size_t i = 0; i < m_args.size(); ++i) {
            delete[] args[i];
        }
        delete[] args;

        // Exit this process.
        exit(-1);
    }
    else if(pid > 0) {
        // Parent Process
        m_running = true;

        m_pid = pid;
    } else if(pid < 0) {
        BOOST_LOG_TRIVIAL(error) << "Could not fork() for app \"" << m_name << "\".";
    }
}
void App::stop()
{
    kill(getPid(), SIGKILL);
    int status;
    waitpid(m_pid, &status, WUNTRACED | WCONTINUED);
    if(WIFEXITED(status)) {
        m_running = false;
        BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" exited with status \"" << WEXITSTATUS(status) << "\"";
    } else if(WIFSIGNALED(status)) {
        m_running = false;
        BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" killed by signal \"" << WTERMSIG(status) << "\"";
    }
}
bool App::isRunning()
{
    return m_running;
}
bool App::isInstalled()
{
    return m_installed;
}
pid_t App::getPid()
{
    return m_pid;
}
void App::update()
{
    if(isInstalled() && isRunning()) {
        int status = 0;
        waitpid(m_pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        // Handle the result
        if(WIFEXITED(status)) {
            m_running = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" exited with status \"" << WEXITSTATUS(status) << "\"";
        } else if(WIFSIGNALED(status)) {
            m_running = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" killed by signal \"" << WTERMSIG(status) << "\"";
        } else if(WIFSTOPPED(status)) {
            m_stopped = true;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" stopped by signal \"" << WSTOPSIG(status) << "\"";
        } else if(WIFCONTINUED(status)) {
            m_stopped = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" continued.";
        }
    }
}
const std::string &App::getPath()
{
    return m_path;
}
const std::string &App::getName()
{
    return m_name;
}
const std::string &App::getWorkingDir()
{
    return m_workingDir;
}
const std::string &App::getExecutable()
{
    return m_executable;
}
bool App::isAutostart()
{
    return m_autostart;
}
}
}
