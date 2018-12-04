//
//  main.cpp
//  server_asio
//
//  Created by Артём Песоцкий on 23/11/2018.
//  Copyright © 2018 Артём Песоцкий. All rights reserved.
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

boost::recursive_mutex mutex;
boost::asio::io_service service;
const int msg_buffer = 1024;


// Client class
class talk_to_client
{
private:
    boost::asio::ip::tcp::socket socket;
    std::string log;
    boost::posix_time::ptime prev_ping;
    char* message;
public:
    static std::vector<std::shared_ptr<talk_to_client>> users;
    talk_to_client()
    : socket(service)
    {
        message = new char[msg_buffer];
        prev_ping = boost::posix_time::microsec_clock::local_time();
    }
    void login(const std::string& l)
    {
        log = l;
        std::cout << "Logged in as " << log << std::endl;
    }
    // Process and write
    void process_request()
    {
        if (std::find(message, message + msg_buffer, '\n'))
        {
            return;
        }
        std::string message(message, msg_buffer);
        std::size_t pos = message.find("login");
        if (pos != std::string::npos)
        {
            login(message.substr(pos));
        }
        if (message.find("ask_clients") != std::string::npos)
        {
            ask_for_users();
        }
        prev_ping = boost::posix_time::microsec_clock::local_time();
    }
    void write(const std::string& msg)
    {
        message = {0};
        socket.write_some(boost::asio::buffer(msg));
    }
    // Get socket and login
    boost::asio::ip::tcp::socket& sock()
    {
        return socket;
    }
    const std::string getLogin()
    {
        return log;
    }
    void answer_to_user()
    {
        // Check for command
        try
        {
            process_request();

        } catch (boost::system::system_error&)
        {
            socket.close();
        }
        // Timeout check
        if (timed_out())
        {
            socket.close();
            std::cout << "Closing " << log << ": Deprecated" << std::endl;
        }
    }
    static void ask_for_users()
    {
        for (size_t i = 0; i < users.size(); i++)
        {
            users[i]->write(users[i]->getLogin());
        }
    }
    bool timed_out() const
    {
        boost::posix_time::ptime now =
        boost::posix_time::microsec_clock::local_time();
        int16_t is_out = (now - prev_ping).total_milliseconds();
        return is_out > 5000 ;
    }
};
void accept_thread()
{
    boost::asio::ip::tcp::acceptor acceptor(service,
    boost::asio::ip::tcp::endpoint
    (boost::asio::ip::address::from_string
    ("127.0.0.1"), 8001));
    while (true)
    {
        auto user = std::make_shared<talk_to_client>();
        acceptor.accept(user->sock());
        boost::recursive_mutex::scoped_lock lock(mutex);
        talk_to_client::users.push_back(user);
    }
}
void handle_clients_thread()
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::millisec(1));
        boost::recursive_mutex::scoped_lock lk(mutex);
        for (auto user : talk_to_client::users)
        {
            user->answer_to_user();
        }
        talk_to_client::users.erase(std::remove_if(
        talk_to_client::users.begin(), talk_to_client::users.end(),
        boost::bind(&talk_to_client::timed_out, _1)),
        talk_to_client::users.end());
    }
}
int main()
{
    boost::thread_group threads;
    threads.create_thread(accept_thread);
    threads.create_thread(handle_clients_thread);
    threads.join_all();
    return 0;
}
