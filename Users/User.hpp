#ifndef Included_NameModel_H
#define Included_NameModel_H 53
#include <string>
class User
{
    protected:
        std::string name;
        std::string password;
        int download_size;

    public:
        User(std::string user_name, std::string user_password, int user_download_size){
            name= user_name;
            password= user_password;
            download_size= user_download_size;
        }
        std::string get_name(){
           return name; 
        }
        bool is_pass(std::string checking_password){
            return password== checking_password;
        }
        virtual bool is_admin()= 0;
        
        void decrease_download_volume(int volume) {
            download_size = download_size - volume;
        }

        bool has_enough_download_volume(int volume) { 
            if (volume <= download_size)
                return true;
            else 
                return false;
        } 
};
#endif