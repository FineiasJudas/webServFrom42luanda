#include "../../includes/Headers.hpp"
#include "../utils/Logger.hpp"
#include "CgiHandler.hpp"
#include <cstdlib>

// helper trimming
static std::string trim(const std::string &s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return std::string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// splitar a uri
static void splitUri(const std::string &uri, std::string &path, std::string &query){
    size_t q = uri.find('?');
    if (q == std::string::npos){
        path = uri;
        query.clear();
    } else{
        path = uri.substr(0, q);
        query = uri.substr(q + 1);
    }
}

// -- Acho que vai ter vários buildEnv's para cada tipo de extenção --
void addPhpVars(std::vector<std::string> &env) { env.push_back("REDIRECT_STATUS=200"); }

static std::string normalizeHeaderKey(const std::string &key){
    std::string up;
    up.reserve(key.size());
    for (size_t i = 0; i < key.size(); ++i){
        char c = key[i];
        if (c == '-'){up += '_';}
        else{up += (char)std::toupper(c);}
    }
    return (up);
}

static void addBasicVars(const Request &req, std::vector<std::string> &env, const std::string upload_dir){
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=" + req.version);
    env.push_back("REQUEST_METHOD=" + req.method);
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("UPLOAD_DIR="+upload_dir);
}

static void addScriptVars(const std::string &script_path, std::vector<std::string> &env){
    env.push_back("SCRIPT_FILENAME=" + script_path);
}

static void addUriVars(const Request &req, std::vector<std::string> &env){
    std::string path, query;
    splitUri(req.uri, path, query);

    env.push_back("REQUEST_URI=" + req.uri);
    env.push_back("QUERY_STRING=" + query);

    if (req.headers.count("Host"))
        {env.push_back("SERVER_NAME=" + req.headers.at("Host"));}
    else {env.push_back("SERVER_NAME=localhost");}
}

static void addContentVars(const Request &req, std::vector<std::string> &env){
    if (req.headers.count("Content-Length"))
        {env.push_back("CONTENT_LENGTH=" + req.headers.at("Content-Length"));}
    if (req.headers.count("Content-Type"))
        {env.push_back("CONTENT_TYPE=" + req.headers.at("Content-Type"));}
}

static void addHttpHeaders(const Request &req, std::vector<std::string> &env){
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it){
        std::string up = normalizeHeaderKey(it->first);
        if (up == "CONTENT_LENGTH" || up == "CONTENT_TYPE"){continue;}
        env.push_back("HTTP_" + up + "=" + it->second);
    }
}

static std::vector<std::string> buildEnv(
    const Request &req,
    const std::string &script_path,
    const ServerConfig &config,
    const CgiConfig &cgiConfig,
    const std::string upload_dir){

    (void)config;
    std::vector<std::string> env;

    addBasicVars(req, env, upload_dir);
    addScriptVars(script_path, env);
    addUriVars(req, env);
    addContentVars(req, env);
    addHttpHeaders(req, env);

    if (cgiConfig.extension == ".php"){addPhpVars(env);}
    return (env);
}

