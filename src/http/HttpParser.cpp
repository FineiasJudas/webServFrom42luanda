#include "../utils/Utils.hpp"
#include "HttpParser.hpp"
#include <sstream>
#include <cstdlib>
#include <string>

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
        return false;

    // Detectar chunked
    if (data.find("Transfer-Encoding: chunked") != std::string::npos)
        return true; // parseChunkedBody decide depois

    // Content-Length
    size_t pos = data.find("Content-Length:");
    if (pos != std::string::npos)
    {
        size_t end = data.find("\r\n", pos);
        std::string len_str = data.substr(pos + 15, end - (pos + 15));
        size_t len = std::atoi(len_str.c_str());

        return data.size() >= header_end + 4 + len;
    }

    // Sem body
    return true;
}

std::string HttpParser::urlDecode(const std::string &str)
{
    std::string result;
    char hex[3] = {0};

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '+')
        {
            result += ' ';
        }
        else if (str[i] == '%' && i + 2 < str.size())
        {
            hex[0] = str[i + 1];
            hex[1] = str[i + 2];

            int value;
            std::istringstream iss(std::string(hex, 2));
            iss >> std::hex >> value;

            result += static_cast<char>(value);
            i += 2;
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

void    HttpParser::parseQueryParams(Request &req)
{
    size_t pos = req.uri.find('?');
    std::string temp_query;

    if (pos != std::string::npos)
    {
        req.path = req.uri.substr(0, pos);
        temp_query = req.uri.substr(pos + 1);
        req.query_string = temp_query;
    }
    else
    {
        req.path = req.uri;
        req.query_string.clear();
    }

    if (req.query_string.empty())
        return;

    std::vector<std::string> pairs = Utils::split(req.query_string, '&');

    for (size_t i = 0; i < pairs.size(); ++i)
    {
        size_t eq = pairs[i].find('=');
        if (eq != std::string::npos)
        {
            std::string key = pairs[i].substr(0, eq);
            std::string value = pairs[i].substr(eq + 1);

            // Decodificar chave e valor
            key = urlDecode(key);
            value = urlDecode(value);

            req.query[key] = value;
        }
    }
}

bool    HttpParser::parseRequest(Buffer &buffer, Request &req, size_t max_body_size)
{
    std::string data = buffer.toString();
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    req.header_end = header_end + 4;

    std::string headers_block = data.substr(0, header_end);
    std::istringstream ss(headers_block);
    std::string line;

    if (!std::getline(ss, line))
        return false;
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    {
        std::istringstream ls(line);
        ls >> req.method >> req.uri >> req.version;
    }

    if (req.method.empty() || req.uri.empty() || req.version.empty())
        return false;

    parseQueryParams(req);
    std::map<std::string, int> header_count;
    while (std::getline(ss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));
        req.headers[key] = val;

        header_count[key]++;
    
        if (key == "Host" && header_count[key] > 1)
        {
            req.bad_request = true;
            req.bad_request_reason = "Multiple Host headers";
            return true;
        }
    }

    if (req.version == "HTTP/1.1")
    {
        if (!req.headers.count("Host") || req.headers["Host"].empty())
        {
            req.bad_request = true;
            req.bad_request_reason = "Missing Host header";
            return true;
        }
    }

    // CHUNKED TEM PRIORIDADE 
    if (req.headers.count("Transfer-Encoding") &&
        req.headers["Transfer-Encoding"] == "chunked")
        return parseChunkedBody(buffer, req);

    // TAMANHO DO CONTEÚDO (BODY) 
    if (req.headers.count("Content-Length"))
    {
        size_t body_len = std::atoi(req.headers["Content-Length"].c_str());

        if (body_len > max_body_size)
        {
            req.too_large_body = true;
            return true;
        }

        if (buffer.size() < req.header_end + body_len)
            return false;

        req.body = data.substr(req.header_end, body_len);
        buffer.consume(req.header_end + body_len);
        return true;
    }

    // Sem body
    buffer.consume(req.header_end);
    return true;
}

bool    HttpParser::parseChunkedBody(Buffer &buffer, Request &req)
{
    size_t pos = req.header_end;
    std::string data = buffer.toString();
    std::string body;

    while (true)
    {
        size_t line_end = data.find("\r\n", pos);
        if (line_end == std::string::npos)
            return false;

        std::string hex = data.substr(pos, line_end - pos);
        long chunk_size = std::strtol(hex.c_str(), NULL, 16);
        if (chunk_size < 0)
            return false;

        pos = line_end + 2;

        // chunk final
        if (chunk_size == 0)
        {
            if (data.size() < pos + 2)
                return false;

            buffer.consume(pos + 2);
            req.body = body;
            return true;
        }

        if (data.size() < pos + chunk_size + 2)
            return false;

        body.append(data, pos, chunk_size);
        pos += chunk_size + 2;
    }
}