#include <iostream>
#include <syslog.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int syslog1() {
    int logfd;
    
    time_t t1;
    fprintf(stderr, "log\n");
    logfd = open("log_self", O_RDWR | O_CREAT | O_APPEND, 0644);
    assert(logfd != -1);

    int stder = dup(STDERR_FILENO);
    assert(stder != -1);
    close(STDERR_FILENO);

    dup2(logfd,STDERR_FILENO);
    close(logfd);

    openlog("syslog2", LOG_PERROR, LOG_DAEMON);
    syslog(LOG_INFO, "s\n", ctime(&ti));

    // 恢复标准出错
    dup2(stder, STDERR_FILENO);
    close(stder);
    closelog();
    return 0;
}

int main() {
    syslog1();
    
    return 0;
}