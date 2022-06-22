#include <exception>
class NoSpecialError: public std::exception
{
    public:
        NoSpecialError(){}
        const char * what () const throw ()
        {
            return "500: ";
        }
};