#include "HttpParser.hpp"
#include <cstdlib>

static std::string  trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");

    if (a == std::string::npos)
        return std::string();

    size_t b = s.find_last_not_of(" \t\r\n");

    return s.substr(a, b - a + 1);
}

bool    HttpParser::hasCompleteRequest(const Buffer &buffer)
{
    std::string data = buffer.toString();
    size_t header_end = data.find("\r\n\r\n");

    if (header_end == std::string::npos)
        return (false);

    // If Content-Length present, check body length
    size_t pos = data.find("Content-Length:");
    if (pos != std::string::npos)
    {
        size_t eol = data.find("\r\n", pos);

        if (eol == std::string::npos)
            return (false);
        std::string val = data.substr(pos + strlen("Content-Length:"), eol - (pos + strlen("Content-Length:")));
        val = trim(val);

        int length = atoi(val.c_str());
        size_t body_start = header_end + 4;

        if (data.size() - body_start < (size_t)length)
            return (false);
    }
    return (true);
}

bool    HttpParser::parseRequest(Buffer &buffer, Request &req, size_t max_body_size)
{
    std::string data = buffer.toString();
    size_t header_end = data.find("\r\n\r\n");

    if (header_end == std::string::npos)
        return (false);

     req.header_end = header_end + 4; // guardar posição para chunked

    std::string headers_block = data.substr(0, header_end);
    std::istringstream ss(headers_block);
    std::string line;

    // Request line
    if (!std::getline(ss, line))
        return (false);
    if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
    {
        std::istringstream ls(line);
        ls >> req.method >> req.uri >> req.version;
    }

    // Headers
    while (std::getline(ss, line))
    {
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);
        if (line.empty())
            break ;
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue ;
        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));
        req.headers[key] = val;
    }

    // Body
    size_t body_len = 0;
    if (req.headers.count("Content-Length"))
    {
        body_len = (size_t)atoi(req.headers["Content-Length"].c_str());
        if (body_len > max_body_size)
        {
            req.too_large_body = true;
            return true; // parsed headers ok, body too large
        }
    }

    size_t total_len = header_end + 4 + body_len;
    if (buffer.size() < total_len)
        return (false); // not complete
    if (body_len > 0)
        req.body = data.substr(header_end + 4, body_len);
    else
        req.body.clear();

    // 2. CHUNKED DETECTADO?
    if (req.headers.count("Transfer-Encoding") &&
        req.headers["Transfer-Encoding"] == "chunked")
        return parseChunkedBody(buffer, req);

    // 3. CASO NORMAL: CONTENT-LENGTH
    if (req.headers.count("Content-Length"))
    {
        int len = atoi(req.headers["Content-Length"].c_str());
        if (data.size() < req.header_end + len)
            return false;

        req.body = data.substr(req.header_end, len);
        buffer.consume(req.header_end + len);
        return true;
    }

    // 4. sem corpo
    buffer.consume(total_len);
    return true;
}

bool HttpParser::parseChunkedBody(Buffer &buffer, Request &req)
{
    std::string data = buffer.toString();
    size_t pos = req.header_end; // posição logo após \r\n\r\n dos headers

    std::string final_body;

    while (true)
    {
        // 1. Ler tamanho em hexadecimal até CRLF
        size_t line_end = data.find("\r\n", pos);
        if (line_end == std::string::npos)
            return false; // ainda não chegou chunk completo

        std::string hexsize = data.substr(pos, line_end - pos);

        // Converter hex -> tamanho do chunk
        long chunk_size = strtol(hexsize.c_str(), NULL, 16);
        if (chunk_size < 0)
            return false;

        pos = line_end + 2;

        // 2. Chegou o chunk final?
        if (chunk_size == 0)
        {
            // confirmar \r\n final
            size_t end_mark = data.find("\r\n", pos);
            if (end_mark == std::string::npos)
                return false;

            // Remover tudo até o fim do chunked
            buffer.consume(end_mark + 2);

            req.body = final_body;
            return true;
        }

        // 3. Verificar se chunk completo já chegou
        if (data.size() < pos + chunk_size + 2)
            return false;

        // 4. Pegar os dados do chunk
        final_body.append(data, pos, chunk_size);

        // 5. Saltar chunk + CRLF
        pos += chunk_size + 2;
    }
}