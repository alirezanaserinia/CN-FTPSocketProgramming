#include <string>
#include <vector>
#define QUIT_RESPONSE "221: Successful Quit."
#define SUCCESSFULLY_LOGGED_IN_RESPONSE "230: User logged in, proceed. Logged out if appropirate."
#define MAX_CLIENTS 30
#define BUF_SIZE  1025
#define CLIENT_BUF_SIZE  1024

std::map<std::string,std::string> status_code_command;
const std::vector<std::string> COMMANDS_NEED_DATA {"ls", "brw"};
// class User
// {
//     protected:
//         std::string name;
//         std::string password;
//         int download_size;

//     public:
//         User(std::string user_name, std::string user_password, int user_download_size){
//             name= user_name;
//             password= user_password;
//             download_size= user_download_size;
//         }
//         std::string get_name(){
//            return name; 
//         }
//         bool is_pass(std::string checking_password){
//             return password== checking_password;
//         }
//         virtual bool is_admin()= 0; 
// };

// class AdminUser : public User
// {
//     public:
//         AdminUser(std::string user_name, std::string user_password, int user_download_size): 
//             User(user_name,user_password,user_download_size){}
            
//         bool is_admin(){
//             return false;
//         }
//         static std::vector<std::string> files_only_admin_has_permission;
// };
// std::vector<std::string> AdminUser::files_only_admin_has_permission= std::vector<std::string>();

// class TypicalUser : public User
// {
//     public:
//         TypicalUser(std::string user_name, std::string user_password, int user_download_size): 
//             User(user_name,user_password,user_download_size){}
//         bool is_admin(){
//             return true;
//         }
// };