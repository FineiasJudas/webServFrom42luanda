#include "../../includes/Headers.hpp"
#include "../cgi/CgiHandler.hpp"
#include "Response.hpp"
#include "../config/Config.hpp"
#include "Request.hpp"
#include <dirent.h>

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

static bool getAutoIndex(const ServerConfig &server,
                          const LocationConfig &loc)
{
    if (loc.auto_index_set)
        return loc.auto_index;

    if (server.auto_index_set)
        return server.auto_index;

    return false; // default nginx
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

    size_t  dot = path.find_last_of('.');

    if (dot == std::string::npos)
        return ("application/octet-stream");

    std::string ext = path.substr(dot);
    std::map<std::string,std::string>::iterator it = mime.find(ext);

    if (it != mime.end())
        return (it->second);
    return ("application/octet-stream");
}

std::string getBoundary(const std::string &contentType)
{
    std::string key = "boundary=";
    size_t pos = contentType.find(key);

    if (pos == std::string::npos)
        return "";
    return contentType.substr(pos + key.size());
}

bool    extractMultipartFile(const std::string &body,
                          const std::string &boundary,
                          std::string &filename,
                          std::string &filedata)
{
    std::string sep = "--" + boundary;
    size_t pos = body.find(sep);
    if (pos == std::string::npos)
        return (false);

    pos += sep.size() + 2; // skip boundary + CRLF

    // encontrar headers da parte
    size_t header_end = body.find("\r\n\r\n", pos);
    if (header_end == std::string::npos)
        return (false);

    std::string header = body.substr(pos, header_end - pos);

    // encontrar filename
    size_t fn_pos = header.find("filename=\"");
    if (fn_pos == std::string::npos)
        return (false);

    fn_pos += 10;
    size_t fn_end = header.find("\"", fn_pos);
    filename = header.substr(fn_pos, fn_end - fn_pos);

    // corpo da parte
    size_t data_start = header_end + 4;
    size_t data_end = body.find(sep, data_start);
    if (data_end == std::string::npos)
        return (false);

    filedata = body.substr(data_start, data_end - data_start - 2);

    return (true);
}

static Response notFoundResponse(const ServerConfig &config)
{
    Response    res;
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
    return res;
}

Response forbiddenPageResponse(const ServerConfig &config)
{
    Response    res;
    std::string errorPath;

    if (config.error_pages.count(403))
        errorPath = config.error_pages.find(403)->second;
    if (!errorPath.empty())
    {
        std::string content = readFile(errorPath);
        if (!content.empty())
        {
            res.body = content;
            res.status = 403;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            return (res);
        }
    }
    return res;
}

static Response    generateDirectoryListing(const std::string &uri,
                                          const std::string &path)
{
    Response res;

    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        res.status = 500;
        res.body = "<h1>500 Internal Server Error</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    std::stringstream ss;
    ss << "<html><body>";
    ss << "<h1>Index of " << uri << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue ;

        ss << "<li><a href=\"" << uri;
        if (uri[uri.size()-1] != '/') ss << "/";
        ss << name << "\">" << name << "</a></li>";
    }

    ss << "</ul></body></html>";
    closedir(dir);

    res.status = 200;
    res.body = ss.str();
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return res;
}

Response methodGet(const ServerConfig &config,
                           const LocationConfig &loc,
                           const std::string &path,
                           const std::string &uri)
{
    Response    res;

    // 1. Se existe um arquivo exatamente no path → retornar arquivo
    if (fileExists(path))
    {
        res.body = readFile(path);
        res.status = 200;
        res.headers["Content-Type"] = getMimeType(path);
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    // 2. Se é diretório
    if (dirExists(path))
    {
        std::string index = path + "/index.html";

        // 2.a. Se tem index.html → retornar index.html
        if (fileExists(index))
        {
            res.body = readFile(index);
            res.status = 200;
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            return res;
        }

        // 2.b. Se auto_index está on → gerar listagem
        bool autoIndex = getAutoIndex(config, loc);

        if (autoIndex)
            return generateDirectoryListing(uri, path);


        // 2.c. Caso contrário → Forbidden
        return forbiddenPageResponse(config);
    }

    // 3. Caso contrário → Not found
    return notFoundResponse(config);
}

Response methodPost(const Request &req,
                    const ServerConfig &config,
                    const std::string &path)
{
    (void)config;

    Response res;

    // Não pode ser um diretório
    if (path[path.size() - 1] == '/')
    {
        res.status = 400;
        res.body = "<h1>400 Bad Request (path is a directory)</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    // Verificar se diretório existe
    size_t pos = path.find_last_of('/');
    std::string dir = (pos != std::string::npos) ? path.substr(0, pos) : ".";
    if (!dirExists(dir))
    {
        res.status = 404;
        res.body = "<h1>404 Not Found (directory does not exist)</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    bool existed = fileExists(path);

    // Abrir ficheiro para escrita (binário)
    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out)
    {
        res.status = 500;
        res.body = "<h1>500 Internal Server Error (cannot write file)</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    // Escrever todo o body (binário ou texto)
    out.write(req.body.data(), req.body.size());
    out.close();

    if (existed)
        res.status = 200;       // Updated
    else
        res.status = 201;       // Created

    res.body = "";
    res.headers["Content-Length"] = "0";
    return res;
}

Response   methodDelete(const std::string &path, const ServerConfig &config)
{
    Response res;

    if (fileExists(path))
    {
        remove(path.c_str());
        res.status = 204;
        res.body = "";
        res.headers["Content-Length"] = "0";
        return (res);
    }
    res = notFoundResponse(config);
    return (res);
}

Response notAloweMethodResponse(const ServerConfig &config)
{
    Response res;

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
    res.body = "<html><body><h1>405 Method Not Allowed</h1></body></html><a href=\"/\">Voltar</a>";
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return (res);
}

Response methodPostMultipart(const Request &req,
                             const std::string &uploadDir)
{
    Response res;
    std::string filename, filedata;

    std::string boundary = getBoundary(req.headers.at("Content-Type"));
    if (boundary.empty())
    {
        res.status = 400;
        res.body = "<h1>400 Bad Request (no boundary)</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    if (!extractMultipartFile(req.body, boundary, filename, filedata))
    {
        res.status = 400;
        res.body = "<h1>400 Bad Request (cannot parse multipart)</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    // salvar ficheiro
    std::string fullpath = uploadDir + "/" + filename;

    std::ofstream out(fullpath.c_str(), std::ios::binary);
    out.write(filedata.c_str(), filedata.size());
    out.close();

    std::string content = readFile("./examples/www/upload/sucessUpload.html");
    res.status = 201;
    res.body = content;
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return res;
}
