#pragma once

#include <string>

#define BOOST_LOG_DYN_LINK 1

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#include <piga/daemon/sdk/LogManager.hpp>

namespace bl = boost::log;

namespace piga
{
namespace daemon
{
enum LogLevel {
    L_DEBUG,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_CRITICAL,
    L_FATAL,
    __L_NUM
}; 
static const char* LogLevelNames[] {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRITICAL",
    "FATAL"
};

typedef ::boost::log::sources::severity_channel_logger_mt<LogLevel, std::string>
    SeverityChannelLogger;
typedef ::boost::log::sources::severity_logger_mt<LogLevel>
    SeverityLogger;

class LogManager : public sdk::LogManager
{
public:
    LogManager();
    ~LogManager();
    
    /**
     * Initializes the current LogManager object to be the global log manager.
     * 
     * This instance has to stay active as long as the program is running!
     */
    void init();
private:
};
}
}
