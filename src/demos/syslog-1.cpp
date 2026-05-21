#include <syslog.h>
#include <iostream>

using namespace std;

int main(int, const char** ) {

    cout << "Hello Izzy!" << endl;

     // 1. Open a connection to the system logger
    // "MyProgram" is the tag prepended to every message
    // LOG_PID includes the process ID in each entry
    // LOG_CONS It instructs the program to write log messages directly to the 
    //   system console (/dev/console) as a fallback if it fails to send them to 
    //   the main syslog daemon.
    // LOG_USER identifies the facility (type of program)
    openlog("syslog-1", LOG_PID | LOG_CONS, LOG_USER);

    // 2. Log messages at different levels
    syslog(LOG_INFO, "This is an informational message.");
    syslog(LOG_WARNING, "This is a warning: System status check failed.");
    syslog(LOG_ERR, "Critical error: %s occurred", "File not found");

    // 3. Close the connection
    closelog();
}
