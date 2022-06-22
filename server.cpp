#include <map>
#include <string.h>
#include <iterator>	
#include <pwd.h>	
#include <grp.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>
#include "json/json.h"
#include "def.hpp"
#include "Exceptions/InvalidUserNameOrPassword.hpp"
#include "Exceptions/BadSequenceOfCommand.hpp"
#include "Exceptions/NeedAccountForLogin.hpp"
#include "Exceptions/SyntaxError.hpp"
#include "Exceptions/NoSpecialError.hpp"
#include "Exceptions/CannotOpenDataConnectio.hpp"
#include "Users/AdminUser.hpp"
#include "Users/TypicalUser.hpp"

typedef std::string (*command_func)(std::vector<std::string>);

std::vector<User*> users;
User *incoming_user[MAX_CLIENTS], *logged_in_user[MAX_CLIENTS];
int client_number;
int client_command_socket[MAX_CLIENTS], client_data_socket[MAX_CLIENTS];
std::string client_program_path[MAX_CLIENTS], client_extrinsic_program_path[MAX_CLIENTS];

void log(std::string message);

std::string ls_command(std::vector<std::string> arg);
std::string brw_command(std::vector<std::string> arg);
std::string mkd_command(std::vector<std::string> arg);
std::string pwd_command(std::vector<std::string> arg);
std::string cwd_command(std::vector<std::string> arg);
std::string help_command(std::vector<std::string> arg);
std::string dele_command(std::vector<std::string> arg);
std::string user_command(std::vector<std::string> arg);
std::string quit_command(std::vector<std::string> arg);
std::string pass_command(std::vector<std::string> arg);
std::string touch_command(std::vector<std::string> arg);
std::string rename_command(std::vector<std::string> arg);
std::string owner_command(std::vector<std::string> arg);
std::string group_command(std::vector<std::string> arg);

