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

//
// ────────────────────────────────────────────────────────────────────────────────
//   LISTAR ARQUIVOS EM /uploads-list
// ────────────────────────────────────────────────────────────────────────────────
//

Response handleUploadsList(const Request &req)
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
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    std::stringstream json;
    json << "{ \"files\": [";

    bool first = true;
    struct dirent *entry;

    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] == '.')
            continue;

        if (!first) json << ",";
        first = false;

        json << "\"" << entry->d_name << "\"";
    }

    json << "] }";
    closedir(dir);

    res.status = 200;
    res.body = json.str();
    res.headers["Content-Type"] = "application/json";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return res;
}

//
// ────────────────────────────────────────────────────────────────────────────────
//   DELETE EM /delete-file?name=
// ────────────────────────────────────────────────────────────────────────────────
//

Response handleDeleteFile(const Request &req)
{
    if (req.query.find("name") == req.query.end())
    {
        Response r;
        r.status = 400;
        r.body = "{\"error\": \"missing ?name=\"}";
        r.headers["Content-Type"] = "application/json";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        return r;
    }

    std::string filename = req.query.at("name");
    std::string fullpath = "./examples/www/uploads/" + filename;

    if (unlink(fullpath.c_str()) != 0)
    {
        Response r;
        r.status = 500;
        r.body = "{\"error\": \"failed to delete\"}";
        r.headers["Content-Type"] = "application/json";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        return r;
    }

    Response r;
    r.status = 200;
    r.body = "{\"status\": \"deleted\"}";
    r.headers["Content-Type"] = "application/json";
    r.headers["Content-Length"] = Utils::toString(r.body.size());
    return r;
}


Response Router::route(const Request &req, const ServerConfig &config)
{
    // ============================================================================
    // 0) ENDPOINTS ESPECIAIS SEM PASSAR POR LOCATION
    // (Eles sempre devem ser processados *antes* do findBestLocation)
    // ============================================================================

    if (req.method == "GET" && req.uri == "/uploads-list")
        return handleUploadsList(req);

    if (req.method == "DELETE" && req.uri.rfind("/delete-file", 0) == 0)
    {
        Request tmp = req;
        parseUri(tmp);
        return handleDeleteFile(tmp);
    }


    // ============================================================================
    // 1) Encontrar location correto
    // ============================================================================

    const LocationConfig &loc = findBestLocation(req.uri, config);
    Logger::log(Logger::INFO, "Rota encontrada: " + loc.path);


    // ============================================================================
    // 1.5) Redirecionamento
    // ============================================================================

    if (loc.redirect_code != 0)
    {
        Response r;

        r.status = loc.redirect_code;
        r.headers["Location"] = loc.redirect_url;

        r.body = "<h1>" + Utils::toString(loc.redirect_code)
                 + " Redirect</h1><p>→ " + loc.redirect_url + "</p>";

        r.headers["Content-Length"] = Utils::toString(r.body.size());
        r.headers["Content-Type"] = "text/html";
        return r;
    }


    // ============================================================================
    // 2) Proteger contra directory traversal
    // ============================================================================

    if (req.uri.find("..") != std::string::npos)
        return forbiddenPageResponse(config);


    // ============================================================================
    // 3) CGI (se extensão combinou)
    // ============================================================================

    std::string ext = getExtension(req.uri);
    if (!loc.cgi_extension.empty() && ext == loc.cgi_extension)
        return CgiHandler::handleCgiRequest(req, config, loc);


    // ============================================================================
    // 4) Resolver caminho real de arquivo
    // ============================================================================

    std::string root = loc.root.empty() ? config.root : loc.root;
    std::string path = root;

    // Corrigir duplo slash
    if (path[path.size() - 1] == '/' && req.uri[0] == '/')
        path += req.uri.substr(1);
    else
        path += req.uri;


    // ============================================================================
    // 5) Métodos HTTP
    // ============================================================================

    // GET
    if (req.method == "GET")
        return methodGet(config, loc, path, req.uri);

    // POST
    if (req.method == "POST")
    {
        if (!loc.upload_dir.empty())
            return methodPostMultipart(req, loc.upload_dir);

        return methodPost(req, config, path);
    }

    // DELETE
    if (req.method == "DELETE")
        return methodDelete(path, config);


    // ============================================================================
    // Método não permitido
    // ============================================================================

    return notAloweMethodResponse(config);
}