//
// Created by matvey on 09.04.23.
//

#ifndef TCPCHATSERVER_SERVER_H
#define TCPCHATSERVER_SERVER_H

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <queue>
#include <fstream>

#include "Client.h"



class Server {
private:
    enum{
        // port_ = 8001,
        first_id_ = 1,
        handle_clients_thread_sleep_ = 3000
    };
    Server();
    Server(const Server &);
    Server& operator=(Server &);
    static unsigned long long last_id_;
    static boost::asio::io_service service_;
    static std::queue<std::pair<std::string, std::string>> messages_; // Очередь с новыми сообщениями (ее изменяет только handle_clients_thread)
    static std::list<std::shared_ptr<Client>>clientsList_; // Список клиентов (общие данные обоих потоков)
    static std::mutex mtx; // mutex для clientsList_
public:
    [[noreturn]] static void accept_thread(int port); // Поток поключения и добавления новых клиентов в clientsList_
    [[noreturn]] static void handle_clients_thread(); // Поток обработки clientsList_, в том числе и удаления
};


#endif //TCPCHATSERVER_SERVER_H
