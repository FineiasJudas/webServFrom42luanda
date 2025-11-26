#include "Router.hpp"
#include "../http/Response.hpp"
#include "../config/Config.hpp"
#include "../utils/Utils.hpp"
#include "../cgi/CgiHandler.hpp"
#include <sstream>

Response    Router::route(const Request &req, const ServerConfig &config)
{
    Response    res;

	 // CGI?
        if (cgiDetect(req, config, res))
            return (res);

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

    std::cout << "\nRota detectada: " << path << std::endl;

    if (req.method == "GET")
        return methodGet(config, path);
    else if (req.method == "POST")
    {
       
        // POST normal (Opção C)
        // multipart?
        if (req.headers.count("Content-Type") &&
            req.headers.at("Content-Type").find("multipart/form-data") != std::string::npos)
        {
            std::string uploadDir = config.locations[0].root; 
            return methodPostMultipart(req, uploadDir);
        }
        return methodPost(req, config, path);
    }
    else if (req.method == "DELETE")
        return methodDelete(path, config);      
    else
        return notAloweMethodResponse(config);
}
