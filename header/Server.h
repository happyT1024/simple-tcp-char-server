#pragma once

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/log/sinks/text_file_backend.hpp>

#include <queue>

#include <Client.h>

class Server {
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
private:
    enum{
        first_id_ = 1,
        handle_clients_thread_sleep_ = 3000
    };
    Server();
    Server(const Server &);
    Server& operator=(Server &);
    static unsigned long long m_last_id;
    static boost::asio::io_service m_service;
    static std::queue<std::pair<std::string, std::string>> m_messages; // Очередь с новыми сообщениями (ее изменяет только handle_clients_thread)
    static std::list<std::shared_ptr<Client>>m_clientsList; // Список клиентов (общие данные обоих потоков)
    static std::mutex m_mtx; // mutex для clientsList_
};
