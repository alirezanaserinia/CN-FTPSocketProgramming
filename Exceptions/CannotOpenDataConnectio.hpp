#include <exception>
class CannotOpenDataConnectio: public std::exception
{
    public:
        CannotOpenDataConnectio(){}
        const char * what () const throw ()
        {
            return "425: ";
        }
};