#include "Router.hpp"
#include "../utils/Utils.hpp"
#include <sstream>

static bool fileExists(const std::string &path)
{
    struct stat s;

    return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}
static bool dirExists(const std::string &path)
{
    struct stat s;

    return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}
static std::string  readFile(const std::string &path)
{
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open())
        return (std::string());

    std::ostringstream  ss;
    ss << f.rdbuf();
    return (ss.str());
}
static std::string  getMimeType(const std::string &path)
{
    static bool inited = false;
    static std::map<std::string,std::string> mime;

    if (!inited)
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
        inited = true;
    }

    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return ("application/octet-stream");

    std::string ext = path.substr(dot);
    std::map<std::string,std::string>::iterator it = mime.find(ext);

    if (it != mime.end())
        return (it->second);
    return ("application/octet-stream");
}

Response    Router::route(const Request &req, const ServerConfig &config)
{
    Response    res;

    // determine root from config (use first location root if set)
    std::string root = "./examples/www";
    if (!config.locations.empty() && !config.locations[0].root.empty())
        root = config.locations[0].root;
    std::string uri = req.uri;
    if (uri.empty())
        uri = "/";

    // prevent URI with .. (basic safety)
    if (uri.find("..") != std::string::npos)
    {
        res.status = 403;
        res.body = "<html><body><h1>403 Forbidden</h1></body></html>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return (res);
    }

    std::string path = root;
    // ensure root doesn't end with '/' duplicate

    if (!root.empty() && root[root.size()-1] == '/' && uri.size() > 0 && uri[0] == '/')
        path += uri.substr(1);
    else
        path += uri;

    if (req.method == "GET")
    {
        if (fileExists(path))
        {
            res.body = readFile(path);
            res.status = 200;
            res.headers["Content-Type"] = getMimeType(path);
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            return (res);
        }
        // directory with index.html
        if (dirExists(path) && fileExists(path + "/index.html"))
        {
            res.body = readFile(path + "/index.html");
            res.status = 200;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            return (res);
        }
        // not found -> check config.error_pages
        std::string errorPath;
        if (config.error_pages.count(404))
            errorPath = config.error_pages.find(404)->second;
        if (!errorPath.empty())
        {
            std::string content = readFile(errorPath);
            if (!content.empty())
            {
                res.body = content;
                res.status = 404;
                res.headers["Content-Type"] = "text/html";
                res.headers["Content-Length"] = Utils::toString(res.body.size());
                return (res);
            }
        }
        // fallback
        res.status = 404;
        res.body = "<html><body><h1>404 Not Found</h1></body></html>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return (res);
    }
    else if (req.method == "DELETE")
    {
        if (fileExists(path))
        {
            remove(path.c_str());
            res.status = 204;
            res.body = "";
            res.headers["Content-Length"] = "0";
            return (res);
        }
        else
        {
            res.status = 404;
            res.body = "<html><body><h1>404 Not Found</h1></body></html>";
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            return (res);
        }
    }
    else
    {
        // 405
        std::string errorPath;
        if (config.error_pages.count(405))
            errorPath = config.error_pages.find(405)->second;
        if (!errorPath.empty())
        {
            std::string content = readFile(errorPath);
            if (!content.empty())
            {
                res.body = content;
                res.status = 405;
                res.headers["Content-Type"] = "text/html";
                res.headers["Content-Length"] = Utils::toString(res.body.size());
                return (res);
            }
        }
        res.status = 405;
        res.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return (res);
    }
}