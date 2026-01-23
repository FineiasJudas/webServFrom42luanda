*This project has been created as part of the 42 curriculum by fjilaias, manandre, and alde-jes.*

## **Description**

**webserv** is a lightweight HTTP web server developed in C++ as part of the 42 curriculum.
The main goal of this project is to understand how a web server works internally by implementing it from scratch, without using external frameworks.

The server can handle multiple client connections simultaneously, parse HTTP requests, and return appropriate responses. It supports basic HTTP methods like **GET** and **POST**, serves static files, manages configuration files, and follows the HTTP/1.1 protocol specifications.

This project explores fundamental low-level networking concepts, such as sockets, non-blocking I/O, multiplexing with `poll`, and proper resource management. The focus is on performance, stability, and compliance with the HTTP standard.

## **Instructions**

### **Compilation**

The project should be compiled using the provided Makefile:

```bash
make
```

This command generates the **webserv** executable.

To clean generated files:

```bash
make clean
```

To clean everything, including the executable:

```bash
make fclean
```

To recompile from scratch:

```bash
make re
```

### **Execution**

After compilation, the server can be run as follows:

```bash
./webserv <configuration_file>
```

Example:

```bash
./webserv config/default.conf
```

### **Usage**

Once started, the server listens on the port defined in the configuration file.
It can be accessed through a web browser or tools like `curl`:

```bash
curl http://localhost:8080
```

### **Requirements**

* Linux operating system
* C++ compiler compatible with **C++98**
* Make

## **Resources**

