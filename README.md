# webserv 42

*Este projeto foi desenvolvido como parte do currículo da 42 por fjilaias, manandre e alde-jes.*


## **Description**
O webserv é um servidor web HTTP leve, desenvolvido em C++, como parte do currículo da 42.
O objetivo principal deste projeto é compreender como um servidor web funciona internamente, implementando-o do zero, sem o uso de frameworks externos.

O servidor é capaz de lidar com múltiplas conexões de clientes em simultâneo, interpretar requisições HTTP e devolver respostas adequadas. Ele suporta métodos HTTP básicos como o GET e o POST, serve ficheiros estáticos, gere ficheiros de configuração e segue as especificações do protocolo HTTP/1.1.

Com este projeto, são explorados conceitos fundamentais de redes de baixo nível, como sockets, I/O não bloqueante, multiplexação com poll, e uma gestão correta de recursos. O foco está no desempenho, na estabilidade e no cumprimento do padrão HTTP.

## **Instructions**
Compilação

O projeto deve ser compilado utilizando o Makefile fornecido.
```bash
meke
```
Este comando gera o executável **webserv**
Para limpar os ficheiros gerados:
```bash
meke clean
```
Para limpar tudo, incluindo o executável:
```bash
meke fclean
```
Para recompilar do zero:
```bash
meke re
```

## **Execution**
Após a compilação, o servidor pode ser executado da seguinte forma:
```bash
./webserv <ficheiro_de_configuração>
```
Exemplo: 
```bash
./webserv config/default.conf
```
## **Usage**
Depois de iniciado, o servidor fica à escuta na porta definida no ficheiro de configuração.
Pode ser acedido através de um navegador web ou com ferramentas como curl:
```bash
curl http://localhost:8080
```

## **Requirements**
- Sistema operativo Linux
- Compilador C++ compatível com C++98
- Make
