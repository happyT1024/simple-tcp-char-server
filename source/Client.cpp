#include <Client.h>
#include <boost/log/trivial.hpp>

#include <sys/select.h>

void swap(Client & lhs, Client & rhs) noexcept {
    std::swap(lhs.m_sock, rhs.m_sock);
    std::swap(lhs.m_ssl, rhs.m_ssl);
    std::swap(lhs.m_clientCfg, rhs.m_clientCfg);
    std::swap(lhs.m_user_exit, rhs.m_user_exit);
    std::swap(lhs.m_id, rhs.m_id);
    std::swap(lhs.m_already_read, rhs.m_already_read);
    std::swap(lhs.m_buffer, rhs.m_buffer);
    std::swap(lhs.m_username, rhs.m_username);
    std::swap(lhs.m_last_ping, rhs.m_last_ping);
    std::swap(lhs.m_messages, rhs.m_messages);
}

Client& Client::operator=(Client other) {
    if(this != &other)
        swap(*this, other);
    return *this;
}

Client::~Client() {
    if (m_ssl) SSL_free(m_ssl);
}

void Client::update_ping() {
    m_last_ping = boost::posix_time::microsec_clock::local_time();
}

boost::asio::ip::tcp::socket &Client::sock() {
    return *m_sock;
}

void Client::set_ssl(SSL *ssl) {
    if (m_ssl) SSL_free(m_ssl);
    m_ssl = ssl;
}

unsigned long long Client::get_id() const {
    return m_id;
}

void Client::answer_to_client() {
    try {
        read_request();
        process_request();
    } catch (boost::system::system_error &) {
        BOOST_LOG_TRIVIAL(error)<<"User id:"<<m_id<<" answer_to_client -> system_error";
        stop();
    }
    if (timed_out()) {
        BOOST_LOG_TRIVIAL(info) << "USER id:" << m_id << " - no ping in time";
        stop();
    }
}

bool Client::get_user_exit() const {
    return m_user_exit;
}

bool Client::wait_for_write_ready(int fd) {
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);
    struct timeval tv = {0, 1000};
    return select(fd + 1, NULL, &write_fds, NULL, &tv) > 0;
}

void Client::write(std::string &msg) {
    if (!m_ssl) return;

    int fd = m_sock->native_handle();

    for (int attempt = 0; attempt < write_max_retries; ++attempt) {
        int ret = SSL_write(m_ssl, msg.data(), static_cast<int>(msg.size()));
        if (ret > 0) {
            return;
        }
        int err = SSL_get_error(m_ssl, ret);
        if (err == SSL_ERROR_WANT_WRITE) {
            BOOST_LOG_TRIVIAL(trace) << "User id:" << m_id << " SSL_write WANT_WRITE attempt " << attempt;
            wait_for_write_ready(fd);
            continue;
        }
        if (err == SSL_ERROR_WANT_READ) {
            return;
        }
        BOOST_LOG_TRIVIAL(info)<<"User id:"<<m_id<<" close connect, msg not send, err="<<err;
        stop();
        return;
    }
    BOOST_LOG_TRIVIAL(error)<<"User id:"<<m_id<<" write failed after " << write_max_retries << " attempts";
    stop();
}

bool Client::timed_out() const {
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long long ms = (now - m_last_ping).total_milliseconds();
    return ms > m_clientCfg.get_m_timed_out();
}

void Client::stop() {
    BOOST_LOG_TRIVIAL(info)<<"Close connection with User id:"<<m_id;
    if (m_ssl) {
        SSL_shutdown(m_ssl);
    }
    boost::system::error_code err;
    if(m_sock->close(err) || err) {
        BOOST_LOG_TRIVIAL(error)<<"Close connection fail. Error:"<<err.message();
    }
    m_user_exit = true;
}

/// Дождаться готовности сокета к чтению (неблокирующий select, мгновенный возврат)
bool Client::wait_for_read_ready(int fd) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    struct timeval tv = {0, 0};
    return select(fd + 1, &read_fds, NULL, NULL, &tv) > 0;
}

/// Обработать ошибку SSL_read.
/// Возвращает true, если ошибка нефатальная и чтение можно повторить.
bool Client::handle_ssl_read_error(int ret) {
    int err = SSL_get_error(m_ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        BOOST_LOG_TRIVIAL(trace) << "User id:" << m_id << " SSL_read WANT_"
                                 << (err == SSL_ERROR_WANT_READ ? "READ" : "WRITE");
        return true;
    }
    if (err == SSL_ERROR_SYSCALL) {
        BOOST_LOG_TRIVIAL(debug) << "User id:" << m_id << " SSL_ERROR_SYSCALL errno=" << errno;
        return false;
    }
    BOOST_LOG_TRIVIAL(error) << "User id:" << m_id << " SSL error: " << err;
    stop();
    return false;
}

/// Неблокирующее чтение: select + SSL_read.
/// Если данных нет, возвращается сразу без ожидания.
void Client::read_request() {
    if (!m_ssl || !m_sock->is_open()) return;

    int fd = m_sock->native_handle();
    if (!wait_for_read_ready(fd)) return;

    std::size_t max_read = m_clientCfg.get_m_max_msg() - m_already_read;
    int ret = SSL_read(m_ssl, m_buffer.data() + m_already_read,
                       static_cast<int>(max_read));
    if (ret > 0) {
        BOOST_LOG_TRIVIAL(trace) << "User id:" << m_id << " read " << ret << " bytes";
        m_already_read += static_cast<std::size_t>(ret);
        return;
    }
    if (!handle_ssl_read_error(ret)) {
        stop();
    }
}

void Client::init_username(std::string &username) {
    if(username.empty()){
        return;
    }
    if (username.size() > m_clientCfg.get_m_max_username()) {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << m_id << " send very big username";
        std::string msg("Username cannot be longer than " + std::to_string(m_clientCfg.get_m_max_username()) + " characters\nTry again: ");
        write(msg);
        return;
    }
    m_username = username;
    m_messages.emplace("Server",username + " join\n");
}

void Client::new_message(std::string &msg) {
    m_messages.emplace(m_username, msg);
}

/// Ищет '\n' в буфере, извлекает строку (без '\n'), сдвигает остаток в начало.
std::string Client::extract_line_from_buffer() {
    char* buf = m_buffer.data();
    char* end = buf + m_already_read;
    char* it = std::find(buf, end, '\n');
    if (it >= end) return {};

    std::string line(buf, static_cast<std::size_t>(it - buf));
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::size_t consumed = static_cast<std::size_t>(it - buf + 1);
    std::copy(it + 1, buf + m_clientCfg.get_m_max_msg(), buf);
    m_already_read -= consumed;
    return line;
}

void Client::process_request() {
    std::string msg = extract_line_from_buffer();
    if (msg.empty()) {
        return;
    }

    update_ping();

    if (m_username.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << m_id << " send username: " << msg;
        init_username(msg);
    } else {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << m_id << " send message: " << msg;
        msg.push_back('\n');
        new_message(msg);
    }
}

bool Client::user_is_ok() {
    return !m_username.empty();
}

std::string Client::get_username() {
    return m_username;
}
