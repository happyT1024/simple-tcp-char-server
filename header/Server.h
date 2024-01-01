#pragma once

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/log/sinks/text_file_backend.hpp>

#include <queue>

#include <Client.h>

class Server {
private:
    enum{
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
    /**
     * Поток поключения и добавления новых клиентов в clientsList_
     * @param port - порт программы, по умолчанию 8001
     */
    [[noreturn]] static void accept_thread(int port = 8001);

    /**
     * Поток обработки clientsList_, в том числе и удаления
     */
    [[noreturn]] static void handle_clients_thread();
};
