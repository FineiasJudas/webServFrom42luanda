#ifndef HEADERS_HPP
#define HEADERS_HPP

# include <map>
# include <vector>
# include <string>
# include <fcntl.h>
# include <cstdlib>
# include <cstring>
# include <unistd.h>
# include <errno.h>
# include <sys/epoll.h>
# include <iostream>
# include <sys/types.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/select.h>

# include "../src/core/Server.hpp"
# include "../src/core/Buffer.hpp"
# include "../src/core/Poller.hpp"
# include "../src/core/Connection.hpp"
# include "../src/utils/Utils.hpp"
# include "../src/utils/Logger.hpp"
# include "../src/http/Request.hpp"
# include "../src/http/Response.hpp"
# include "../src/http/HttpParser.hpp"
# include "../src/http/Router.hpp"
# include "../src/cgi/CgiHandler.hpp"
# include "../src/cgi/StaticFiles.hpp"
# include "../src/config/Config.hpp"
# include "../src/config/ConfigParser.hpp"

#endif