int main(int argc, char const *argv[])
{
     //////////////////////////  initialize command list and status code map

    std::map<std::string,command_func> command;
    command.insert(std::pair<std::string,command_func>("ls",ls_command));
    command.insert(std::pair<std::string,command_func>("brw",brw_command));
    command.insert(std::pair<std::string,command_func>("mkd",mkd_command));
    command.insert(std::pair<std::string,command_func>("pwd",pwd_command));
    command.insert(std::pair<std::string,command_func>("cwd",cwd_command));
    command.insert(std::pair<std::string,command_func>("user",user_command));
    command.insert(std::pair<std::string,command_func>("quit",quit_command));
    command.insert(std::pair<std::string,command_func>("pass",pass_command));
    command.insert(std::pair<std::string,command_func>("help",help_command));
    command.insert(std::pair<std::string,command_func>("dele",dele_command));
    command.insert(std::pair<std::string,command_func>("touch",touch_command));
    command.insert(std::pair<std::string,command_func>("rename",rename_command));
    command.insert(std::pair<std::string,command_func>("owner",owner_command));
    command.insert(std::pair<std::string,command_func>("group",group_command));

    status_code_command.insert(std::pair<std::string,std::string>("221: ","Successful Quit."));
    status_code_command.insert(std::pair<std::string,std::string>("226: ","List transfer done."));
    status_code_command.insert(std::pair<std::string,std::string>("226_2: ","Successful Download."));
    status_code_command.insert(std::pair<std::string,std::string>("230: ","User logged in, proceed. Logged out if appropirate."));
    status_code_command.insert(std::pair<std::string,std::string>("250: ","deleted."));
    status_code_command.insert(std::pair<std::string,std::string>("250_2: ","Successful change."));
    status_code_command.insert(std::pair<std::string,std::string>("257: ","created."));
    status_code_command.insert(std::pair<std::string,std::string>("331: ","User name okay, need password."));
    status_code_command.insert(std::pair<std::string,std::string>("332: ","Need account for login."));
    status_code_command.insert(std::pair<std::string,std::string>("425: ","Can't open data connection."));
    status_code_command.insert(std::pair<std::string,std::string>("430: ","Invalid username or password."));
    status_code_command.insert(std::pair<std::string,std::string>("500: ","Error."));
    status_code_command.insert(std::pair<std::string,std::string>("501: ","Syntax error in parameters or arguments."));        
    status_code_command.insert(std::pair<std::string,std::string>("503: ","Bad sequence of command."));
    status_code_command.insert(std::pair<std::string,std::string>("550: ","File unavailable."));
    
    //////////////////////////  parse and read data from Json file
    
    std::ifstream cofig_file("config.json");
    Json::Reader reader;
    Json::Value json_obj;
    reader.parse(cofig_file, json_obj);
    int command_channel_port= json_obj["commandChannelPort"].asInt();
    int data_channel_port= json_obj["dataChannelPort"].asInt();
    const Json::Value& files_json= json_obj["files"];
    const Json::Value& users_json= json_obj["users"];
    for (int i = 0; i < files_json.size(); i++){
        AdminUser::files_only_admin_has_permission.push_back(files_json[i].asString());
    }
    for (int i = 0; i < users_json.size(); i++){
        std::string user_name= users_json[i]["user"].asString();
        std::string user_pass= users_json[i]["password"].asString();
        int user_download_size= std::stoi(users_json[i]["size"].asString());

        if (users_json[i]["admin"].asString()== "true")
            users.push_back(new AdminUser(user_name, user_pass, user_download_size));
        else
            users.push_back(new TypicalUser(user_name, user_pass, user_download_size));
    }


    //////////////////////////  initialize all client's sockets to zero

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		client_command_socket[i]= 0;
        client_data_socket[i]= 0;
        logged_in_user[i]= NULL;
        incoming_user[i]= NULL;
        client_program_path[i]= "";
        client_extrinsic_program_path[i]= "";
	}
    fd_set client_fds; 

    //////////////////////////  create socket

    int command_channel_fd, data_channel_fd, new_command_channel_fd, new_data_channel_fd, nbytes, nbytes2;
    struct sockaddr_in srv_command_port, srv_data_port;
    command_channel_fd = socket(AF_INET, SOCK_STREAM, 0);
    data_channel_fd= socket(AF_INET, SOCK_STREAM, 0);

    //////////////////////////  bind to determined port

    srv_command_port.sin_family = AF_INET;
    srv_command_port.sin_port = htons(command_channel_port);
    srv_command_port.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    bind(command_channel_fd, (struct sockaddr*) &srv_command_port, sizeof(srv_command_port));

    srv_data_port.sin_family = AF_INET;
    srv_data_port.sin_port = htons(data_channel_port);
    srv_data_port.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    bind(data_channel_fd, (struct sockaddr*) &srv_data_port, sizeof(srv_data_port));

    //////////////////////////  listen to the port and accept client

    std::cout<<"Server is listening !!\n";
    listen(command_channel_fd, 5);
    listen(data_channel_fd, 5);

    int max_sd, sd, data_sd;
    struct sockaddr_in address,data_address;
    int addrlen= sizeof(address);
    int data_addrlen= sizeof(data_address);
    char *command_channel_message = "Welcome to our server !! It is command channel\r\n";
    char *data_channel_message = "Welcome to our server !! It is data channel\r\n";
    char buffer[BUF_SIZE];
    while(true)
	{
		//clear the socket set
		FD_ZERO(&client_fds);
	
		//add master socket to set
		FD_SET(command_channel_fd, &client_fds);
		max_sd = command_channel_fd;
			
		//add child sockets to set
		for (int i= 0 ; i< MAX_CLIENTS ; i++)
		{
			//socket descriptor
			sd = client_command_socket[i];
				
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &client_fds);
				
			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}
	
		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		select(max_sd+ 1 , &client_fds , NULL , NULL , NULL);
			
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(command_channel_fd, &client_fds))
		{
			new_command_channel_fd= accept(command_channel_fd, 
                                        (struct sockaddr *)&address, (socklen_t*)&addrlen);
            new_data_channel_fd= accept(data_channel_fd, 
                                        (struct sockaddr *)&data_address, (socklen_t*)&data_addrlen);

			//////////////////////////  send new connection greeting message in command channel

			send(new_command_channel_fd, command_channel_message, strlen(command_channel_message), 0);
            read(new_command_channel_fd, buffer, 1024);

            //////////////////////////  send new connection greeting message in data channel

			send(new_data_channel_fd, data_channel_message, strlen(data_channel_message), 0);
				
			//add new socket to array of sockets
			for (int i= 0; i< MAX_CLIENTS; i++)
			{
				//if position is empty
				if (client_command_socket[i]== 0 )
				{
					client_command_socket[i] = new_command_channel_fd;
                    client_data_socket[i] = new_data_channel_fd;
                    client_program_path[i]= client_extrinsic_program_path[i]= std::string(buffer);
                    memset(&buffer, 0, sizeof(buffer));
					printf("Adding to list of sockets as %d\n" , i);
					break;
				}
			}
		}
			
		//else its some IO operation on some other socket
		for (client_number= 0; client_number< MAX_CLIENTS; client_number++)
		{
			sd= client_command_socket[client_number];
            data_sd= client_data_socket[client_number];
				
			if (FD_ISSET(sd , &client_fds))
			{
				//Check if it was for closing, and also read the incoming message
				if (read(sd, buffer, 1024)== 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" ,
						inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close(sd);
                    close(data_sd);
					client_command_socket[client_number]= 0;
                    client_data_socket[client_number]= 0;
				}
					
				//Echo back the message that came in
				else
				{
                    std::string buf_string= std::string(buffer);
                    std::stringstream buf_stream(buf_string);
                    std::istream_iterator<std::string> begin(buf_stream);
                    std::istream_iterator<std::string> end;

                    std::cout<< "Client: "<< buf_string<<'\n';

                    //////////////////////////////  parse client's request

                    std::vector<std::string> input_command(begin, end);
                    std::string result;
                    try{
                        std::vector<std::string> argument_list(input_command.begin()+1, input_command.end());
                        if ((logged_in_user[client_number]== NULL) && input_command[0]!="user" && input_command[0]!="quit" 
                                && input_command[0]!="pass")
                            throw new NeedAccountForLogin();
                        if (command.count(input_command[0])== 0)
                            throw new SyntaxError();
                        result= command[input_command[0]](argument_list);
                    }
                    catch(std::exception* exep){
                        result= exep->what()+ status_code_command[exep->what()];
                    }
                    std::cout<< "Response: "<<result<< '\n';
                    memset(&buffer, 0, sizeof(buffer));
                    sprintf(buffer, result.c_str());
                    send(sd , buffer , strlen(buffer) , 0 );
				}
			}
		}
	}
    
    return 0;
}

