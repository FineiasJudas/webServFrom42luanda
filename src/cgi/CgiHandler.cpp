#include "../../includes/Headers.hpp"
#include "CgiHandler.hpp"

// helper trimming
static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
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
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) {
        std::string key = it->first;
        std::string val = it->second;
        // Normalize key: make uppercase, replace '-' with '_'
        std::string up;
        for (size_t i = 0; i < key.size(); ++i) {
            char c = key[i];
            if (c == '-') up += '_';
            else up += (char)toupper(c);
        }
        std::string envname = std::string("HTTP_") + up + "=" + val;
        env.push_back(envname);
    }

    // If ServerConfig has error_pages or other info, you can add them here if needed.

    return env;
}

CgiResult CgiHandler::execute(const Request& req,
                              const std::string& script_path,
                              const ServerConfig& config)
{
    CgiResult result;
    result.exit_status = -1;
    result.raw_output.clear();

    // ============================
    // 1) Verifica se o script existe
    // ============================
    struct stat st;
    if (stat(script_path.c_str(), &st) < 0)
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI script not found: " + script_path + "\n";
        return result;
    }

    // ============================
    // 2) Monta o interpretador correto
    // ============================
    std::string     interpreter;
    for (size_t i = 0; i < config.locations.size(); i++)
    {
        if (script_path.find(config.locations[i].root) == 0 &&
            !config.locations[i].cgi_path.empty())
        {
            interpreter = config.locations[i].cgi_path;
            break;
        }
    }
    
    if (interpreter.empty())
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI interpreter not defined in config\n";
        return result;
    }

    // interpreter deve existir no SO
    if (access(interpreter.c_str(), X_OK) != 0)
    {
        std::ostringstream ss;
        ss << "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
           << "CGI interpreter not executable: " << interpreter
           << " (" << strerror(errno) << ")\n";
        result.raw_output = ss.str();
        return result;
    }

    // ============================
    // 3) Monta o ENV
    // ============================
    std::vector<std::string> env_strings = buildEnv(req, script_path, config);

    std::vector<char*> envp;
    for (size_t i = 0; i < env_strings.size(); ++i)
        envp.push_back(strdup(env_strings[i].c_str()));
    envp.push_back(NULL);

    // ============================
    // 4) Monta o ARGV
    // ============================
    std::vector<char*> argv;
    argv.push_back(strdup(interpreter.c_str()));  // argv[0]
    argv.push_back(strdup(script_path.c_str()));  // argv[1]
    argv.push_back(NULL);

    // ============================
    // 5) Cria pipes STDIN / STDOUT
    // ============================
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0)
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI pipe creation failed\n";
        return result;
    }

    // ============================
    // 6) Fork
    // ============================
    pid_t pid = fork();
    if (pid < 0)
    {
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI fork failed\n";
        return result;
    }

    // ============================
    // 7) Child Process
    // ============================
    if (pid == 0)
    {
        // → STDIN
        dup2(stdin_pipe[0], STDIN_FILENO);
        // → STDOUT
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        execve(interpreter.c_str(), argv.data(), envp.data());

        _exit(127);  // exec falhou
    }

    // ============================
    // 8) Parent Process
    // ============================
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    // envia body (POST)
    write(stdin_pipe[1], req.body.c_str(), req.body.size());
    close(stdin_pipe[1]);

    // lê STDOUT do CGI
    char buffer[4096];
    std::ostringstream out;
    ssize_t r;

    while ((r = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
        out.write(buffer, r);

    close(stdout_pipe[0]);

    // espera processo
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        result.exit_status = WEXITSTATUS(status);
    else
        result.exit_status = -1;

    result.raw_output = out.str();

    // libera memória
    for (size_t i = 0; i < envp.size(); ++i)
        free(envp[i]);
    for (size_t i = 0; i < argv.size(); ++i)
        free(argv[i]);

    return result;
}

Response CgiHandler::parseCgiOutput(const std::string &raw)
{
    Response res;

    // 1. separar headers e body
    // Tenta CRLF primeiro (padrão CGI)
    size_t pos = raw.find("\r\n\r\n");

    std::cout << "\nRaw CGI Output:\n" << raw << std::endl;

    // Se não existir, tenta LF-LF (Python print padrão)
    if (pos == std::string::npos)
        pos = raw.find("\n\n");

    if (pos == std::string::npos)
    {
        // CGI realmente inválido
        res.status = 500;
        res.body = "<h1>500 Invalid CGI Output</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return res;
    }

    std::string header_part = raw.substr(0, pos);
    std::string body_part   = raw.substr(pos + 4);

    res.body = body_part;

    // 2. parse headers linha por linha
    std::istringstream  iss(header_part);
    std::string line;
    bool has_status = false;

    while (std::getline(iss, line))
    {
        if (line.size() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);

        if (!line.size())
            continue;

        size_t sep = line.find(':');
        if (sep == std::string::npos)
            continue;

        std::string key = line.substr(0, sep);
        std::string val = trim(line.substr(sep + 1));

        if (key == "Status") {
            has_status = true;
            res.status = atoi(val.c_str());
        }
        else {
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
                                  const LocationConfig &loc)
{
    Response    res;

    // monta caminho real do script

    std::string script_path = loc.root + req.uri.substr(loc.path.size());

    CgiResult   result = CgiHandler::execute(req, script_path, config);

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