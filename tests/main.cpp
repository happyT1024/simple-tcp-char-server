//
// Created by matvey on 18.04.23.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "init_log.h"

#include "Server/Client/Client.h"

boost::asio::io_service service;

TEST(Client, getUsername){
    std::queue<std::pair<std::string, std::string>> messages;
    unsigned long long int id=1;
    Client client(messages, service, id);
    /**
     * В начале имя пользователя должно быть пустым
     */
    ASSERT_TRUE(client.get_username().empty());
    std::string name("NAME");
    client.username_ = name;
    ASSERT_EQ(client.get_username(), name);
}

TEST(Client, init_username){
    std::queue<std::pair<std::string, std::string>> messages;
    unsigned long long int id=1;
    Client client(messages, service, id);
    std::string name = "";
    client.init_username(name);
    /**
     * имя пользователя не должно обновиться, поэтому список новых сообщений будет пуст
     */
    ASSERT_TRUE(messages.empty());
    name = std::string("a", client.max_username);
    client.init_username(name);
    ASSERT_EQ(client.get_username(), name);
    /**
     * А тут уже должно добавиться новое сообщение
     */
    ASSERT_FALSE(messages.empty());
    client.username_="";
    name = std::string("a", client.max_username+1);
    client.init_username(name);
    ASSERT_NE(client.get_username(), name);
    client.username_="";
}

TEST(Client, user_is_ok){
    std::queue<std::pair<std::string, std::string>> messages;
    unsigned long long int id=1;
    Client client(messages, service, id);
    ASSERT_FALSE(client.user_is_ok());
    std::string name("a", client.max_username+1);
    client.init_username(name);
    ASSERT_FALSE(client.user_is_ok());
    name = "";
    client.init_username(name);
    ASSERT_FALSE(client.user_is_ok());
    name = "trololo";
    client.init_username(name);
    ASSERT_TRUE(client.user_is_ok());
}

int main(int argc, char **argv){
    init_test_log();
    logging::add_common_attributes();
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
