# webserv 42

*This project was developed as part of the 42 curriculum by fjilaias, manandre, and alde-jes.*

## **Description**
The webserv is a lightweight HTTP web server, developed in C++, as part of the 42 curriculum.

The main objective of this project is to understand how a web server works internally, implementing it from scratch, without the use of external frameworks.

The server is capable of handling multiple client connections simultaneously, interpreting HTTP requests, and returning appropriate responses. It supports basic HTTP methods such as GET and POST, serves static files, manages configuration files, and follows the HTTP/1.1 protocol specifications.

This project explores fundamental concepts of low-level networking, such as sockets, non-blocking I/O, multiplexing with polling, and proper resource management. The focus is on performance, stability, and compliance with the HTTP standard.

## **Instructions**
Compilation

The project must be compiled using the provided Makefile.

``bash
make
``` This command generates the executable **webserv**
To clear the generated files:
```bash
make clean
``` To clear everything, including the executable:
```bash
make fclean
``` To recompile from scratch:
```bash
make re
```

## **Execution**
After compilation, the server can be run as follows:
```bash
./webserv <configuration_file>
```
Example:
```bash
./webserv config/default.conf
```

## **Usage**
After starting, the server listens on the port defined in the configuration file.

It can be accessed through a web browser or with tools like curl:
```bash
curl http://localhost:8080
```

## **Requirements**

- Linux operating system
- C++ compiler compatible with C++98
- Make