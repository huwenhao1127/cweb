#include <log.h>

namespace cweb {
    Logger::Logger(string name, string log_path) 
    : m_name(name), m_log_path(log_path){
        logfd = open(log_path + "/" + m_name, O_RDWR | O_CREAT | O_APPEND, 0644);
        assert(logfd != -1);
        
        openlog(m_name, LOG_PID|LOG_CONS|LOG_NDELAY, 0);
        int ret = setlogmask(m_level);
        assert(ret != -1);
    }

    Logger::~Logger() {
        closelog()
    }

    Logger::addLog(char* message, ...) {
        syslog(LOG_INFO, "%s", "hello world!");
    }

    Logger::setLevel(){

    }
    
    int ret = setlogmask(7);
    assert(ret != -1);
    void closelog();
}

