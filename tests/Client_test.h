#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Client.h>

class Client_Test : public testing::Test {
protected:
	Client_Test()
	: service(boost::asio::io_service{})
	, ssl_ctx(SSL_CTX_new(TLS_method()))
	, client(Client(messages, service, id, ssl_ctx))
	{}

	~Client_Test() override {
		if (ssl_ctx) SSL_CTX_free(ssl_ctx);
	}

	void SetUp() override {
		name = "TEST_NAME";
	}

	void TearDown() override {}

	boost::asio::io_service service;
	SSL_CTX *ssl_ctx;
	std::queue<std::pair<std::string, std::string>> messages;
	unsigned long long int id=1;
	Client client;
	std::string name;
};

TEST_F(Client_Test, getUsername){
	ASSERT_TRUE(client.get_username().empty());
}

TEST_F(Client_Test, init_username){
	client.init_username(name);
	ASSERT_EQ(client.get_username(), name);
}

TEST_F(Client_Test, user_is_ok_empty){
	ASSERT_FALSE(client.user_is_ok());
	client.init_username(name);
	ASSERT_TRUE(client.user_is_ok());
}

TEST_F(Client_Test, init_username_too_long){
	std::string long_name(100, 'A');
	client.init_username(long_name);
	ASSERT_TRUE(client.get_username().empty());
}

TEST_F(Client_Test, init_username_empty){
	client.init_username(name);
	ASSERT_EQ(client.get_username(), name);
}
