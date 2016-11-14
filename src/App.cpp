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
App::App(const std::string &defaultAppPath, uid_t defaultUID, char **envp)
    : m_appPath(defaultAppPath), m_uid(defaultUID), m_envp(envp)
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
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't define autostart!";
        m_autostart = false;
    }
    if(!root.lookupValue("run_as_root", m_runAsRoot)) {
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't define run_as_root!";
        m_runAsRoot = false;
    }
    if(!root.lookupValue("wait_for_signal", m_waitForSignal)) {
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't define wait_for_signal!";
        m_waitForSignal = false;
    }
    if(!root.lookupValue("restart_on_crash", m_restartOnCrash)) {
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't define restart_on_crash!";
        m_restartOnCrash = false;
    }
    if(!root.lookupValue("restart_on_exit", m_restartOnExit)) {
        BOOST_LOG_TRIVIAL(warning) << "App config file \"" << configPath << "\" doesn't define restart_on_exit!";
        m_restartOnExit = false;
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
            for(std::size_t i = 0; i < args.getLength(); ++i) {
                m_args[i] = *(args[i]);
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
    root.add("run_as_root", Setting::TypeBoolean) = false;
    root.add("wait_for_signal", Setting::TypeBoolean) = false;
    root.add("restart_on_crash", Setting::TypeBoolean) = false;
    root.add("restart_on_exit", Setting::TypeBoolean) = false;
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

        // Set the user ID to the specified user if it shouldn't be run as root.
        if(!m_runAsRoot)
			setuid(m_uid);

        setsid();

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

        if(m_executable[0] == '.') {
			execvp((m_path + m_executable).c_str(), args);
        } else {
			execvp(m_executable.c_str(), args);
        }


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
    m_waitpid_counter = 0;
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
bool App::isRunning() const
{
    return m_running;
}
bool App::isInstalled() const
{
    return m_installed;
}
pid_t App::getPid() const
{
    return m_pid;
}

inline static void handle_exit_code_and_restart(App *app, int code)
{
	if(code == EXIT_SUCCESS) {
		// This means a normal exit. Should it be restarted?
		if(app->restartOnExit())
            app->start();
	} else {
		// This means a crash. Should it be restarted?
		if(app->restartOnCrash())
            app->start();
	}
}

void App::update()
{
    if(isInstalled() && isRunning()) {
        ++m_waitpid_counter;

        if(m_waitpid_counter < 5) {
            // The waitpid function with the WNOHANG flag
            // only returns, if the process is already running or has
            // finished. The system needs some time to start the process,
            // in this case we give it 5 * 1/60 seconds.

            return;
        }

        int status = 0;
        pid_t pid = waitpid(m_pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        switch(pid) {
            // Information from http://linux.die.net/man/2/waitpid
            case 0:
                // The child did not change state yet. Do not continue to signal handling.
                return;
            case -1:
                // An error occured.
                BOOST_LOG_TRIVIAL(error) << "Waitpid on pid " << m_pid << " returned an error!";
                break;
            default:
                // This indicates success! Continue to the signal handling.
                // (Because only the m_pid variable is defined, the pid greater than 0 means
                // success.)
                break;
        }

        // Handle the result
        if(WIFEXITED(status)) {
            m_running = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" exited with status \"" << WEXITSTATUS(status) << "\"";

            handle_exit_code_and_restart(this, WEXITSTATUS(status));
        } else if(WIFSIGNALED(status)) {
            m_running = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" killed by signal \"" << WTERMSIG(status) << "\"";

            handle_exit_code_and_restart(this, WEXITSTATUS(status));
        } else if(WIFSTOPPED(status)) {
            m_stopped = true;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" stopped by signal \"" << WSTOPSIG(status) << "\"";
        } else if(WIFCONTINUED(status)) {
            m_stopped = false;
            BOOST_LOG_TRIVIAL(debug) << "App \"" << m_name << "\" continued.";
        }
    }
}
bool App::restartOnExit() const
{
    return m_restartOnExit;
}
bool App::restartOnCrash() const
{
    return m_restartOnCrash;
}
bool App::shouldWaitForSignal() const
{
    return m_waitForSignal;
}
const std::string &App::getPath() const
{
    return m_path;
}
const std::string &App::getName() const
{
    return m_name;
}
const std::string &App::getWorkingDir() const
{
    return m_workingDir;
}
const std::string &App::getExecutable() const
{
    return m_executable;
}
bool App::isAutostart() const
{
    return m_autostart;
}
}
}
