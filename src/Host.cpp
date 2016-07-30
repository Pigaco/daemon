#include <piga/daemon/Host.hpp>
#include <dlfcn.h>
#include <boost/log/trivial.hpp>
#include <piga/hosts/host.h>
#include <piga/event.h>
#include <piga/event_game_input.h>
#include <boost/asio/deadline_timer.hpp>
#include <functional>
#include <chrono>

namespace piga
{
namespace daemon
{

std::shared_ptr<piga_host> Host::m_globalHost = std::shared_ptr<piga_host>(nullptr);
thread_local std::shared_ptr<piga_event> Host::m_cacheEvent = std::shared_ptr<piga_event>(nullptr);

Host::Host(const std::string &path, std::shared_ptr<boost::asio::io_service> io_service, std::shared_ptr<piga_host> globalHost)
    : m_path(path), m_io_service(io_service)
{
    // Set the global host
    Host::m_globalHost = globalHost;
    Host::m_cacheEvent = std::shared_ptr<piga_event>(piga_event_create(), piga_event_free);

    // Try to open the handle.
    m_dlHandle = dlopen(path.c_str(), RTLD_LAZY);

    if(m_dlHandle != nullptr)
    {
        //Load informational functions.
        m_getPigaMajorVersion = dlsym(m_dlHandle, "getPigaMajorVersion");
        m_getPigaMinorVersion = dlsym(m_dlHandle, "getPigaMinorVersion");
        m_getPigaMiniVersion = dlsym(m_dlHandle, "getPigaMiniVersion");
        m_getName = dlsym(m_dlHandle, "getName");
        m_getDescription = dlsym(m_dlHandle, "getDescription");
        m_getAuthor = dlsym(m_dlHandle, "getAuthor");
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Could not open the dlhandle to \"" << path << "\".";
        return;
    }
}

Host::~Host()
{
    destroy();
}

void Host::init(int playerCount)
{
    destroy();

    m_dlHandle = dlopen(m_path.c_str(), RTLD_LAZY);

    //Load functions
    m_getButtonState = dlsym(m_dlHandle, "getButtonState");
    m_init = dlsym(m_dlHandle, "init");
    m_useUpdateFunction = dlsym(m_dlHandle, "useUpdateFunction");
    m_update = dlsym(m_dlHandle, "update");
    m_destroy = dlsym(m_dlHandle, "destroy");
    m_getPigaMajorVersion = dlsym(m_dlHandle, "getPigaMajorVersion");
    m_getPigaMinorVersion = dlsym(m_dlHandle, "getPigaMinorVersion");
    m_getPigaMiniVersion = dlsym(m_dlHandle, "getPigaMiniVersion");
    m_getName = dlsym(m_dlHandle, "getName");
    m_getDescription = dlsym(m_dlHandle, "getDescription");
    m_getAuthor = dlsym(m_dlHandle, "getAuthor");
    m_setInputCallback = dlsym(m_dlHandle, "setCallbackFunc");

    m_implementsOutputs = dlsym(m_dlHandle, "implementsOutputs");
    m_getOutputCount = dlsym(m_dlHandle, "getOutputCount");
    m_getOutputType = dlsym(m_dlHandle, "getOutputType");
    m_getOutputPos = dlsym(m_dlHandle, "getOutputPos");
    m_getOutputName = dlsym(m_dlHandle, "getOutputName");
    m_getOutputDescription = dlsym(m_dlHandle, "getOutputDescription");
    m_getIntOutputRangeMin = dlsym(m_dlHandle, "getIntOutputRangeMin");
    m_getIntOutputRangeMax = dlsym(m_dlHandle, "getIntOutputRangeMax");
    m_getFloatOutputRangeMin = dlsym(m_dlHandle, "getFloatOutputRangeMin");
    m_getFloatOutputRangeMax = dlsym(m_dlHandle, "getFloatOutputRangeMax");
    m_getDoubleOutputRangeMin = dlsym(m_dlHandle, "getDoubleOutputRangeMin");
    m_getDoubleOutputRangeMax = dlsym(m_dlHandle, "getDoubleOutputRangeMax");
    m_setColorOutput = dlsym(m_dlHandle, "setColorOutput");
    m_setBoolOutput = dlsym(m_dlHandle, "setBoolOutput");
    m_setStringOutput = dlsym(m_dlHandle, "setStringOutput");
    m_setIntOutput = dlsym(m_dlHandle, "setIntOutput");
    m_setFloatOutput = dlsym(m_dlHandle, "setFloatOutput");
    m_setDoubleOutput = dlsym(m_dlHandle, "setDoubleOutput");

    if(test())
    {
        //Init the library
        int code = ((Init) m_init)(playerCount);
        switch(code)
        {
            case HOST_RETURNCODE_USEFIXEDFUNCTION:
                m_type = FixedFunction;
                m_controls.resize(playerCount);
                for(auto &controls : m_controls)
                {
                    for(auto &control : controls)
                    {
                        control.second = 0;
                    }
                }
                BOOST_LOG_TRIVIAL(debug) << "Loading host library with the fixed function pipeline.";

                break;
            case HOST_RETURNCODE_USEINPUTMETHODS:
                BOOST_LOG_TRIVIAL(debug) << "Loading host library with the input method pipeline.";
                m_type = InputMethods;
                m_controls.clear();
                break;
            case HOST_RETURNCODE_USECALLBACK:
                BOOST_LOG_TRIVIAL(debug) << "Loading host library with the callback pipeline.";
                m_type = InputCallback;
                setInputCallback(&Host::globalInputCallback);
                break;
            default:
                m_type = Undefined;
                break;
        }

        if(m_useUpdateFunction) {
            float use = useUpdateFunction();
            if(use != 0) {
                m_timer = std::make_shared<boost::asio::deadline_timer>(*m_io_service,
                    boost::posix_time::millisec(useUpdateFunction()));

                std::function<void(const boost::system::error_code&)> handler = [&](const boost::system::error_code& error) {
                    if(!error) {
                        update();

                        m_timer->async_wait(handler);
                    }
                };

                m_timer->async_wait(handler);
            }
        }

        BOOST_LOG_TRIVIAL(info) << "Loaded shared object \"" << getName() << "\" with the API-Version " << getPigaMajorVersion() << "." << getPigaMinorVersion() << "." << getPigaMiniVersion()
             << " - the daemon is running on " << HOST_VERSION_MAJOR << "." << HOST_VERSION_MINOR << "." << HOST_VERSION_MINI;
    }
    else
    {
        BOOST_LOG_TRIVIAL(warning) << "Shared object \"" << m_path << "\" could not be loaded because of an API version mismatch!";
        BOOST_LOG_TRIVIAL(warning) << "The shared object was compiled with Piga Host API-Version " << getPigaMajorVersion() << "." << getPigaMinorVersion() << "." << getPigaMiniVersion()
             << ", while the daemon is running on " << HOST_VERSION_MAJOR << "." << HOST_VERSION_MINOR << "." << HOST_VERSION_MINI << ".";
    }
}

bool Host::test()
{
    if(HOST_VERSION_MAJOR != getPigaMajorVersion())
    {
        return false;
    }
    if(HOST_VERSION_MINOR != getPigaMinorVersion())
    {
        return true;
    }
    if(HOST_VERSION_MINI != getPigaMiniVersion())
    {
        return true;
    }
    return true;
}

void Host::destroy()
{
    if(m_dlHandle != nullptr)
    {
        BOOST_LOG_TRIVIAL(debug) << "Destroyed shared library \"" << getName() << "\".";

        dlclose(m_dlHandle);
        m_dlHandle = nullptr;

        m_getPigaMajorVersion = nullptr;
        m_getPigaMinorVersion = nullptr;
        m_getPigaMiniVersion = nullptr;
        m_init = nullptr;
        m_useUpdateFunction = nullptr;
        m_update = nullptr;
        m_destroy = nullptr;
        m_getButtonState = nullptr;
        m_getName = nullptr;
        m_getDescription = nullptr;
        m_getAuthor = nullptr;
        m_setInputCallback = nullptr;
        m_implementsOutputs = nullptr;
        m_getOutputCount = nullptr;
        m_getOutputType = nullptr;
        m_getOutputPos = nullptr;
        m_getOutputName = nullptr;
        m_getOutputDescription = nullptr;
        m_getIntOutputRangeMin = nullptr;
        m_getIntOutputRangeMax = nullptr;
        m_getFloatOutputRangeMin = nullptr;
        m_getFloatOutputRangeMax = nullptr;
        m_getDoubleOutputRangeMin = nullptr;
        m_getDoubleOutputRangeMax = nullptr;
        m_setColorOutput = nullptr;
        m_setBoolOutput = nullptr;
        m_setStringOutput = nullptr;
        m_setIntOutput = nullptr;
        m_setFloatOutput = nullptr;
        m_setDoubleOutput = nullptr;
    }
}

int Host::getPigaMajorVersion()
{
    return ((GetPigaMajorVersion) m_getPigaMajorVersion)();
}

int Host::getPigaMinorVersion()
{
    return ((GetPigaMinorVersion) m_getPigaMinorVersion)();
}

int Host::getPigaMiniVersion()
{
    return ((GetPigaMiniVersion) m_getPigaMiniVersion)();
}

const char *Host::getName()
{
    if(m_getName != nullptr)
        return ((GetString) m_getName)();
    else
        return "";
}

const char *Host::getDescription()
{
    if(m_getDescription != nullptr)
        return ((GetString) m_getDescription)();
    else
        return "";
}

const char *Host::getAuthor()
{
    if(m_getAuthor != nullptr)
        return ((GetString) m_getAuthor)();
    else
        return "";
}

void Host::setInputCallback(Host::InputCallbackFunctionType callback)
{
    ((SetInputCallback) m_setInputCallback)(callback);
}

void Host::inputCallback(int controlCode, int playerID, int value)
{
    m_controls[playerID][controlCode] = value;
}

bool Host::implementsOutputs()
{
    return ((ImplementsOutputs) m_implementsOutputs)();
}

int Host::getOutputCount()
{
    return ((GetOutputCount) m_getOutputCount)();
}

int Host::getOutputType(int outputID)
{
    return ((GetOutputType) m_getOutputType)(outputID);
}

int Host::getOutputPos(int outputID)
{
    return ((GetOutputPos) m_getOutputPos)(outputID);
}

const char *Host::getOutputName(int outputID)
{
    return ((GetOutputName) m_getOutputName)(outputID);
}

const char *Host::getOutputDescription(int outputID)
{
    return ((GetOutputDescription) m_getOutputDescription)(outputID);
}

int Host::getIntOutputRangeMin(int outputID)
{
    return ((GetIntOutputRangeMin) m_getIntOutputRangeMin)(outputID);
}

int Host::getIntOutputRangeMax(int outputID)
{
    return ((GetIntOutputRangeMax) m_getIntOutputRangeMax)(outputID);
}

float Host::getFloatOutputRangeMin(int outputID)
{
    return ((GetFloatOutputRangeMin) m_getFloatOutputRangeMin)(outputID);
}

float Host::getFloatOutputRangeMax(int outputID)
{
    return ((GetFloatOutputRangeMax) m_getFloatOutputRangeMax)(outputID);
}

double Host::getDoubleOutputRangeMin(int outputID)
{
    return ((GetDoubleOutputRangeMin) m_getDoubleOutputRangeMin)(outputID);
}

double Host::getDoubleOutputRangeMax(int outputID)
{
    return ((GetDoubleOutputRangeMax) m_getDoubleOutputRangeMax)(outputID);
}

int Host::setColorOutput(int outputID, float r, float g, float b, float a)
{
    return ((SetColorOutput) m_setColorOutput)(outputID, r, g, b, a);
}

int Host::setBoolOutput(int outputID, int value)
{
    return ((SetBoolOutput) m_setBoolOutput)(outputID, value);
}

int Host::setStringOutput(int outputID, const char *outString)
{
    return ((SetStringOutput) m_setStringOutput)(outputID, outString);
}

int Host::setIntOutput(int outputID, int value)
{
    return ((SetIntOutput) m_setIntOutput)(outputID, value);
}

int Host::setFloatOutput(int outputID, float value)
{
    return ((SetFloatOutput) m_setFloatOutput)(outputID, value);
}

int Host::setDoubleOutput(int outputID, double value)
{
    return ((SetDoubleOutput) m_setDoubleOutput)(outputID, value);
}

float Host::useUpdateFunction()
{
    return ((UseUpdateFunction) m_useUpdateFunction)();
}
void Host::update()
{
    ((Update) m_update)();
}
void Host::globalInputCallback(int player, int input, int value)
{
    piga_host_set_player_input(m_globalHost.get(), player, input, value);
    piga_event_set_type(m_cacheEvent.get(), PIGA_EVENT_GAME_INPUT);
    piga_event_game_input *evInput = piga_event_get_game_input(m_cacheEvent.get());
    piga_event_game_input_set_input_id(evInput, static_cast<char>(input));
    piga_event_game_input_set_player_id(evInput, static_cast<char>(player));
    piga_event_game_input_set_input_value(evInput, value);

    piga_host_push_event(m_globalHost.get(), m_cacheEvent.get());
}

}
}
