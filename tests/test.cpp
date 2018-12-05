// Copyright 2018 Your Name <your_email>

#include <gtest/gtest.h>

boost::asio::ip::tcp::acceptor acceptor(service,
    boost::asio::ip::tcp::endpoint
    (boost::asio::ip::address::from_string
    ("127.0.0.1"), 8001));
auto user = std::make_shared<talk_to_client>();
        acceptor.accept(user->sock());
talk_to_client::users.push_back(user);

#include <header.hpp>