### **NGINX**
* [Beginner’s Guide](https://nginx.org/en/docs/beginners_guide.html)
* [Nginx Study Guide](https://iris-dungeon-e66.notion.site/Guia-de-estudos-Nginx-2433cdde6693807da12cd3bf69f04163)
* [Practical Nginx: A Beginner’s Step-by-Step Project Guide](https://mohammadtaheri.medium.com/practical-nginx-a-beginners-step-by-step-project-guide-6f4c7540c06f)

### **HTTP/1.1**
* [YouTube – The Evolution of HTTP Protocol – vadebyte](https://www.youtube.com/watch?v=NLLlFOgRcpM)
* [YouTube – HTTP Protocol – Mauro de Boni](https://www.youtube.com/watch?v=NLLlFOgRcpM)

### **Non-blocking Systems**
* [webserv: Building a Non-Blocking Web Server in C++98](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7)
* [E/S Bloqueante e Não Bloqueante em Sistemas Operacionais](https://www.geeksforgeeks.org/operating-systems/blocking-and-nonblocking-io-in-operating-system/)

### **How AI was used**

During the development of **webserv**, AI was used only to generate ideas, support function testing, and clarify concepts. All code was reviewed, tested, and discussed among the team, ensuring full understanding and complete control over the project.

For example:

Initially, in the `createListenSockets` function, we accepted malformed listening arrangements, such as `listen 127.0.0.1:`, without a port. We didn't handle IP+PORT and used a prohibited function, `inet_addr`, to associate the IP with the PORT, configuring the Socket address.

But after studies and consultations, we found a way to use an alternative function for this purpose, `getaddrinfo`,

`
addrinfo structure hints;
struct addrinfo *res;

std::memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

  int ret = getaddrinfo(
ip == "0.0.0.0"? NULL: ip.c_str(),
porta.c_str(),
and hints,
&res);

`

which retrieves information from the associated socket in a `struct addrinfo res` structure, allowing the creation of a socket that accepts a PORT or IP+PORT `int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);`


Another case:

In `route`, we had to know how to decide whether a given request should be handled by the server or by GCI.

/* 7 HTTP methods */
std::string search = req.method;

bool method_req_exists = std::find(loc.methods.begin(), loc.methods.end(), search) != loc.methods.end();

if (!method_req_exists)
return notAllowMethodResponse(config);

// from here on the method is allowed
std::string ext = getExtension(rq.path);

// 1 If it's not GET → only CGI
if (req.method! = "GET" && ext! = "")
{
for (size_t i = 0; i < loc.cgi.size(); i++)
{
if (loc.cgi[i].extension == ext)
return CgiHandler::handleCgiRequest(rq,config,loc, loc.cgi[i], connection);

}

return notAlloweMethodResponse(config);

}

//2 GET
for (size_t i = 0; i < loc.cgi.size(); i++)

{
if (loc.cgi[i].extension == ext)
return CgiHandler::handleCgiRequest(rq, config, loc, loc.cgi[i], conn);

}

// 3 Static GET
return methodGet(config, loc, fsPath, rq.uri);


However, the issue arose when dealing with CGI, we need to create a process and execute a script.

But the CGI is blocked due to the loop with usleep(10000) which waits for the child process to finish, freezing the server during:

" // 6) Fork
pid_t pid = fork();
if (pid < 0)
{
result.raw_output =
"Status: 500\r\nContent type: text/simple\r\n\r\n"
"CGI fork failed\n";
return result;
}

// 7) Child process
if (pid == 0)
{
// → STDIN
dup2(stdin_pipe[0], STDIN_FILENO);
// → STDOUT
dup2(stdout_pipe[1], STDOUT_FILENO);
dup2(stdout_pipe[1],STDERR_FILENO);

close(stdin_pipe[1]);
close(stdout_pipe[0]);

execve(interpreter.c_str(), argv.data(), envp.data());

_exit(127); // exec failed

}

// 8) Parent Process...

close(stdin_pipe[0]);

close(stdout_pipe[1]);

write(stdin_pipe[1], req.body.c_str(), req.body.size());

close(stdin_pipe[1]);

charbuffer[4096];

std::string output;

time_t start_time = time(NULL);

int status = 0;

bool finished = false;

bool timeout = false;

while (!finished)

{
ssize_t r = read(stdout_pipe[0], buffer, sizeof(buffer));

if (r > 0)

output.append(buffer, r);

pid_t w = waitpid(pid, &status, WNOHANG);

if (w == -1)

{
finished = true;

}
else if (w > 0)

{
finished = true;

}
other {
if (config.cgi_timeout > 0 &&
difftime(tempo(NULL),start_time) > config.cgi_timeout)

{
kill(pid,SIGKILL);
waitpid(pid, NULL, 0);
timeout = true;
finished = true;

}
}

sleep(10000);

}"

Therefore, with the help of AI, content that makes With the server:

1. Do not wait for the CGI process in execute()
2. Return immediately after the fork with a "pending" state
3. Add pipes to the epoll to read asynchronously
4. Save the CGI state in the connection

In each, we add a structure to control a possible CGI connection state:
"structure CgiState {
pid_t pid;
int stdout_fd;
int stdin_fd;
output std::string;
time_t initial_time;
bool stdin_closed;

CgiState(): pid(-1), stdout_fd(-1), stdin_fd(-1),
start_time(0), stdin_closed(false) {}
};", where in execute we create:

"// Create CGI state CgiState *state = new CgiState();
state->pid = pid;
state->stdin_fd = stdin_pipe[1];
state->stdout_fd = stdout_pipe[0];
state->start_time = hour(NULL);
state->stdin_closed = false; and we return a flag

"// Return as pending
result.is_pending = true;
result.cgi_state = state;

"

On the MasterServer we check if it is a CGI and "
Response res = Router::route(req, *sc, conn);

if (res.status == 0 && conn->cgi_state)
{
// Adds CGI stdout to epoll for reading
poller.addFd(conn->cgi_state->stdout_fd, EPOLLIN | EPOLLOUT);
cgiFdToClientFd[conn->cgi_state->stdout_fd] = clientFd;

// If we still have stdin open, add it for writing
if (conn->cgi_state->stdin_fd != -1)

{
poller.addFd(conn->cgi_state->stdin_fd, EPOLLIN | EPOLLOUT);
cgiFdToClientFd[conn->cgi_state->stdin_fd] = clientFd;

}

has_pending_cgi = true;
processed++;
break ;

}

// Só mudar para escrita se não há CGI pendente
if (processed > 0 && !has_pending_cgi)
    poller.modifyFd(clientFd, EPOLLOUT);

// Se há CGI pendente, desativar leitura até CGI terminar
if (has_pending_cgi)
    poller.modifyFd(clientFd, 0);  // Sem eventos até CGI terminar

" and we jump to handle the next request, when the CGI is synchronized.

3 case:
