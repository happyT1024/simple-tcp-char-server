#pragma once

#include <boost/asio.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <gtest/gtest.h>

#include <queue>

#include <ClientCfg.h>

class Client {
public:
    Client(std::queue<std::pair<std::string, std::string>> & messages, boost::asio::io_service & service,
           unsigned long long & id, SSL_CTX *ssl_ctx, ClientCfg clientCfg = ClientCfg{})
            : m_messages(messages)
            , m_id(id)
            , m_already_read(0)
            , m_user_exit(false)
            , m_clientCfg(clientCfg)
    {
        m_sock = std::make_unique<boost::asio::ip::tcp::socket>(
            boost::asio::ip::tcp::socket(service));
        m_ssl = SSL_new(ssl_ctx);
        m_buff = std::make_unique<char*>(new char[m_clientCfg.get_m_max_msg()]);
    }

    Client& operator=(Client other);

    friend void swap(Client & lhs, Client & rhs) noexcept;

    ~Client();

    void update_ping();

    boost::asio::ip::tcp::socket & sock();

    void set_ssl(SSL *ssl);

    [[nodiscard]] unsigned long long get_id() const;

    void answer_to_client();

    [[nodiscard]] bool get_user_exit() const;

    std::string get_username();

    void write(std::string & msg);

    bool user_is_ok();

    void init_username(std::string & username);

private:
    [[nodiscard]] bool timed_out() const;

    void stop();

    void read_request();

    void process_request();

    void new_message(std::string & msg);

private:
    std::unique_ptr<boost::asio::ip::tcp::socket> m_sock;
    SSL *m_ssl;
    ClientCfg m_clientCfg;
    bool m_user_exit;
    unsigned long long m_id;
    std::size_t m_already_read;
    std::unique_ptr<char*> m_buff;
    std::string m_username;
    boost::posix_time::ptime m_last_ping;
    std::queue<std::pair<std::string, std::string>> & m_messages;

};
