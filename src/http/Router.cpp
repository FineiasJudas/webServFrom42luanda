#include "Router.hpp"
#include "../utils/Logger.hpp"
#include "../http/Response.hpp"
#include "../config/Config.hpp"
#include "../utils/Utils.hpp"
#include <dirent.h>
#include "../cgi/CgiHandler.hpp"
#include <sstream>

static std::string  getExtension(const std::string &path)
{
    size_t pos = path.rfind('.');

    if (pos == std::string::npos)
        return "";
    return path.substr(pos);
}

const LocationConfig    &findBestLocation(const std::string& uri,
                                       const ServerConfig& config)
{
    const LocationConfig    *best = NULL;
    size_t  bestLen = 0;

    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const LocationConfig    &loc = config.locations[i];

        // sanity check
        if (loc.path.empty() || loc.path[0] != '/')
            continue ;

        // prefix match
        //std::cout << "\nEncontar " << uri << " no PATH " << loc.path << std::endl;
        if (uri.find(loc.path) == 0)
        {
            if (loc.path.size() > bestLen)
            {
                best = &loc;
                bestLen = loc.path.size();
            }
        }
    }

    // fallback: usually "/"
    if (!best)
        // guaranteed by config parser
        return config.locations[0];

    return (*best);
}

void    parseUri(Request &req)
{
    size_t pos = req.uri.find('?');

    if (pos == std::string::npos)
    {
        req.path = req.uri;
        return;
    }

    req.path = req.uri.substr(0, pos);

    std::string qs = req.uri.substr(pos + 1);
    std::stringstream   ss(qs);
    std::string pair;

    while (std::getline(ss, pair, '&'))
    {
        size_t  eq = pair.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = pair.substr(0, eq);
        std::string val = pair.substr(eq + 1);

        req.query[key] = val;
    }
}


Response    handleUploadsList(const Request &req)
{
    (void)req;

    Response res;
    std::string folder = "./examples/www/uploads";

    DIR *dir = opendir(folder.c_str());
    if (!dir)
    {
        res.status = 500;
        res.body = "{\"error\": \"cannot open uploads folder\"}";
        res.headers["Content-Type"] = "application/json";
        return res;
    }

    std::stringstream json;
    json << "{ \"files\": [";
    bool first = true;

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] == '.')
            continue ;

        if (!first) json << ",";
        first = false;

        json << "\"" << entry->d_name << "\"";
    }

    json << "] }";
    closedir(dir);

    res.status = 200;
    res.body = json.str();
    res.headers["Content-Type"] = "application/json";
    return res;
}

Response    handleDeleteFile(const Request &req)
{
    Response    res;

    if (req.query.count("name") == 0)
    {
        res.status = 400;
        res.body = "{\"error\": \"missing file name\"}";
        res.headers["Content-Type"] = "application/json";
        return res;
    }

    std::string name = req.query.find("name")->second;

    std::cout << "Delete file: " << name << std::endl;

    if (name.empty())
    {
        res.status = 400;
        res.body = "{\"error\": \"missing file name\"}";
        res.headers["Content-Type"] = "application/json";
        return res;
    }

    std::string fullpath = "./examples/www/uploads/" + name;

    if (unlink(fullpath.c_str()) != 0)
    {
        res.status = 404;
        res.body = "{\"error\": \"file not found\"}";
    }
    else
    {
        res.status = 200;
        res.body = "{\"message\": \"deleted\"}";
    }

    res.headers["Content-Type"] = "application/json";
    return res;
}


Response    Router::route(const Request &req, const ServerConfig &config)
{
    Request tmp = req;

    // 1) Encontrar location
    const LocationConfig    &loc = findBestLocation(req.uri, config);
    Logger::log(Logger::INFO, "Rota encontrada: " + loc.path);

    // 2) Bloquear directory traversal
    if (req.uri.find("..") != std::string::npos)
        return forbiddenPageResponse(config);

    // 3) ENDPOINT ESPECIAL → /uploads-list (GET)
    if (req.uri == "/uploads-list" && req.method == "GET")
        return handleUploadsList(req);

    // 4) ENDPOINT ESPECIAL → /delete-file (DELETE)
    parseUri(tmp);
    if (tmp.path == "/delete-file" && tmp.method == "DELETE")
        return handleDeleteFile(tmp);

    // 5) CGI
    std::string ext = getExtension(req.uri);
    if (!loc.cgi_extension.empty() && ext == loc.cgi_extension)
        return CgiHandler::handleCgiRequest(req, config, loc);

    // 6) Resolver caminho real do arquivo
    std::string root = loc.root.empty() ? config.root : loc.root;
    std::string path = root;

    if (path[path.size() - 1] == '/' && req.uri[0] == '/')
        path += req.uri.substr(1);
    else
        path += req.uri;

    // 7) GET
    if (req.method == "GET")
        return methodGet(config, loc, path, req.uri);

    // 8) POST
    if (req.method == "POST")
    {
        if (!loc.upload_dir.empty())
            return methodPostMultipart(req, loc.upload_dir);

        return methodPost(req, config, path);
    }

    // 9) DELETE em arquivos normais
    if (req.method == "DELETE")
        return methodDelete(path, config);

    // 10) Método não permitido
    return notAloweMethodResponse(config);
}
