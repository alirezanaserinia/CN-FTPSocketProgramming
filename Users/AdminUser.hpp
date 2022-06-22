#include "User.hpp"

class AdminUser : public User
{
    public:
        AdminUser(std::string user_name, std::string user_password, int user_download_size): 
            User(user_name,user_password,user_download_size){}
            
        bool is_admin(){
            return false;
        }
        static std::vector<std::string> files_only_admin_has_permission;
};
std::vector<std::string> AdminUser::files_only_admin_has_permission= std::vector<std::string>();