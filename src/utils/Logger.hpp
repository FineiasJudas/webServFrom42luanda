#pragma once
#include "../../includes/Headers.hpp"
#include <string>
#include <iostream>
#include <ctime>

class   Logger
{
    public:
        enum Level {
            INFO,
            DEBUG,
            ERROR,
            CONNECTION,
            CGI
        };

        static void log(Level level, const std::string &msg) {
            std::string prefix = timestamp() + " ";

            switch (level) {
                case INFO:       prefix += "[INFO] "; break;
                case DEBUG:      prefix += "[DEBUG] "; break;
                case ERROR:      prefix += "[ERROR] "; break;
                case CONNECTION: prefix += "[CONN] "; break;
                case CGI:        prefix += "[CGI] "; break;
            }

            std::cout << prefix << msg << std::endl;
        }

    private:
        static std::string timestamp() {
            time_t now = time(NULL);
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            return buf;
        }

};
