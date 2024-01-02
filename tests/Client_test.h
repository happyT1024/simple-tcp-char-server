#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Client.h>

class Client_Test : public testing::Test {
protected:
	void SetUp() override {
		client = Client(messages, service, id);
		name = "TEST_NAME";
	}

	// void TearDown() override {}

	boost::asio::io_service service;
	std::queue<std::pair<std::string, std::string>> messages;
	unsigned long long int id=1;
	Client client;
	std::string name;
};

TEST_F(Client_Test, getUsername){
	ASSERT_TRUE(client.get_username().empty());
}
