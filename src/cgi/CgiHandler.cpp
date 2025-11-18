#include "../../includes/Headers.hpp"
#include "CgiHandler.hpp"

// helper trimming
/*static std::string  trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");

    if (a == std::string::npos)
        return std::string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}*/

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
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
        it != req.headers.end(); ++it)
    {
        std::string key = it->first;
        std::string val = it->second;
        // Normalize key: make uppercase, replace '-' with '_'
        std::string up;
        for (size_t i = 0; i < key.size(); ++i)
        {
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
    const std::string& script_path, const ServerConfig& config)
{
    CgiResult result;
    result.exit_status = -1;
    result.raw_output.clear();

    // Check script exists and is a file
    struct stat st;

    if (stat(script_path.c_str(), &st) < 0)
    {
        // script not found
        std::ostringstream ss;
        ss << "Status: 500\r\nContent-Type: text/plain\r\n\r\nCGI script not found: " << script_path << "\n";
        result.raw_output = ss.str();
        return result;
    }

    // build environment vector
    std::vector<std::string> env_strings = buildEnv(req, script_path, config);

    // convert to char* envp[]
    std::vector<char*> envp;
    for (size_t i = 0; i < env_strings.size(); ++i)
    {
        char *e = strdup(env_strings[i].c_str());
        envp.push_back(e);
    }
    envp.push_back(NULL);

    // argv: execute the script file itself. If you want to call an interpreter explicitly,
    // change argv to point to interpreter and first arg to script_path.
    std::vector<char*> argv;
    // Use script_path as argv[0]
    char *a0 = strdup(script_path.c_str());
    argv.push_back(a0);
    argv.push_back(NULL);

    // Create pipes
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) == -1)
    {
        // pipe error
        std::ostringstream ss;
        ss << "Status: 500\r\nContent-Type: text/plain\r\n\r\nCGI pipe creation failed: " << strerror(errno) << "\n";
        result.raw_output = ss.str();
        // free env/argv
        for (size_t i=0;i<envp.size()-1;++i) free(envp[i]);
        free(a0);
        return result;
    }
    if (pipe(stdout_pipe) == -1)
    {
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        std::ostringstream ss;
        ss << "Status: 500\r\nContent-Type: text/plain\r\n\r\nCGI pipe creation failed: " << strerror(errno) << "\n";
        result.raw_output = ss.str();
        for (size_t i=0;i<envp.size()-1;++i) free(envp[i]);
        free(a0);
        return result;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        // fork failed
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        std::ostringstream ss;
        ss << "Status: 500\r\nContent-Type: text/plain\r\n\r\nCGI fork failed: " << strerror(errno) << "\n";
        result.raw_output = ss.str();
        for (size_t i=0;i<envp.size()-1;++i) free(envp[i]);
        free(a0);
        return result;
    }

    if (pid == 0)
    {
        // Child process
        // dup stdin_pipe[0] -> STDIN_FILENO
        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
            _exit(127);
        }
        // dup stdout_pipe[1] -> STDOUT_FILENO
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            _exit(127);
        }
        // Also redirect stderr to stdout so we capture errors
        if (dup2(stdout_pipe[1], STDERR_FILENO) == -1) {
            _exit(127);
        }

        // Close unused fds in child
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);

        // execve
        // Note: envp.data() isn't C++98; use envp[0]... prepare array
        // Build env array for execve
        size_t envcount = envp.size();
        char **env_array = (char**)malloc(sizeof(char*) * envcount);
        for (size_t i = 0; i < envcount; ++i) env_array[i] = envp[i];
        // Build argv array
        size_t argcnt = argv.size();
        char **argv_array = (char**)malloc(sizeof(char*) * argcnt);
        for (size_t i = 0; i < argcnt; ++i) argv_array[i] = argv[i];

        // Execute script directly. If script is not executable or lacks shebang, this will fail.
        execve(script_path.c_str(), argv_array, env_array);

        // If execve fails
        _exit(127);
    } else {
        // Parent
        // Close child's ends
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // Write request body to child's stdin (if any)
        ssize_t total_written = 0;
        const char *body_ptr = req.body.c_str();
        ssize_t body_len = (ssize_t)req.body.size();

        while (total_written < body_len) {
            ssize_t w = write(stdin_pipe[1], body_ptr + total_written, body_len - total_written);
            if (w < 0) {
                if (errno == EINTR) continue;
                // write error
                break;
            }
            total_written += w;
        }
        // finished writing
        close(stdin_pipe[1]);

        // Read all from child's stdout
        const size_t BUF_SZ = 4096;
        char buffer[BUF_SZ];
        std::ostringstream out;

        while (true) {
            ssize_t r = read(stdout_pipe[0], buffer, BUF_SZ);
            if (r < 0) {
                if (errno == EINTR) continue;
                // read error -> break
                break;
            } else if (r == 0) {
                // EOF
                break;
            } else {
                out.write(buffer, r);
            }
        }

        // Close read end
        close(stdout_pipe[0]);

        // Wait for child
        int status = 0;
        pid_t w = waitpid(pid, &status, 0);
        if (w == -1) {
            result.exit_status = -1;
        } else {
            if (WIFEXITED(status)) result.exit_status = WEXITSTATUS(status);
            else result.exit_status = -1;
        }

        // Fill result
        result.raw_output = out.str();

        // free envp and argv allocated strings
        for (size_t i = 0; i < envp.size()-1; ++i) free(envp[i]); // last is NULL
        free(a0);
        // argv had only a0; if you allocate more later, free them too
    }

    return result;
}