std::string user_command(std::vector<std::string> arg){
    for (auto user: users){
        if (user->get_name()== arg[0]){
            incoming_user[client_number]= user;
            return "331: " + status_code_command["331: "];
        }
    }
    throw new InvalidUserNameOrPassword();
}

std::string quit_command(std::vector<std::string> arg){
    if (logged_in_user[client_number]== NULL)
        throw new NeedAccountForLogin();
    log(logged_in_user[client_number]->get_name()+ " logged out.");
    logged_in_user[client_number]= NULL;
    return "221: " + status_code_command["221: "];
}

std::string pass_command(std::vector<std::string> arg){
    if (incoming_user[client_number]== NULL)
        throw new BadSequenceOfCommand();
    if (incoming_user[client_number]->is_pass(arg[0])){
        logged_in_user[client_number]= incoming_user[client_number];
        log(logged_in_user[client_number]->get_name()+ " logged in.");
        return "230: " + status_code_command["230: "];
    }
    throw new InvalidUserNameOrPassword();
}

std::string help_command(std::vector<std::string> arg){
    std::string help_result= "214\n";
    help_result+= "USER [name], Its argument is used to specify the user's string. It is used for user authentication.\n";
    help_result+= "PASS [password], Its argument is used to specify the user's password. It is used to complete authentication.\n";
    help_result+= "PWD. It is used to show working directory path.\n";
    help_result+= "MKD [directory path], Its argument is used to specify the directory path. It is used for making a new directory path.\n";
    help_result+= "DELE -f [filename], Its argument is used to specify the file's name. It is used for deletion a file.\n";
    help_result+= "DELE -d [directory path], Its argument is used to specify the directory path. It is used for deletion a directory.\n";
    help_result+= "LS. It is used to show all of files in current directory.\n";
    help_result+= "CWD [path], Its argument is used to specify the directory path. It is used to change directory.\n";
    help_result+= "RENAME [from] [to], Its arguments is used to specify current file's name and new file's name. It is used to renaming a file.\n";
    help_result+= "RETR [name], Its argument is used to specify the file's name. It is used to download a file.\n";
    return help_result;
}

void log(std::string message){
    std::ofstream log_file("log.txt",std::ios::out | std::ios::app);
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string date= std::string(dt);
    //date.erase(std::remove(date.begin(), date.end(), '\n'), date.end());
    log_file<< "The local date and time is: "<< date<< " | ";
    log_file<< "What happended: "<< message<< '\n';
    log_file.close();
}

std::string touch_command(std::vector<std::string> arg){
    std::ofstream new_file(arg[0],std::ios::out);
    new_file.close();
    log("a new file is created: "+ arg[0]);
    return "257: " + arg[0]+ ' '+ status_code_command["257: "];
}

std::string brw_command(std::vector<std::string> arg){
    char buffer[BUF_SIZE];
    int data_sd= client_data_socket[client_number];
    std::vector<std::string> dir_list; 
    std::string result= ""; 
    struct dirent *dir;
    char server_path[256];
    getcwd(server_path, 256);
    DIR *server_directory = opendir(server_path); 
    if (server_directory) {
        while ((dir = readdir(server_directory)) != NULL) 
            result+= std::string(dir->d_name)+ '\n';
        closedir(server_directory); 
    }
    std::cout<< "Data transfer: "<< result<<"\n";
    memset(&buffer, 0, sizeof(buffer));
    sprintf(buffer, result.c_str());
    send(data_sd , buffer , strlen(buffer) , 0 );
    return "226: "+ status_code_command["226: "];
}

