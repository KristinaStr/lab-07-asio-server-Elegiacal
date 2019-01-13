//
//  main.cpp
//  lab_37
//
//  Created by Артём Песоцкий on 20/11/2018.
//  Copyright © 2018 Артём Песоцкий. All rights reserved.
//

#include <iostream>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <gtest/gtest.h>

boost::asio::io_service service;
boost::recursive_mutex mutex;

class talk_to_client
{
    std::string log;
    boost::asio::ip::tcp::socket socket;
    std::chrono::time_point<std::chrono::system_clock> prev_ping;
public:
    talk_to_client()
    : socket(service)
    , prev_ping(std::chrono::system_clock::now())
    {}
    
    const std::string getLogin()
    {
        return log;
    }
    
    void setLogin(const std::string& newLogin)
    {
        log = newLogin;
    }
    
    std::chrono::time_point<std::chrono::system_clock> getLastPing()
    {
        return prev_ping;
    }
    
    void setLastPing(const std::chrono::time_point
                     <std::chrono::system_clock>& ping)
    {
        prev_ping = ping;
    }
    
    boost::asio::ip::tcp::socket& sock()
    {
        return socket;
    }
};

std::vector<talk_to_client*> clients;

void accept_thread()
{
    boost::asio::ip::tcp::acceptor acceptor
    {
        service, boost::asio::ip::tcp::endpoint
        {boost::asio::ip::address::from_string("127.0.0.1"), 8001}
    };

    auto client = new talk_to_client();
    boost::recursive_mutex::scoped_lock lock{mutex};
    clients.push_back(client);
}

void handle_clients_thread()
{
    for (auto i = 0; i < 65535; ++i)  //  while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        boost::recursive_mutex::scoped_lock lock{mutex};
        for (int j = clients.size() - 1; j >= 0; --j)
        {
            boost::asio::streambuf buf;
            size_t n = 0;
            boost::asio::streambuf::const_buffers_type buff = buf.data();
            std::string message(
            boost::asio::buffers_begin(buff),
            boost::asio::buffers_begin(buff) + n);
            auto now = std::chrono::system_clock::now();
            if ((now - clients[j]->getLastPing()) < std::chrono::seconds{5})
            {
                if (message.find("login ") != std::string::npos)
                {
                    std::string login = message.substr(
                    message.find("login") + 6);
                    clients[j]->setLogin(login);
                    clients[j]->setLastPing(
                    std::chrono::system_clock::now());
                    clients[j]->sock().write_some(
                    boost::asio::buffer("ping_ok"));
                } else if (message.find("ping") != std::string::npos)
                {
                    clients[j]->setLastPing(
                    std::chrono::system_clock::now());
                    clients[j]->sock().write_some(
                    boost::asio::buffer("ping_ok"));
                } else if (message.find("clients") != std::string::npos)
                {
                    clients[j]->setLastPing(
                    std::chrono::system_clock::now());
                    clients[j]->sock().write_some(
                    boost::asio::buffer("client_list_chaned"));
                    for (size_t m = 0; m < clients.size(); ++m)
                    {
                        clients[m]->sock().write_some(
                        boost::asio::buffer(clients[m]->getLogin()));
                    }
                }
            } else {
                delete clients[j];
                clients.erase(clients.begin() + j);
                std::cout << "Client disconnected" << std::endl;
            }
        }
    }
}

TEST(Server, ttcTest)
{
    std::vector <std::thread> threads;
    clients.clear();
    for (int i = 0; i < 5; ++i)
    {
        auto client = new talk_to_client();
        clients.push_back(client);
    }
    threads.emplace_back(accept_thread);
    threads.emplace_back(handle_clients_thread);
    threads[0].join();
    threads[1].join();
    if (!clients.empty())
    {
        throw std::runtime_error("Some error occurred");
    }
}
