#include <piga/daemon/LogManager.hpp>
#include <boost/core/null_deleter.hpp>
#include <fstream>
#include <iostream>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>

#include <iomanip>

namespace bl = boost::log;
namespace expr = boost::log::expressions;

namespace piga
{
namespace daemon 
{
sdk::LogManager* sdk::LogManager::m_instance = nullptr;
    
LogManager::LogManager()
{
    
}
LogManager::~LogManager()
{
}

std::ostream& operator<< (std::ostream& strm, LogLevel lvl)
{
    const char* str = LogLevelNames[lvl];
    if (lvl < __L_NUM && lvl >= 0)
        strm << str;
    else
        strm << static_cast< int >(lvl);
    return strm;
}

void LogManager::init()
{
    boost::log::register_simple_formatter_factory< LogLevel, char >("Severity");
    m_instance = this;
    boost::shared_ptr< bl::core > core = bl::core::get();
    core->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
    
    std::string loggingPath = "./";
    
    if(getuid() == 0) {
        loggingPath = "/var/log/";
    }
    boost::shared_ptr< bl::sinks::text_file_backend > fileBackend =
    boost::make_shared< bl::sinks::text_file_backend >(
        bl::keywords::file_name = loggingPath + "pigadaemon_%5N.log",
        bl::keywords::rotation_size = 5 * 1024 * 1024,
        bl::keywords::time_based_rotation = bl::sinks::file::rotation_at_time_point(12, 0, 0)
    );
    
    typedef bl::sinks::synchronous_sink< bl::sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > fileSink(new sink_t(fileBackend));
    
    fileSink->set_formatter
    (
        expr::stream
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
            << " <" << expr::attr<LogLevel>("Severity") << ">"
            << " [" << expr::attr< std::string >("Channel") << "]: "
            << expr::smessage
    );
    
    core->add_sink(fileSink);
    
    boost::shared_ptr< bl::sinks::text_ostream_backend > clogBackend=
    boost::make_shared< bl::sinks::text_ostream_backend >();
    clogBackend->add_stream(
        boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
    clogBackend->add_stream(
        boost::shared_ptr< std::ostream >(new std::ostream(&m_outBuffer), boost::null_deleter()));
    
    clogBackend->auto_flush(true);
    typedef bl::sinks::synchronous_sink< bl::sinks::text_ostream_backend > clogSink_t;
    boost::shared_ptr< clogSink_t > clogSink(new clogSink_t(clogBackend));
    
    clogSink->set_formatter
    (
        expr::stream
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
            << " <" << expr::attr<LogLevel>("Severity") << ">"
            << " [" << expr::attr< std::string >("Channel") << "]: "
            << expr::smessage
    );
    
    
    core->add_sink(clogSink);
}

}
}
