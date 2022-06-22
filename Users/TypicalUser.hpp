#include "User.hpp"

class TypicalUser : public User
{
    public:
        TypicalUser(std::string user_name, std::string user_password, int user_download_size): 
            User(user_name,user_password,user_download_size){}
        bool is_admin(){
            return true;
        }
};