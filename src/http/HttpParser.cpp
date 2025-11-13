#include "HttpParser.hpp"
#include <sstream>
#include <cstdlib>
#include <algorithm>

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

// ------------------------------------------------------------
// 1️⃣ Detecta se há um request HTTP completo no buffer
// ------------------------------------------------------------
bool HttpParser::hasCompleteRequest(const Buffer &buffer) {
    std::string data = buffer.toString();

    // procura fim dos headers
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    // verifica se há Content-Length (body esperado)
    size_t pos = data.find("Content-Length:");
    if (pos != std::string::npos) {
        size_t end = data.find("\r\n", pos);
        std::string line = data.substr(pos, end - pos);
        size_t colon = line.find(':');
        int length = std::atoi(line.substr(colon + 1).c_str());

        // corpo começa após "\r\n\r\n"
        size_t body_start = header_end + 4;
        return (data.size() - body_start >= (size_t)length);
    }
    // sem corpo, então está completo
    return true;
}

// ------------------------------------------------------------
// 2️⃣ Extrai e consome o request completo do buffer
// ------------------------------------------------------------
bool HttpParser::parseRequest(Buffer &buffer, Request &req, size_t max_body_size) {
    std::string data = buffer.toString();
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    std::string header_block = data.substr(0, header_end);
    std::istringstream header_stream(header_block);
    std::string line;

    // 1️⃣ Primeira linha — método, URI, versão
    if (!std::getline(header_stream, line))
        return false;
    {
        std::istringstream request_line(line);
        request_line >> req.method >> req.uri >> req.version;
    }

    // 2️⃣ Cabeçalhos
    while (std::getline(header_stream, line)) {
        if (line == "\r" || line.empty())
            break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim(line.substr(0, colon));
            std::string val = trim(line.substr(colon + 1));
            if (!key.empty())
                req.headers[key] = val;
        }
    }

    // 3️⃣ Corpo (se houver)
    size_t body_start = header_end + 4;
    size_t body_len = 0;
    if (req.headers.count("Content-Length"))
        body_len = std::atoi(req.headers["Content-Length"].c_str());
    if (body_len > max_body_size)
        throw std::runtime_error("Body too large");

    req.body = data.substr(body_start, body_len);

    // 4️⃣ Remove request completo do buffer
    buffer.consume(body_start + body_len);

    return true;
}
