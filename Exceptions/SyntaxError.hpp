#include <exception>
class SyntaxError: public std::exception
{
    public:
        SyntaxError(){}
        const char * what () const throw ()
        {
            return "501: ";
        }
};