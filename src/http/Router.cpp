#include <sstream>
#include <dirent.h>
#include "Router.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Logger.hpp"
#include "../config/Config.hpp"
#include "../http/Response.hpp"
#include "../cgi/CgiHandler.hpp"
#include "../session/SessionManager.hpp"

/*
    * Extrai a extensão do nome do ficheiro
*/
static std::string getExtension(const std::string &path)
{
    size_t pos = path.rfind('.');
    if (pos == std::string::npos){return "";}
    return path.substr(pos);
}

/*
    * Verifica se o URI corresponde ao caminho da localização
*/
bool matchLocation(const std::string &uri, const std::string &locPath)
{
    if (uri.find(locPath) != 0){return (false);}
    // location termina com /
    if (locPath[locPath.size() - 1] == '/'){return (true);}
    if (uri.size() == locPath.size()){return (true);}
    if (uri[locPath.size()] == '/'){return (true);}
    return (false);
}

/*
    * Encontra a melhor localização correspondente ao URI
*/
const LocationConfig &findBestLocation(const std::string &uri,
                                       const ServerConfig &config)
{
    const LocationConfig *best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const LocationConfig &loc = config.locations[i];

        if (loc.path.empty() || loc.path[0] != '/')
            continue;

        if (matchLocation(uri, loc.path))
        {
            if (loc.path.size() > bestLen)
            {
                best = &loc;
                bestLen = loc.path.size();
            }
        }
    }

    if (!best){return config.locations[0];} // "/"
    return (*best);
}


/*
    * Constrói o path real no sistema de ficheiros
*/

static std::string makeRealPath(const std::string &uri,
                                const LocationConfig &loc,
                                const ServerConfig &srv)
{
    std::string root = loc.root.empty() ? srv.root : loc.root;
    std::string suffix;

    if (uri.size() >= loc.path.size()){suffix = uri.substr(loc.path.size());}

    if (!root.empty() && root[root.size() - 1] == '/' &&
        !suffix.empty() && suffix[0] == '/'){return root + suffix.substr(1);}

    if (!root.empty() && root[root.size() - 1] != '/' &&
        !suffix.empty() && suffix[0] != '/'){return root + "/" + suffix;}

    return (root + suffix);
}

/*
    * Roteador principal
*/

Response Router::route(const Request &req, const ServerConfig &config)
{
    Response r;
    Request rq = req;

    /* Rotas internas */
    if (handleCsrf(rq, r)){return r;}
    if (handleLogin(rq, r)){return r;}
    if (handleLogout(rq, r)){return r;}
    if (handleSession(rq, r)){return r;}

    /* Encontrar location */
    const LocationConfig &loc = findBestLocation(rq.uri, config);
    Logger::log(Logger::INFO, "Location escolhida: " + loc.path);

    /* Redirect */
    if (loc.redirect_code)
    {
        r.status = loc.redirect_code;
        r.headers["Location"] = loc.redirect_url;
        r.body = "<h1>Redirect</h1>";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        r.headers["Content-Type"] = "text/html";
        return r;
    }

    /* Segurança */
    if (rq.uri.find("..") != std::string::npos){return forbiddenPageResponse(config);}

    /* Resolver FS path */
    std::string fsPath = makeRealPath(rq.path, loc, config);
    Logger::log(Logger::INFO, "FS PATH: " + fsPath);

    /* Métodos HTTP */
    std::string busca = req.method;
    bool metodo_req_existe = std::find(loc.methods.begin(), loc.methods.end(), busca) != loc.methods.end();

    if (metodo_req_existe)
    {
        /*
        * Não usar GET direitamente aqui(Garantir que os metodos sao validos no parse),
        * verificar a lista de métodos permitidos na location
         */

        std::string ext = getExtension(rq.path);
        Logger::log(Logger::INFO, "::::::::::::::::METODO: " + req.method);
        for (size_t i = 0; i < loc.methods.size(); i++){Logger::log(Logger::INFO, "::::::::::::::::CGI EXT: " + loc.methods[i]);}

        Logger::log(Logger::INFO, " ::: CGI EXTENCION: " + ext);
        // CGI
        for (size_t i = 0; i < loc.cgi.size(); i++)
        {
            if (loc.cgi[i].extension == ext)
            {
                Logger::log(Logger::INFO, " ::: CGI PARA EXTENCAO: " + ext);
                return (CgiHandler::handleCgiRequest(rq, config, loc, loc.cgi[i]));
            }
        }
        // GET
        if (rq.method == "GET"){return methodGet(config, loc, fsPath, rq.uri);}
    }

    return notAloweMethodResponse(config);
}