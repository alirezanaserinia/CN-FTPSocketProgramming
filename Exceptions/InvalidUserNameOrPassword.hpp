#include <exception>
class InvalidUserNameOrPassword: public std::exception
{
    public:
        InvalidUserNameOrPassword(){}
        const char * what () const throw ()
        {
            return "430: ";
        }
};