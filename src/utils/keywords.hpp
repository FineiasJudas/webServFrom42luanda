#ifndef KEYWORDS_HPP
#define KEYWORDS_HPP

#include <string>

namespace KW {
    const std::string SERVER = "server";
    const std::string LOCATION = "location";
    const std::string LISTEN = "listen";
    const std::string ROOT = "root";
    const std::string ERROR_PAGE = "error_page";
    const std::string SERVER_NAME = "server_name";
    const std::string MAX_BODY_SIZE = "max_body_size";
    const std::string AUTO_INDEX = "auto_index";
    const std::string METHODS = "methods";
    const std::string CGI_PATH = "cgi_path";
    const std::string CGI_EXTENSION = "cgi_extension";
    const std::string DIRECTORY_LISTING = "directory_listing";
    const std::string ON = "on";
    const std::string OFF = "off";
    const std::string UPLOAD_DIR = "upload_dir";


    //caracteres especiais
    const std::string BLOCK_START = "{";
    const std::string BLOCK_END = "}";
    const std::string COMMENT = "#";
    const std::string SEMICOLON = ";";
    const std::string SPACE = " ";
    const std::string TAB = "\t";
    const std::string NEW_LINE = "\n";
    const std::string ROOT_DEFAULT = "/";


    //valores default
    const std::string DEFAULT_ERROR_PAGE = "/error.html";
    const std::string INDEX = "index";
    const int MAX_VALUE_PORT = 65535;
    const int MIN_VALUE_PORT = 0;


}

#endif
