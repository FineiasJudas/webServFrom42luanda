#include "../../includes/Headers.hpp"
#include "CgiHandler.hpp"

// helper trimming
static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return std::string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// split uri into path and query
static void splitUri(const std::string &uri, std::string &path, std::string &query)
{
    size_t q = uri.find('?');
    if (q == std::string::npos)
    {
        path = uri;
        query.clear();
    }
    else
    {
        path = uri.substr(0, q);
        query = uri.substr(q + 1);
    }
}

/*
Acho que vai ter vários buildEnv's para cada tipo de extenção
*/
void addPhpVars(std::vector<std::string> &env) { env.push_back("REDIRECT_STATUS=200"); }

static std::string normalizeHeaderKey(const std::string &key)
{
    std::string up;
    up.reserve(key.size());
    for (size_t i = 0; i < key.size(); ++i)
    {
        char c = key[i];
        if (c == '-')
            up += '_';
        else
            up += (char)std::toupper(c);
    }
    return up;
}

static void addBasicVars(const Request &req, std::vector<std::string> &env)
{
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=" + req.version);
    env.push_back("REQUEST_METHOD=" + req.method);
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REDIRECT_STATUS=200");
}

static void addScriptVars(const std::string &script_path, std::vector<std::string> &env)
{
    env.push_back("SCRIPT_FILENAME=" + script_path);
}

static void addUriVars(const Request &req, std::vector<std::string> &env)
{
    std::string path, query;
    splitUri(req.uri, path, query);

    env.push_back("REQUEST_URI=" + req.uri);
    env.push_back("QUERY_STRING=" + query);

    if (req.headers.count("Host"))
        env.push_back("SERVER_NAME=" + req.headers.at("Host"));
    else
        env.push_back("SERVER_NAME=localhost");
}

static void addContentVars(const Request &req, std::vector<std::string> &env)
{
    if (req.headers.count("Content-Length"))
        env.push_back("CONTENT_LENGTH=" + req.headers.at("Content-Length"));
    if (req.headers.count("Content-Type"))
        env.push_back("CONTENT_TYPE=" + req.headers.at("Content-Type"));
}

static void addHttpHeaders(const Request &req, std::vector<std::string> &env)
{
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it)
    {
        std::string up = normalizeHeaderKey(it->first);
        env.push_back("HTTP_" + up + "=" + it->second);
    }
}

static std::vector<std::string> buildEnv(
    const Request &req,
    const std::string &script_path,
    const ServerConfig &config,
    const CgiConfig &cgiConfig)
{
    (void)config;
    std::vector<std::string> env;

    addBasicVars(req, env);
    addScriptVars(script_path, env);
    addUriVars(req, env);
    addContentVars(req, env);
    addHttpHeaders(req, env);

    if (cgiConfig.cgi_extension == ".php")
    {
        addPhpVars(env);
    }
    // else if (cgiConfig.cgi_extension == ".py")
    // {
    //     addPythonVars(env);
    // }

    return env;
}

/*
static std::vector<std::string> buildEnv(const Request &req,
                                         const std::string &script_path, const ServerConfig &config)
{
    (void)config;
    std::vector<std::string> env;

    // Basic CGI/1.1 required vars
    env.push_back(std::string("GATEWAY_INTERFACE=CGI/1.1"));
    env.push_back(std::string("SERVER_PROTOCOL=") + req.version);
    env.push_back(std::string("REQUEST_METHOD=") + req.method);

    // SCRIPT_NAME and PATH_INFO: simplest approach - use script path as SCRIPT_NAME
    // user/router should compute script_path as file-system path; we'll set SCRIPT_FILENAME to that.
    env.push_back(std::string("SCRIPT_FILENAME=") + script_path);

    // URI split
    std::string path, query;
    splitUri(req.uri, path, query);
    env.push_back(std::string("REQUEST_URI=") + req.uri);
    env.push_back(std::string("QUERY_STRING=") + query);

    // Host header -> SERVER_NAME
    if (req.headers.count("Host"))
        env.push_back(std::string("SERVER_NAME=") + req.headers.find("Host")->second);
    else
        env.push_back(std::string("SERVER_NAME=localhost"));

    // CONTENT_LENGTH / CONTENT_TYPE from request headers (for POST)
    if (req.headers.count("Content-Length"))
        env.push_back(std::string("CONTENT_LENGTH=") + req.headers.find("Content-Length")->second);
    if (req.headers.count("Content-Type"))
        env.push_back(std::string("CONTENT_TYPE=") + req.headers.find("Content-Type")->second);

    // Standard useful vars
    env.push_back(std::string("SERVER_SOFTWARE=webserv/1.0"));

    // Add HTTP_ headers as CGI expects (HTTP_HEADERNAME)
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it)
    {
        std::string key = it->first;
        std::string val = it->second;
        // Normalize key: make uppercase, replace '-' with '_'
        std::string up;
        for (size_t i = 0; i < key.size(); ++i)
        {
            char c = key[i];
            if (c == '-')
                up += '_';
            else
                up += (char)toupper(c);
        }
        std::string envname = std::string("HTTP_") + up + "=" + val;
        env.push_back(envname);
    }

    // If ServerConfig has error_pages or other info, you can add them here if needed.

    return env;
}
    */

