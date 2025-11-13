#include "Router.hpp"
#include "../utils/Utils.hpp"
#include <fstream>
#include <sys/stat.h>

static bool fileExists(const std::string &path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}

static bool dirExists(const std::string &path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}

Response Router::route(const Request &req, const ServerConfig &config)
{
    (void)config;
    Response res;
    std::string root = "./examples/www"; // por enquanto hardcoded
    std::string path = root + req.uri;

    if (req.method == "GET")
    {
        if (fileExists(path))
        {
            std::ifstream file(path.c_str());
            std::stringstream ss;
            ss << file.rdbuf();
            res.body = ss.str();
            res.status = 200;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
        else if (dirExists(path) && fileExists(path + "/index.html"))
        {
            std::ifstream file((path + "/index.html").c_str());
            std::stringstream ss;
            ss << file.rdbuf();
            res.body = ss.str();
            res.status = 200;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
        else
        {
            res.status = 404;
            res.body = "<html><body><h1>404 Not Found</h1></body></html>";
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
        }
    }
    else if (req.method == "DELETE")
    {
        if (fileExists(path))
        {
            std::remove(path.c_str());
            res.status = 204; // No Content
            res.body = "";
        }
        else
        {
            res.status = 404;
            res.body = "<html><body><h1>404 Not Found</h1></body></html>";
        }
        res.headers["Content-Length"] = Utils::toString(res.body.size());
    }
    else
    {
        res.status = 405;
        res.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
    }

    return (res);
}


