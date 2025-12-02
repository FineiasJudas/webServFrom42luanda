
#include "Logger.hpp"
#include <iostream>
#include <sstream>
#include <ctime>

Logger::LogLevel Logger::currentLevel = Logger::INFO;
std::ofstream Logger::logFile;

void Logger::init(LogLevel level, const std::string &filename)
{
    currentLevel = level;
    if (!filename.empty())
    {
        // abre em append
        logFile.open(filename.c_str(), std::ios::app);
        if (!logFile.is_open())
        {
            // se falhar em abrir ficheiro, informa no stdout
            std::cerr << "Logger: não foi possível abrir o ficheiro de logs: " << filename << std::endl;
        }
    }
}

void Logger::shutdown()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

std::string Logger::levelToString(LogLevel level)
{
    switch (level)
    {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO ";
        case WARN:  return "WARN ";
        case ERROR: return "ERROR";
        case NEW:   return "NEW  ";
        case WINT:   return "WINT ";
    }
    return "UNKN ";
}

std::string Logger::timestamp()
{
    std::time_t t = std::time(NULL);
    char buf[64];
    std::tm *tm_info = std::localtime(&t);
    if (tm_info)
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    else
        std::snprintf(buf, sizeof(buf), "%lld", (long long)t);
    return std::string(buf);
}

void Logger::log(LogLevel level, const std::string &message)
{
    if (level < currentLevel)
        return ;

    std::ostringstream oss;
    oss << "[" << timestamp() << "] "
        << levelToString(level) << " | "
        << message;

    std::string out = oss.str();

    // escreve no ficheiro se aberto (sem cor)
    if (logFile.is_open())
    {
        logFile << out << std::endl;
        // garantir flush para ver logs em tempo real durante debug
        logFile.flush();
    }

    // escolher cor com base no nível
    const char* color = "";
    const char* reset = "\x1b[0m";
    switch (level)
    {
        case DEBUG: color = "\x1b[32m"; break; // cyan
        case INFO:  color = "\x1b[37m"; break; // green
        case WARN:  color = "\x1b[33m"; break; // yellow
        case ERROR: color = "\x1b[31m"; break; // red
        case NEW:   color = "\x1b[36m"; break; // xxx
        case WINT:   color = "\x1b[35m"; break; // xxx
        default:    color = ""; break;
    }

    // escreve no stdout/stderr com cor
    if (level == ERROR)
        std::cerr << color << out << reset << std::endl;
    else
        std::cout << color << out << reset << std::endl;
}
