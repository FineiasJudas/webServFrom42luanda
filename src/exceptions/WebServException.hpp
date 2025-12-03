#ifndef WEB_SERV_EXCEPTION
#define WEB_SERV_EXCEPTION

#include <iostream>

class WebServException: public std::exception {
    private:
        std:string msg;
    
    public:
        WebServException(std::string value):msg(value);
        virtual ~WebServException();
        virtual const char *what() const throw();

};

#endif