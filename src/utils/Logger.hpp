#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

class   Logger
{
    public:
        enum    LogLevel
        {
            DEBUG = 0,
            INFO  = 1,
            WARN  = 2,
            ERROR = 3,
            NEW   = 4,
            WINT  = 5
        };

        // Inicializa o logger. Se filename vazio -> escreve apenas no stdout.
        static void init(LogLevel level, const std::string &filename = "");

        // Fecha ficheiro de log (se aberto)
        static void shutdown();

        // Função principal de log
        static void log(LogLevel level, const std::string &message);

    private:
        static LogLevel currentLevel;
        static std::ofstream    logFile;

        static std::string  levelToString(LogLevel level);
        static std::string  timestamp();
};

// Macros auxiliares para mostrar file:line (stringizada)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Macros de uso simples:
// exemplo: LOG_INFO("Servidor iniciado");
#define LOG_DEBUG(msg) Logger::log(Logger::DEBUG, std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))
#define LOG_INFO(msg)  Logger::log(Logger::INFO,  std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))
#define LOG_WARN(msg)  Logger::log(Logger::WARN,  std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))
#define LOG_ERROR(msg) Logger::log(Logger::ERROR, std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))
#define LOG_NEW(msg)   Logger::log(Logger::NEW, std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))
#define LOG_WINT(msg)   Logger::log(Logger::WINT, std::string(__FILE__ ":" STR(__LINE__) " ") + (msg))

#endif
