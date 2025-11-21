#include "../../includes/Headers.hpp"
#include "../cgi/CgiHandler.hpp"
#include "Response.hpp"
#include "../config/Config.hpp"
#include "Request.hpp"

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

static std::string getExtension(const std::string &path)
{
    size_t pos = path.rfind('.');
    if (pos == std::string::npos)
        return "";
    return path.substr(pos);
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
        return false;

    pos += sep.size() + 2; // skip boundary + CRLF

    // encontrar headers da parte
    size_t header_end = body.find("\r\n\r\n", pos);
    if (header_end == std::string::npos)
        return false;

    std::string header = body.substr(pos, header_end - pos);

    // encontrar filename
    size_t fn_pos = header.find("filename=\"");
    if (fn_pos == std::string::npos)
        return false;

    fn_pos += 10;
    size_t fn_end = header.find("\"", fn_pos);
    filename = header.substr(fn_pos, fn_end - fn_pos);

    // corpo da parte
    size_t data_start = header_end + 4;
    size_t data_end = body.find(sep, data_start);
    if (data_end == std::string::npos)
        return false;

    filedata = body.substr(data_start, data_end - data_start - 2);

    return true;
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
bool    cgiDetect(const Request &req,
                                const ServerConfig &config, Response &res)
{
    std::string ext = getExtension(req.uri);

    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        const LocationConfig &loc = config.locations[i];

        // 1. o location precisa ter CGI habilitado
        if (loc.cgi_extension.empty())
            continue ;
        else
            std::cout << "\n1 - O location tem CGI habilitado" << std::endl;

        // 2. a URI precisa começar com o path do location
        if (req.uri.find(loc.path) != 0)
            continue ;
        else
            std::cout << "\n2 - A URI começa com o path do location" << std::endl;
        // 3. extensão tem que bater
        if (ext != loc.cgi_extension)
            continue ;
        else
            std::cout << "\n3 - A extensão bate" << std::endl;

        // 4. caminho completo do script deve existir
        std::string realPath = loc.root + "/" + req.uri.substr(loc.path.size());
        if (!fileExists(realPath))
            continue ;
        else
        {
            std::cout << "\n4 - O caminho completo do script existe" << std::endl;
            std::cout << "\nRota do CGI detectada: " << realPath << std::endl;
            res = CgiHandler::handleCgiRequest(req, config, loc);
            return (true);
        }
    }
    return (false);
}

Response   methodGet(const ServerConfig &config,
                                const std::string &path)
{
    Response    res;

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
    // not found
    res = notFoundResponse(config);
    return (res);
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
        res.body = "<h1>400 Bad Request (path is a directory)</h1>";
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
        res.body = "<h1>404 Not Found (directory does not exist)</h1>";
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
        res.body = "<h1>500 Internal Server Error (cannot write file)</h1>";
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
    res.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
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
        res.body = "<h1>400 Bad Request (no boundary)</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    if (!extractMultipartFile(req.body, boundary, filename, filedata))
    {
        res.status = 400;
        res.body = "<h1>400 Bad Request (cannot parse multipart)</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    // salvar ficheiro
    std::string fullpath = uploadDir + "/" + filename;

    std::ofstream out(fullpath.c_str(), std::ios::binary);
    out.write(filedata.c_str(), filedata.size());
    out.close();

    res.status = 201;
    res.body = "<h1>Upload ok!</h1>";
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return res;
}