CgiResult CgiHandler::execute(const Request &req,
                              const std::string &script_path,
                              const ServerConfig &config,
                              const LocationConfig &loc,
                              const CgiConfig &cgiConfig)
{
    CgiResult result;
    result.exit_status = -1;
    result.raw_output = "";

    // ------------------ SCRIPT EXISTS? ------------------
    struct stat st;
    if (stat(script_path.c_str(), &st) != 0)
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI script not found: " +
            script_path + "\n";
        return result;
    }

    /*

    criar uma funcao auxi, para isto também

    */

    // ------------------ INTERPRETER ---------------------
    std::string interpreter;
    // for (size_t i = 0; i < config.locations.size(); i++)
    //{
    if (script_path.find(loc.root) == 0 &&
        !cgiConfig.cgi_path.empty())
    {
        interpreter = cgiConfig.cgi_path;
        std::cout << "interpreter: " << interpreter << std::endl;
        //  break;
    }
    //}

    if (interpreter.empty())
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI interpreter not defined\n";
        return result;
    }

    if (access(interpreter.c_str(), X_OK) != 0)
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "Interpreter not executable\n";
        return result;
    }

    /*
    criar BUILD ENV e ARGV  para cada tipo de extencao.
    criar em remover este codigo desta funcao e criar uma funcao aux
    */

    // ------------------ BUILD ENV -----------------------
    std::vector<std::string> env_strings = buildEnv(req, script_path, config, cgiConfig);
    std::vector<char *> envp;
    for (size_t i = 0; i < env_strings.size(); i++)
        envp.push_back(strdup(env_strings[i].c_str()));
    envp.push_back(NULL);

    // ------------------ ARGV ----------------------------
    std::vector<char *> argv_vec;
    argv_vec.push_back(strdup(interpreter.c_str()));
    argv_vec.push_back(strdup(script_path.c_str()));
    argv_vec.push_back(NULL);

    int stdin_pipe[2];
    int stdout_pipe[2];

    bool early_error = false;
    std::string early_error_response;

    // -------------- CREATE PIPES ------------------------
    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0)
    {
        early_error = true;
        early_error_response =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "Failed to create pipes\n";
    }

    pid_t pid = -1;

    if (!early_error)
    {
        // ------------------ FORK ------------------------
        pid = fork();
        if (pid < 0)
        {
            early_error = true;
            early_error_response =
                "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
                "Fork failed\n";
        }
    }

    // ----------------------------------------------------
    // CHILD PROCESS
    // ----------------------------------------------------
    if (!early_error && pid == 0)
    {
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        execve(interpreter.c_str(), &argv_vec[0], &envp[0]);
        _exit(127);
    }

    // ----------------------------------------------------
    // PARENT PROCESS
    // ----------------------------------------------------
    std::string output;
    int status = 0;
    bool finished = false;
    bool timeout = false;

    if (!early_error)
    {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        write(stdin_pipe[1], req.body.c_str(), req.body.size());
        close(stdin_pipe[1]);

        fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

        time_t start_time = time(NULL);
        char buffer[4096];

        while (!finished)
        {
            ssize_t r = read(stdout_pipe[0], buffer, sizeof(buffer));
            if (r > 0)
                output.append(buffer, r);

            pid_t w = waitpid(pid, &status, WNOHANG);

            if (w == -1)
                finished = true;
            else if (w > 0)
                finished = true;
            else
            {
                if (config.cgi_timeout > 0 &&
                    difftime(time(NULL), start_time) > config.cgi_timeout)
                {
                    kill(pid, SIGKILL);
                    waitpid(pid, NULL, 0);
                    timeout = true;
                    finished = true;
                }
            }

            usleep(10000);
        }

        close(stdout_pipe[0]);
    }

    // ----------------------------------------------------
    // RETURN RESPONSE
    // ----------------------------------------------------
    if (early_error)
    {
        result.exit_status = 500;
        result.raw_output = early_error_response;
    }
    else if (timeout)
    {
        result.exit_status = 504;
        result.raw_output =
            "Status: 504\r\nContent-Type: text/html\r\n\r\n"
            "<h1>504 CGI Timeout</h1>";
    }
    else
    {
        if (WIFEXITED(status))
            result.exit_status = WEXITSTATUS(status);
        else
            result.exit_status = -1;

        result.raw_output = output;
    }

    /*
            pode ser outra funcao auxi
    */
    // ----------------------------------------------------
    // CLEANUP MEMORY
    // ----------------------------------------------------
    for (size_t i = 0; i < envp.size(); i++)
        free(envp[i]);

    for (size_t i = 0; i < argv_vec.size(); i++)
        free(argv_vec[i]);

    return result;
}

