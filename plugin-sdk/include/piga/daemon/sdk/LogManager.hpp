#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>

#include <boost/signals2/signal.hpp>

namespace piga 
{
namespace daemon 
{
namespace sdk
{
class LogManagerCachedLogsBuffer : public std::stringbuf
{
public: 
    typedef boost::signals2::signal<void(const std::string&)> BufferFlushSignal;
    virtual int sync() {
        std::lock_guard<std::mutex> bufferGuard(m_bufferMutex);
       
        bufferFlush(this->str());
        
        return 0;
    }
    BufferFlushSignal bufferFlush;
private:
    std::mutex m_bufferMutex;
};
    
class LogManager
{
public:
    LogManagerCachedLogsBuffer& getOutBuffer() {return m_outBuffer;}
    
    static LogManager* get() {return m_instance;}
protected:
    LogManagerCachedLogsBuffer m_outBuffer;
    static LogManager *m_instance;
};
}
}
}