CgiResult   CgiHandler::execute(const Request &req,
                              const std::string &script_path,
                              const ServerConfig &config,
                              const LocationConfig &loc,
                              const CgiConfig &cgiConfig){

    (void)loc;
    CgiResult result;
    result.exit_status = -1;
    result.is_pending = false;
    result.cgi_state = NULL;
    
    // - SCRIPT EXISTS? -
    struct stat st;
    if (stat(script_path.c_str(), &st) != 0){
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI script not found: " +
            script_path + "\n";
        return (result);
    }

    // - INTERPRETER -
    std::string interpreter;
    if (cgiConfig.path.empty()){
        if (cgiConfig.extension == ".php")
            {interpreter = "/usr/bin/php-cgi";}
        else if (cgiConfig.extension == ".py")
            {interpreter = "/usr/bin/python3";}
    } else {interpreter = cgiConfig.path;}

    if (interpreter.empty()){
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "CGI interpreter not defined\n";
        return (result);
    }

    if (access(interpreter.c_str(), X_OK) != 0){
        result.raw_output =
            "Status: 500\r\nContent-Type: text/plain\r\n\r\n"
            "Interpreter not executable\n";
        return (result);
    }

    // - BUILD ENV -
    std::vector<std::string> env_storage = buildEnv(req, script_path, config, cgiConfig, loc.upload_dir);
    std::vector<char *> envp;
    envp.reserve(env_storage.size() + 1);
    for (size_t i = 0; i < env_storage.size(); ++i)
        {envp.push_back(const_cast<char *>(env_storage[i].c_str()));}
    envp.push_back(NULL);

    // - ARGV -
    std::vector<std::string> argv_storage;
    argv_storage.reserve(2 + req.query.size());
    argv_storage.push_back(interpreter);
    argv_storage.push_back(script_path);
    for (std::map<std::string, std::string>::const_iterator it = req.query.begin();
         it != req.query.end();
         ++it){
        std::string pair = it->first + "=" + it->second;
        argv_storage.push_back(pair);
    }

    std::vector<char *> argv_vec;
    argv_vec.reserve(argv_storage.size() + 1);
    for (size_t i = 0; i < argv_storage.size(); ++i)
        {argv_vec.push_back(const_cast<char *>(argv_storage[i].c_str()));}

    argv_vec.push_back(NULL);
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0){
        result.raw_output = "Status: 500\r\nContent-Type: text/plain\r\n\r\nFailed to create pipes\n";
        return (result);
    }

    // - Tornar pipes não-bloqueantes ANTES do fork -
    fcntl(stdin_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid < 0){
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        result.raw_output = "Status: 500\r\nContent-Type: text/plain\r\n\r\nFork failed\n";
        return (result);
    }

    // - FILHO -
    if (pid == 0){
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        execve(interpreter.c_str(), &argv_vec[0], &envp[0]);
        std::exit(127);
    }

    // - PAI - NÃO BLOQUEIA -
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    // - Criar estado CGI -
    CgiState *state = new CgiState();
    state->pid = pid;
    state->stdin_fd = stdin_pipe[1];
    state->stdout_fd = stdout_pipe[0];
    state->start_time = std::time(NULL);
    state->stdin_closed = false;
    state->write_offset = 0;

    // CORRIGIDO: Guardar body para escrever depois
    if (!req.body.empty()){
        state->pending_write = req.body;

        // Tentar escrever o máximo possível imediatamente
        ssize_t written = write(state->stdin_fd,
                               state->pending_write.c_str(),
                               state->pending_write.size());

        if (written > 0)
            state->write_offset = written;
        else if (written < 0){
            Logger::log(Logger::ERROR, "CGI: Erro ao escrever no stdin: " + 
                       std::string(strerror(errno)));
        }

        // Verificar se escrevemos tudo
        if (state->write_offset >= state->pending_write.size()){
            close(state->stdin_fd);
            state->stdin_fd = -1;
            state->stdin_closed = true;
            //Logger::log(Logger::INFO, "CGI: stdin fechado (tudo escrito de imediato)");
        }
    } else {
        // Sem body, fechar stdin imediatamente
        close(state->stdin_fd);
        state->stdin_fd = -1;
        state->stdin_closed = true;
        //Logger::log(Logger::INFO, "CGI: stdin fechado (sem body)");
    }
    
    // Retornar como pendente
    result.is_pending = true;
    result.cgi_state = state;
    
    /*Logger::log(Logger::INFO, "CGI iniciado de forma não-bloqueante, PID: " + 
               Utils::toString(pid));*/
    
    return (result);
}

Response    CgiHandler::parseCgiOutput(const std::string &raw)
{
    Response    res;

    size_t pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos){pos = raw.find("\n\n");}

    if (pos == std::string::npos){
        /*
            Veja só todos iguais, refatorar
        */
        res.status = 500;
        res.body = "<h1>500 Invalid CGI Output</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return (res);
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
            {line.erase(line.size() - 1);}

        if (!line.size()){continue;}
        size_t sep = line.find(':');
        if (sep == std::string::npos){continue;}
        std::string key = line.substr(0, sep);
        std::string val = trim(line.substr(sep + 1));

        if (key == "Status"){
            has_status = true;
            res.status = std::atoi(val.c_str());
        } else {res.headers[key] = val;}
    }

    if (!has_status){res.status = 200;}

    // garantir Content-Length se não fornecido
    if (res.headers.find("Content-Length") == res.headers.end())
        {res.headers["Content-Length"] = Utils::toString(res.body.size());}
    return (res);
}

Response CgiHandler::handleCgiRequest(const Request &req,
                                      const ServerConfig &config,
                                      const LocationConfig &loc,
                                      const CgiConfig &cgiConfig, Connection *conn){
    Response    res;
    std::string script_path = loc.root + "/" + req.path.substr(loc.path.size());

    CgiResult result = CgiHandler::execute(req, script_path, config, loc, cgiConfig);

    conn->cgi_state = result.cgi_state;
    //Se está pendente, retornar resposta vazia (será tratado depois)
    if (result.is_pending){
        res.status = 0;  // Sinal especial de "pendente"
        return res;
    }
    if (result.exit_status == 504){
        res.status = 504;
        res.body = "<h1>504 Gateway Timeout (CGI)</h1>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return (res);
    }

    if (result.exit_status != 0 && result.exit_status != 200){
        res.status = 500;
        res.body = "<h1>500 CGI Execution Failed</h1>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return (res);
    }

    return CgiHandler::parseCgiOutput(result.raw_output);
}