#include "Router.hpp"
#include "../utils/Utils.hpp"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <map>
#include <sstream>

// ------------------------------------------------------------
// Funções utilitárias internas
// ------------------------------------------------------------

static bool fileExists(const std::string &path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}

static bool dirExists(const std::string &path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}

// Dicionário de tipos MIME comuns
static std::string getMimeType(const std::string &path)
{
    static bool initialized = false;
    static std::map<std::string, std::string> mime;

    if (!initialized)
    {
        mime[".html"] = "text/html";
        mime[".htm"]  = "text/html";
        mime[".css"]  = "text/css";
        mime[".js"]   = "application/javascript";
        mime[".png"]  = "image/png";
        mime[".jpg"]  = "image/jpeg";
        mime[".jpeg"] = "image/jpeg";
        mime[".gif"]  = "image/gif";
        mime[".ico"]  = "image/x-icon";
        mime[".txt"]  = "text/plain";
        mime[".json"] = "application/json";
        initialized = true;
    }

    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dot);

    std::map<std::string, std::string>::iterator it = mime.find(ext);
    if (it != mime.end())
        return it->second;
    return "application/octet-stream";
}


// Lê o conteúdo de um arquivo
static std::string readFile(const std::string &path) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ------------------------------------------------------------
// Função principal: Router::route()
// ------------------------------------------------------------
Response Router::route(const Request &req, const ServerConfig &config) {
    Response res;

    // Por enquanto, usamos a primeira location como root principal
    std::string root = "./examples/www";
    if (!config.locations.empty() && !config.locations[0].root.empty())
        root = config.locations[0].root;

    std::string path = root + req.uri;

    if (req.method == "GET") {
        if (fileExists(path)) {
            // Serve arquivo normal
            res.body = readFile(path);
            res.status = 200;
            res.headers["Content-Type"] = getMimeType(path);
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
        else if (dirExists(path) && fileExists(path + "/index.html")) {
            // Serve diretório com index.html
            res.body = readFile(path + "/index.html");
            res.status = 200;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
        else {
            // Serve página de erro configurada (ou padrão)
            int code = 404;
            std::string errorPath;

            if (config.error_pages.count(code))
                errorPath = config.error_pages.find(code)->second;
            else
                errorPath = "./examples/www/errors/NotFound.html";

            res.body = readFile(errorPath);
            if (res.body.empty())
                res.body = "<html><body><h1>404 Not Found</h1></body></html>";

            res.status = code;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
    }
    else if (req.method == "DELETE") {
        if (fileExists(path)) {
            std::remove(path.c_str());
            res.status = 204;
            res.body = "";
        } else {
            res.status = 404;
            res.body = "<html><body><h1>404 Not Found</h1></body></html>";
        }
        res.headers["Content-Length"] = Utils::toString(res.body.size());
    }
    else {
        int code = 405;
        std::string errorPath;

        if (config.error_pages.count(code))
            errorPath = config.error_pages.find(code)->second;
        else
            errorPath = "./examples/www/errors/MethodNotAllowed.html";

        res.body = readFile(errorPath);
        if (res.body.empty())
            res.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";

        res.status = code;
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
    }

    return res;
}