std::string mkd_command(std::vector<std::string> arg){
    if (mkdir(arg[0].c_str(), 0777)== -1)
        throw new NoSpecialError();
    log("a new directory is created: "+ arg[0]);
    return "257: " + arg[0]+ ' '+ status_code_command["257: "]; 
}

std::string dele_command(std::vector<std::string> arg){
    if (arg[0]== "-f"){
        ////////////////////////////////    delete a file
        
        if (remove(arg[1].c_str())!= 0)
            throw new NoSpecialError();
        log(arg[1]+ " file is deleted.");
        return "250: " + arg[1]+ ' '+ status_code_command["250: "]; 
    }
    else if (arg[0]== "-d"){
        //////////////////////////////  delete a directory

        if (rmdir(arg[1].c_str()) == -1)  // Remove the directory
            throw new NoSpecialError();
        log(arg[1]+ " directory is deleted.");
        return "250: " + arg[1]+ ' '+ status_code_command["250: "];
    }
    else
        throw new SyntaxError();
}

std::string rename_command(std::vector<std::string> arg){
    if (rename(arg[0].c_str(), arg[1].c_str()) != 0)
		throw new NoSpecialError();
	log(arg[0]+ " file is renamed to "+ arg[1]);
    return "250: " + status_code_command["250_2: "]; 
}

std::string pwd_command(std::vector<std::string> arg){
    if (arg.size())
        throw new SyntaxError();
    return "257: " + client_extrinsic_program_path[client_number];
}

std::string cwd_command(std::vector<std::string> arg){
    if (arg.size()> 1)
        throw new SyntaxError();

    std::vector<std::string> path_directory_vec;
    std::stringstream new_directory_relational_path(client_extrinsic_program_path[client_number]);
    std::string directory_path_part, new_directory= "";
    while (std::getline(new_directory_relational_path, directory_path_part, '/')){
        path_directory_vec.push_back(directory_path_part);
    }
    if (arg.size()== 0){
        new_directory= client_program_path[client_number];
    }
    else if (arg.size()== 1){
        std::stringstream new_directory_relational_path(arg[0]);
        while (std::getline(new_directory_relational_path, directory_path_part, '/')) {
            if (directory_path_part== "..")
                path_directory_vec.pop_back();
            else
                path_directory_vec.push_back(directory_path_part);
        }
        for (auto directory_path_part: path_directory_vec) 
            new_directory+= directory_path_part+ '/';
    }
    
    client_extrinsic_program_path[client_number]= new_directory;
    return "250: " + status_code_command["250_2: "];
}

std::string ls_command(std::vector<std::string> arg){
    char buffer[BUF_SIZE];
    int data_sd= client_data_socket[client_number];
    std::vector<std::string> dir_list; 
    std::string result= ""; 
    struct dirent *dir;
    std::string client_path= client_extrinsic_program_path[client_number];
    DIR *server_directory = opendir(client_path.c_str()); 
    if (server_directory) {
        while ((dir = readdir(server_directory)) != NULL) 
            result+= std::string(dir->d_name)+ '\n';
        closedir(server_directory); 
    }
    std::cout<< "Data transfer: "<< result<<"\n";
    memset(&buffer, 0, sizeof(buffer));
    sprintf(buffer, result.c_str());
    send(data_sd , buffer , strlen(buffer) , 0 );
    return "226: "+ status_code_command["226: "];
}

std::string owner_command(std::vector<std::string> arg){
    struct stat info;
    stat(arg[0].c_str(), &info);  // Error check omitted
    struct passwd *pw = getpwuid(info.st_uid);
    if (pw != 0) {
        return pw->pw_name;
    }

}

std::string group_command(std::vector<std::string> arg){
    struct stat info;
    stat(arg[0].c_str(), &info);  // Error check omitted
    struct group  *gr = getgrgid(info.st_gid);
    if (gr != 0) {
        return gr->gr_name;
    }

}

std::string retr_command(std::vector<std::string> arg){
    if (arg.size()!= 1)
        throw new SyntaxError();
    if (!logged_in_user[client_number]->has_enough_download_volume(10))
        throw new CannotOpenDataConnectio();
    std::string client_path= client_program_path[client_number];
    std::ifstream  source_file(arg[0], std::ios::binary);
    std::ofstream  downloaded_file(client_path+ arg[0], std::ios::binary);
    downloaded_file << source_file.rdbuf();
    logged_in_user[client_number]->decrease_download_volume(10);
    return "226: "+ status_code_command["226_2: "];
}