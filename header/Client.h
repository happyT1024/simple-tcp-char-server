#pragma once

#include <boost/asio.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <queue>
#include <vector>

#include <ClientCfg.h>

/**
 * Обёртка над TCP-сокетом с TLS (OpenSSL).
 * Хранит буфер входящих данных, username, очередь сообщений.
 * Жизненным циклом управляет Server через shared_ptr.
 */
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
        m_buffer.resize(m_clientCfg.get_m_max_msg());
    }

    Client& operator=(Client other);

    friend void swap(Client & lhs, Client & rhs) noexcept;

    ~Client();

    /// Обновить время последнего пинга
    void update_ping();

    /// Получить ссылку на сокет (для acceptor.accept)
    boost::asio::ip::tcp::socket & sock();

    /// Заменить SSL-сессию (освобождает старую)
    void set_ssl(SSL *ssl);

    [[nodiscard]] unsigned long long get_id() const;

    /// Прочитать и обработать одно входящее сообщение
    void answer_to_client();

    [[nodiscard]] bool get_user_exit() const;

    std::string get_username();

    /// Отправить сообщение с ретраями при WANT_WRITE
    void write(std::string & msg);

    /// Вернул ли клиент username (сообщил своё имя)
    bool user_is_ok();

    /// Установить username и добавить join-сообщение в очередь
    void init_username(std::string & username);

private:
    enum Constants {
        write_max_retries = 5
    };

    [[nodiscard]] bool timed_out() const;

    void stop();

    /// Дождаться готовности сокета к чтению (неблокирующий select)
    bool wait_for_read_ready(int fd);

    /// Обработать ошибку SSL_read; true — можно повторить, false — фатально
    bool handle_ssl_read_error(int ret);

    void read_request();

    /// Извлечь одну строку из буфера (до '\n'), сдвинуть остаток
    std::string extract_line_from_buffer();

    void process_request();

    /// Дождаться готовности сокета к записи (select c таймаутом 1ms)
    bool wait_for_write_ready(int fd);

    /// Добавить сообщение от этого клиента в очередь broadcast
    void new_message(std::string & msg);

private:
    std::unique_ptr<boost::asio::ip::tcp::socket> m_sock;
    SSL *m_ssl;
    ClientCfg m_clientCfg;
    bool m_user_exit;
    unsigned long long m_id;
    std::size_t m_already_read;
    std::vector<char> m_buffer;
    std::string m_username;
    boost::posix_time::ptime m_last_ping;
    std::queue<std::pair<std::string, std::string>> & m_messages;
};
