#include <exception>
class NeedAccountForLogin: public std::exception
{
    public:
        NeedAccountForLogin(){}
        const char * what () const throw ()
        {
            return "332: ";
        }
};