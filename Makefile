NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = src/main.cpp src/WebServ/WebServ.cpp

OBJS = $(SRC:.cpp=.o)

HEADERS = src/WebServ/WebServ.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean:	clean
	rm -f $(NAME)

re:	fclean all