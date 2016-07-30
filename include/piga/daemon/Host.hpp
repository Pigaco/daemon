#ifndef PIGA_DAEMON_HOST_HPP_INCLUDED
#define PIGA_DAEMON_HOST_HPP_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <piga/host.h>
#include <piga/event.h>

namespace piga
{
namespace daemon
{
/**
 * @brief The Host class handles the interface between dynamic host libraries and
 * the piga backend.
 *
 * This class has been migrated and changed from the old libpiga codebase to further
 * improve decapsulation of daemon, hub and library code.
 */
class Host
{
public:
    Host(const std::string &path, std::shared_ptr<boost::asio::io_service> io_service, std::shared_ptr<piga_host> globalHost);
    ~Host();
    typedef void (*InputCallbackFunctionType)(int, int, int);

    enum Type {
        Undefined,
        FixedFunction,
        InputMethods,
        InputCallback,

        _COUNT
    };

    void init(int playerCount);
    bool test();
    void destroy();

    int getPigaMajorVersion();
    int getPigaMinorVersion();
    int getPigaMiniVersion();

    const char* getName();
    const char* getDescription();
    const char* getAuthor();

    void setInputCallback(InputCallbackFunctionType callback);
    void inputCallback(int controlCode, int playerID, int value);

    bool implementsOutputs();
    int getOutputCount();
    int getOutputType(int outputID);
    int getOutputPos(int outputID);
    const char* getOutputName(int outputID);
    const char* getOutputDescription(int outputID);
    int getIntOutputRangeMin(int outputID);
    int getIntOutputRangeMax(int outputID);
    float getFloatOutputRangeMin(int outputID);
    float getFloatOutputRangeMax(int outputID);
    double getDoubleOutputRangeMin(int outputID);
    double getDoubleOutputRangeMax(int outputID);
    int setColorOutput(int outputID, float r, float g, float b, float a);
    int setBoolOutput(int outputID, int value);
    int setStringOutput(int outputID, const char *outString);
    int setIntOutput(int outputID, int value);
    int setFloatOutput(int outputID, float value);
    int setDoubleOutput(int outputID, double value);
    float useUpdateFunction();
    void update();

    static void globalInputCallback(int player, int input, int value);
private:
    std::string m_path;
    std::vector<std::map<char, int> > m_controls;
    std::shared_ptr<boost::asio::io_service> m_io_service;
    std::shared_ptr<boost::asio::deadline_timer> m_timer;

    static std::shared_ptr<piga_host> m_globalHost;
    thread_local static std::shared_ptr<piga_event> m_cacheEvent;

    // Mapped functions
    typedef int (*GetPigaMajorVersion)();
    typedef int (*GetPigaMinorVersion)();
    typedef int (*GetPigaMiniVersion)();
    typedef int (*Init)(int);
    typedef float (*UseUpdateFunction)();
    typedef void (*Update)();
    typedef void (*Destroy)();
    typedef int (*GetButtonState)(int, int);
    typedef const char*(*GetString)(void);
    typedef void (*SetInputCallback)(InputCallbackFunctionType);

    typedef int (*ImplementsOutputs)();
    typedef int (*GetOutputCount)();
    typedef int (*GetOutputType)(int);
    typedef int (*GetOutputPos)(int);
    typedef const char* (*GetOutputName)(int);
    typedef const char* (*GetOutputDescription)(int);
    typedef int (*GetIntOutputRangeMin)(int);
    typedef int (*GetIntOutputRangeMax)(int);
    typedef float (*GetFloatOutputRangeMin)(int);
    typedef float (*GetFloatOutputRangeMax)(int);
    typedef double (*GetDoubleOutputRangeMin)(int);
    typedef double (*GetDoubleOutputRangeMax)(int);
    typedef int (*SetColorOutput)(int, int, int, int, int);
    typedef int (*SetBoolOutput)(int, int);
    typedef int (*SetStringOutput)(int, const char*);
    typedef int (*SetIntOutput)(int, int);
    typedef int (*SetFloatOutput)(int, float);
    typedef int (*SetDoubleOutput)(int, double);

    void *m_getPigaMajorVersion = nullptr;
    void *m_getPigaMinorVersion = nullptr;
    void *m_getPigaMiniVersion = nullptr;
    void *m_init = nullptr;
    void *m_useUpdateFunction = nullptr;
    void *m_update = nullptr;
    void *m_destroy = nullptr;
    void *m_getButtonState = nullptr;
    void *m_setInputCallback = nullptr;

    void *m_getName = nullptr;
    void *m_getDescription = nullptr;
    void *m_getAuthor = nullptr;

    void *m_implementsOutputs = nullptr;
    void *m_getOutputCount = nullptr;
    void *m_getOutputType = nullptr;
    void *m_getOutputPos = nullptr;
    void *m_getOutputName = nullptr;
    void *m_getOutputDescription = nullptr;
    void *m_getIntOutputRangeMin = nullptr;
    void *m_getIntOutputRangeMax = nullptr;
    void *m_getFloatOutputRangeMin = nullptr;
    void *m_getFloatOutputRangeMax = nullptr;
    void *m_getDoubleOutputRangeMin = nullptr;
    void *m_getDoubleOutputRangeMax = nullptr;
    void *m_setColorOutput = nullptr;
    void *m_setBoolOutput = nullptr;
    void *m_setStringOutput = nullptr;
    void *m_setIntOutput = nullptr;
    void *m_setFloatOutput = nullptr;
    void *m_setDoubleOutput = nullptr;

    void *m_dlHandle = nullptr;

    Type m_type = Undefined;
};
}
}

#endif
