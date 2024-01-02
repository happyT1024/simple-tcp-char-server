#include <Client.h>
#include <boost/log/trivial.hpp>

void swap(Client & lhs, Client & rhs) noexcept {
    std::swap(lhs.m_sock, rhs.m_sock);
    std::swap(lhs.m_clientCfg, rhs.m_clientCfg);
    std::swap(lhs.m_user_exit, rhs.m_user_exit);
    std::swap(lhs.m_id, rhs.m_id);
    std::swap(lhs.m_already_read, rhs.m_already_read);
    std::swap(lhs.m_buff, rhs.m_buff);
    std::swap(lhs.m_username, rhs.m_username);
    std::swap(lhs.m_last_ping, rhs.m_last_ping);
    std::swap(lhs.m_messages, rhs.m_messages);
}

Client& Client::operator=(Client other) {
    if(this != &other)
        swap(*this, other);
    return *this;
}

void Client::update_ping() {
    m_last_ping = boost::posix_time::microsec_clock::local_time();
}

boost::asio::ip::tcp::socket &Client::sock() {
    return *m_sock;
}

unsigned long long Client::get_id() const {
    return m_id;
}

void Client::answer_to_client() {
    try {
        read_request();
        process_request();
    } catch (boost::system::system_error &) {
        /**
         * это исключение никогда не вылезало
         */
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

void Client::write(std::string &msg) {
    try {
        m_sock->write_some(boost::asio::buffer(msg));
    }catch(boost::wrapexcept<boost::system::system_error> & e){
        /**
         * может случиться когда пользователь закрыл соединение, но его еще не удалили, стандартная ситуация
         */
        BOOST_LOG_TRIVIAL(info)<<"User id:"<<m_id<<" close connect, msg not send";
        stop();
    }
}

bool Client::timed_out() const {
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long long ms = (now - m_last_ping).total_milliseconds();
    return ms > m_clientCfg.get_m_timed_out();
}

void Client::stop() {
    BOOST_LOG_TRIVIAL(info)<<"Close connection with User id:"<<m_id;
    boost::system::error_code err;
    if(m_sock->close(err) || err) {
        BOOST_LOG_TRIVIAL(error)<<"Close connection fail. Error:"<<err.message();
    }
    m_user_exit = true;
}

void Client::read_request() {
    if (m_sock->available())
        m_already_read += m_sock->read_some(
                boost::asio::buffer(*m_buff + m_already_read, m_clientCfg.get_m_max_msg() - m_already_read));
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

void Client::process_request() {
    bool found_enter = std::find(*m_buff, *m_buff + m_already_read, '\n') < *m_buff + m_already_read;
    if (!found_enter) {
        return;
    }

    update_ping();
    size_t pos = std::find(*m_buff, *m_buff + m_already_read, '\n') - *m_buff;
    std::string msg(*m_buff, pos - 1);
    std::copy(*m_buff + m_already_read, *m_buff + m_clientCfg.get_m_max_msg(), *m_buff);
    m_already_read -= pos + 1;

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
