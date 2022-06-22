#include <exception>
class BadSequenceOfCommand: public std::exception
{
    public:
        BadSequenceOfCommand(){}
        const char * what () const throw ()
        {
            return "503: ";
        }
};