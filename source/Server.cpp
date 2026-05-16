#include <Server.h>
#include <boost/log/trivial.hpp>

unsigned long long Server::m_last_id = first_id_;
SSL_CTX *Server::m_ssl_ctx = nullptr;
std::queue<std::pair<std::string, std::string>> Server::m_messages = std::queue<std::pair<std::string, std::string>>();
boost::asio::io_service Server::m_service;
std::list<std::shared_ptr<Client>> Server::m_clientsList;
std::mutex Server::m_mtx;

/// Инициализировать SSL-контекст: загрузить сертификат и ключ, проверить их соответствие.
/// При ошибке — exit(1).
void Server::init_ssl_ctx() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    m_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!m_ssl_ctx) {
        BOOST_LOG_TRIVIAL(fatal) << "Unable to create SSL context";
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    SSL_CTX_set_ecdh_auto(m_ssl_ctx, 1);

    if (SSL_CTX_use_certificate_file(m_ssl_ctx, "cert/server.crt", SSL_FILETYPE_PEM) <= 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Unable to load certificate file";
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "cert/server.key", SSL_FILETYPE_PEM) <= 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Unable to load private key file";
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    if (!SSL_CTX_check_private_key(m_ssl_ctx)) {
        BOOST_LOG_TRIVIAL(fatal) << "Private key does not match the certificate";
        exit(1);
    }
}

/// Создать acceptor на указанном порту (IPv4).
boost::asio::ip::tcp::acceptor Server::create_acceptor(int port) {
    return boost::asio::ip::tcp::acceptor(m_service,
                                          boost::asio::ip::tcp::endpoint(
                                                  boost::asio::ip::tcp::v4(), port));
}

/// Дождаться нового TCP-подключения и создать Client.
std::shared_ptr<Client> Server::accept_connection(boost::asio::ip::tcp::acceptor & acceptor) {
    auto client = std::make_shared<Client>(m_messages, m_service, m_last_id, m_ssl_ctx);
    m_last_id++;
    acceptor.accept(client->sock());
    client->update_ping();
    return client;
}

/// Выполнить TLS handshake. В случае ошибки — залогировать и вернуть false.
bool Server::perform_ssl_handshake(const std::shared_ptr<Client> & client) {
    SSL *ssl = SSL_new(m_ssl_ctx);
    SSL_set_fd(ssl, client->sock().native_handle());
    if (SSL_accept(ssl) <= 0) {
        BOOST_LOG_TRIVIAL(error) << "SSL handshake failed for User id:" << client->get_id()
                                 << " err=" << ERR_error_string(ERR_get_error(), nullptr);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return false;
    }
    BOOST_LOG_TRIVIAL(debug) << "SSL handshake successful for User id:" << client->get_id();
    client->sock().non_blocking(true);
    client->set_ssl(ssl);
    return true;
}

/// Отправить приветственное сообщение новому клиенту.
void Server::send_greetings(const std::shared_ptr<Client> & client) {
    std::string greetings = "==================================\n"
                            " Welcome to Simple TCP Chat (TLS)\n"
                            "==================================\n"
                            "What is your name : \n";
    client->write(greetings);
    BOOST_LOG_TRIVIAL(trace) << "greetings send";
}

/// Добавить клиента в общий список (под мьютексом).
void Server::register_client(const std::shared_ptr<Client> & client) {
    std::lock_guard<std::mutex> lock(m_mtx);
    BOOST_LOG_TRIVIAL(trace) << "m_mtx lock tread accept_thread";

    m_clientsList.push_back(client);
    BOOST_LOG_TRIVIAL(debug) << "USER id:" << client->get_id() << " add to ClientsList";
    BOOST_LOG_TRIVIAL(debug) << "ClientsList size = " << m_clientsList.size();

    BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread accept_thread";
}

/// Бесконечный цикл приёма новых подключений: accept → handshake → greetings → регистрация.
void Server::accept_thread(int port) {
    try {
        init_ssl_ctx();
        BOOST_LOG_TRIVIAL(debug) << "Thread accept_thread enable";

        auto acceptor = create_acceptor(port);
        while (true) {
            auto client = accept_connection(acceptor);
            if (!perform_ssl_handshake(client)) continue;

            BOOST_LOG_TRIVIAL(info) << "USER id:" << client->get_id() << " connect to server";
            send_greetings(client);
            register_client(client);

            boost::this_thread::sleep(boost::posix_time::millisec(1));
        }
    }catch(const std::exception & e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR accept_thread: " << e.what();
        exit(1);
    }
}

void Server::process_all_clients() {
    for (const auto &x: m_clientsList) {
        x->answer_to_client();
    }
}

void Server::remove_disconnected_clients() {
    m_clientsList.erase(
            std::remove_if(
                    m_clientsList.begin(),
                    m_clientsList.end(),
                    [&](const std::shared_ptr<Client> &c) -> bool {
                        if (c->get_user_exit()) {
                            BOOST_LOG_TRIVIAL(info) << "USER id:" << c->get_id() << " delete";
                            m_messages.emplace("Server", c->get_username() + " leave the chat");
                        };
                        return c->get_user_exit();
                    }),
            m_clientsList.end());
}

void Server::broadcast_messages() {
    while (!m_messages.empty()) {
        std::string msg = m_messages.front().first + ": " + m_messages.front().second;
        for (const auto &x: m_clientsList) {
            if (x->user_is_ok())
                x->write(msg);
        }
        m_messages.pop();
    }
}

/// Бесконечный цикл обработки клиентов: обработка сообщений, удаление отключившихся, broadcast.
void Server::handle_clients_thread() {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Thread handle_clients_thread enable";
        while (true) {
            boost::this_thread::sleep(boost::posix_time::millisec(1));

            {
                std::lock_guard<std::mutex> lock(m_mtx);
                BOOST_LOG_TRIVIAL(trace) << "m_mtx lock tread handle_clients_thread";
                BOOST_LOG_TRIVIAL(trace) << "Processing " << m_clientsList.size() << " clients";

                if (m_clientsList.empty()) {
                    BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread handle_clients_thread";
                    BOOST_LOG_TRIVIAL(debug) << "m_clientsList empty -> trace handle_clients_thread sleep "
                                             << handle_clients_thread_sleep_ << "ms";
                    boost::this_thread::sleep(boost::posix_time::millisec((long long) handle_clients_thread_sleep_));
                    continue;
                }

                process_all_clients();
                remove_disconnected_clients();
                broadcast_messages();

                BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread handle_clients_thread";
            }
        }
    }catch(const std::exception & e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR handle_clients_thread: " << e.what();
        exit(2);
    }
}