Response CgiHandler::parseCgiOutput(const std::string &raw)
{
    Response res;

    // 1. separar headers e body
    // Tenta CRLF primeiro (padrão CGI)
    size_t pos = raw.find("\r\n\r\n");

    std::cout << "\nRaw CGI Output:\n"
              << raw << std::endl;

    // Se não existir, tenta LF-LF (Python print padrão)
    if (pos == std::string::npos)
        pos = raw.find("\n\n");

    if (pos == std::string::npos)
    {
        // CGI realmente inválido
        /*
            Veja só todos iguais, refatorar
        */
        res.status = 500;
        res.body = "<h1>500 Invalid CGI Output</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return res;
    }

    std::string header_part = raw.substr(0, pos);
    std::string body_part = raw.substr(pos + 4);

    res.body = body_part;

    // 2. parse headers linha por linha
    std::istringstream iss(header_part);
    std::string line;
    bool has_status = false;

    while (std::getline(iss, line))
    {
        if (line.size() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!line.size())
            continue;

        size_t sep = line.find(':');
        if (sep == std::string::npos)
            continue;

        std::string key = line.substr(0, sep);
        std::string val = trim(line.substr(sep + 1));

        if (key == "Status")
        {
            has_status = true;
            res.status = atoi(val.c_str());
        }
        else
        {
            res.headers[key] = val;
        }
    }

    if (!has_status)
        res.status = 200;

    // 3. garantir Content-Length se não fornecido
    if (res.headers.find("Content-Length") == res.headers.end())
        res.headers["Content-Length"] = Utils::toString(res.body.size());

    return res;
}

Response CgiHandler::handleCgiRequest(const Request &req,
                                      const ServerConfig &config,
                                      const LocationConfig &loc,
                                      const CgiConfig &cgiConfig)
{
    Response res;
    //(void) cgiConfig;

    // monta caminho real do script

    std::string script_path = loc.root + req.uri.substr(loc.path.size());

    // passar de alguma forma a extencao
    CgiResult result = CgiHandler::execute(req, script_path, config, loc, cgiConfig);

    if (result.exit_status == 504)
    {
        /*
        uma funcao para esses retornos, parecem todos iguas.
        Reutilizar
        */
        res.status = 504;
        res.body = "<h1>504 Gateway Timeout (CGI)</h1>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    if (result.exit_status != 0 && result.exit_status != 200 && !result.raw_output.size())
    {
        res.status = 500;
        res.body = "<h1>500 CGI Execution Failed</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return res;
    }

    return CgiHandler::parseCgiOutput(result.raw_output);
}