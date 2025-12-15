
# webserv

## KEYWORDS_HPP
Esse header define um namespace chamado KW, onde cada campo é uma constante std::string usada como palavra-chave no projecto.

Ele serve para evitar escrever strings “literais” pelo código e garantir que todas as palavras sejam consistentes e fáceis de modificar num só lugar.

### usar no código
bash ``
#include "src/utils/keywords.hpp"

if (token == KW::SERVER)
    faz ou deixa de fazer alguma coisa;

``

### adicionar mais conteúdo
no arquivo src/utils/keywords.hpp, adicionar  
bash ``
const std::string TIMEOUT = "timeout";
``

## WebServException.hpp

Este arquivo define a classe base de exceções WebServException e várias exceções específicas para o servidor web (ex.: PortException, SocketException, BindException, etc.).

Ele permite tratar erros de forma organizada e consistente, fornecendo mensagens claras para cada tipo de falha.

### usar no código

bash ``
if (port < 1 || port > 65535)
    throw PortException("Porta inválida: " + portStr);
``
### adicionar novas exceções

bash ``
class NomeDaExcecao : public WebServException {
public:
    inline NomeDaExcecao(const std::string &value)
        : WebServException("NomeDaExcecao: " + value) {}
};
``

