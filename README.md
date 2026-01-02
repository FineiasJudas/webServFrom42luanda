upload keep-alive
-carregar arquivo muito grande(Status 413 Payload Too Large)
req: http://localhost:8080/uploads.html

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

[2025-12-14 20:17:05] INFO  | PATH: /uploads/m22.png
[2025-12-14 20:17:05] DEBUG | Status 200 OK
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 5
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 6
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 7
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 8
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 9
[2025-12-14 20:17:26] WARN  | [TIMEOUT] Keep-Alive Timeout FD 10
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 5
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 6
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 7
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 8
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 9
[2025-12-14 20:17:26] WARN  | Conexão fechada FD 10
[2025-12-14 20:17:56] NEW   | Nova conexão aceita FD 5
[2025-12-14 20:17:56] NEW   | Status 413 Payload Too Large
[2025-12-14 20:17:59] NEW   | Status 413 Payload Too Large
[2025-12-14 20:18:01] NEW   | Status 413 Payload Too Large
[2025-12-14 20:18:02] NEW   | Status 413 Payload Too Large
[2025-12-14 20:18:03] NEW   | Status 413 Payload Too Large
[2025-12-14 20:18:19] WARN  | [TIMEOUT] Read Timeout FD 5
[2025-12-14 20:18:19] WARN  | Conexão fechada FD 5




## Não usar o `errno`

Não precisamos do while no read:

- O controle do fluxo de leitura não é feito pelo `read()`, mas pelo `epoll`/`poll`.  
- Um socket só entra na função `handleRead()` porque o `poll`/`epoll` indicou que **há dados disponíveis para ler**.

### Comportamento esperado:

1. Quando os dados acabam, o `read()` retorna `-1`.  
2. Nesse ponto, nós **paramos de ler** e voltamos ao loop principal.  
3. O próximo evento de leitura será tratado **quando o `poll`/`epoll` indicar novamente** que há dados.  

**Conclusão:**  
Não é necessário diferenciar entre “erro temporário” ou “erro real” dentro do `read()`. O epoll/poll garante que você só será notificado quando houver algo a processar. 


# CGI - COMUM GETAWAY INTERFACE
## CGI - PHP
INTERPERTADOR: /usr/bin/php-cgi
### ARQUIVO GRANDE DEMIS
PARECE QUE O ARQUIVO /etc/php/8.3/cgi/php.ini contem vars que limitão o tamanho do arquivo
a carregar. Alterei para:
upload_max_filesize = 10M      ; tamanho máximo de cada arquivo
post_max_size = 20M            ; tamanho máximo do corpo da requisição

bash `
 ~ php-cgi -i | grep "Loaded Configuration File"
<tr><td class="e">Loaded Configuration File </td><td class="v">/etc/php/8.3/cgi/php.ini </td></tr>
➜  ~ vi /etc/php/8.3/cgi/php.ini
➜  ~ sudo vi /etc/php/8.3/cgi/php.ini
`

# 📘 2️⃣ Texto pronto para o README (pode copiar e colar)

### ### HTTP Methods Handling

Este servidor implementa um roteamento explícito e seguro dos métodos HTTP, conforme o subject do projeto.

#### GET

* GET é o único método permitido para servir ficheiros estáticos.
* GET também pode ser encaminhado para CGI, caso a extensão do ficheiro esteja configurada como CGI na location.

#### POST, DELETE e outros métodos

* Métodos diferentes de GET **não são permitidos para conteúdo estático**.
* POST, DELETE e outros métodos só são aceitos quando a requisição é encaminhada para um CGI.
* Caso um método não permitido seja usado fora de um CGI, o servidor retorna **405 Method Not Allowed**.

#### DELETE

* O método DELETE é totalmente suportado pelo servidor.
* Pode ser testado diretamente via `curl`:

  ```bash
  curl -X DELETE http://localhost:8080/cgi-bin/php/delete_file.php?file=example.txt
  ```
* Em navegadores, onde DELETE não é suportado nativamente por formulários ou iframes, é utilizado GET como fallback para operações administrativas via CGI.

Essa abordagem garante:

* Separação clara entre conteúdo estático e dinâmico
* Segurança contra operações destrutivas fora do CGI
* Conformidade com o subject do projeto

---

## 🔥 Observação final (importante para avaliação)

Se o avaliador perguntar:

> “Por que DELETE não funciona no browser?”

Resposta curta e certa:

> “DELETE funciona no servidor e pode ser testado via curl. Para o frontend HTML, usamos GET como fallback porque browsers não suportam DELETE de forma confiável em formulários e iframes.”

