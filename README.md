# webserv 42

*This project was developed as part of the 42 curriculum by fjilaias, manandre, and alde-jes.*

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

* [Nginx Study Guide](https://iris-dungeon-e66.notion.site/Guia-de-estudos-Nginx-2433cdde6693807da12cd3bf69f04163)

### **HTTP/1.1**

* [YouTube – The Evolution of HTTP Protocol – vadebyte](https://www.youtube.com/watch?v=NLLlFOgRcpM)
* [YouTube – HTTP Protocol – Mauro de Boni](https://www.youtube.com/watch?v=NLLlFOgRcpM)

### **Non-blocking Systems**

* [webserv: Building a Non-Blocking Web Server in C++98](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7)

### **How AI was used**

During the development of **webserv**, AI was used only to generate ideas, support function testing, and clarify concepts. All code was reviewed, tested, and discussed among the team, ensuring full understanding and complete control over the project.
