#pragma once

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/log/sinks/text_file_backend.hpp>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <queue>

#include <Client.h>

/**
 * Статический класс-диспетчер TCP/TLS-сервера.
 *
 * accept_thread      — бесконечный цикл: accept → SSL handshake → регистрация
 * handle_clients_thread — бесконечный цикл: обработка → удаление → broadcast
 */
class Server {
public:
    [[noreturn]] static void accept_thread(int port = 8001);

    [[noreturn]] static void handle_clients_thread();
private:
    enum{
        first_id_ = 1,
        handle_clients_thread_sleep_ = 3000
    };
    Server();
    Server(const Server &);
    Server& operator=(Server &);
    static void init_ssl_ctx();
    static boost::asio::ip::tcp::acceptor create_acceptor(int port);
    static std::shared_ptr<Client> accept_connection(boost::asio::ip::tcp::acceptor & acceptor);
    static bool perform_ssl_handshake(const std::shared_ptr<Client> & client);
    static void send_greetings(const std::shared_ptr<Client> & client);
    static void register_client(const std::shared_ptr<Client> & client);
    static void process_all_clients();
    static void remove_disconnected_clients();
    static void broadcast_messages();
    static unsigned long long m_last_id;
    static SSL_CTX *m_ssl_ctx;
    static boost::asio::io_service m_service;
    static std::queue<std::pair<std::string, std::string>> m_messages;
    static std::list<std::shared_ptr<Client>>m_clientsList;
    static std::mutex m_mtx;
};
