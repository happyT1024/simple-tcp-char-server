//
// Created by matvey on 08.05.23.
//

#include "settings.h"



void settings::Server::setName(const std::string &name) {
    Server::name = name;
}

void settings::Server::setPort(int port) {
    Server::port = port;
}

void settings::Server::setMaxMsg(int maxMsg) {
    max_msg = maxMsg;
}

void settings::Server::setTimeout(int timeout) {
    Server::timeout = timeout;
}

void settings::Server::setMaxUsername(int maxUsername) {
    max_username = maxUsername;
}


