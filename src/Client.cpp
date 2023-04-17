//
// Created by matvey on 05.04.23.
//

#include "Client.h"

void Client::update_ping() {
    last_ping = boost::posix_time::microsec_clock::local_time();
}

boost::asio::ip::tcp::socket &Client::sock() {
    return sock_;
}

unsigned long long Client::get_id() const {
    return id_;
}

void Client::answer_to_client() {
    try {
        read_request();
        process_request();
    } catch (boost::system::system_error &) {
        /**
         * это исключение никогда не вылезало
         */
        BOOST_LOG_TRIVIAL(error)<<"User id:"<<id_<<" answer_to_client -> system_error";
        stop();
    }
    if (timed_out()) {
        BOOST_LOG_TRIVIAL(info) << "USER id:" << id_ << " - no ping in time";
        stop();
    }
}

bool Client::get_user_exit() const {
    return user_exit_;
}

void Client::write(std::string &msg) {
    try {
        sock_.write_some(boost::asio::buffer(msg));
    }catch(boost::wrapexcept<boost::system::system_error> e){
        /**
         * может случиться когда пользователь закрыл соединение, но его еще не удалили, стандартная ситуация
         */
        BOOST_LOG_TRIVIAL(info)<<"User id:"<<id_<<" close connect, msg not send";
        stop();
    }
}

bool Client::timed_out() const {
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long long ms = (now - last_ping).total_milliseconds();
    return ms > timed_out_;
}

void Client::stop() {
    BOOST_LOG_TRIVIAL(info)<<"Close connection with User id:"<<id_;
    boost::system::error_code err;
    sock_.close(err);
    user_exit_ = true;
}

void Client::read_request() {
    if (sock_.available())
        already_read_ += sock_.read_some(
                boost::asio::buffer(buff_ + already_read_, max_msg - already_read_));
}

void Client::init_username(std::string &username) {
    if (username.size() > max_username) {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << id_ << " send very big username";
        std::string msg("Username cannot be longer than " + std::to_string(max_username) + " characters\nTry again: ");
        write(msg);
        return;
    }
    username_ = username;
    messages_.emplace("Server",username + " join\n");
}

void Client::new_message(std::string &msg) {
    messages_.emplace(username_, msg);
}

void Client::process_request() {
    bool found_enter = std::find(buff_, buff_ + already_read_, '\n') < buff_ + already_read_;
    if (!found_enter) {
        return;
    }

    update_ping();
    size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
    std::string msg(buff_, pos - 1);
    std::copy(buff_ + already_read_, buff_ + max_msg, buff_);
    already_read_ -= pos + 1;

    if (username_.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << id_ << " send username: " << msg;
        init_username(msg);
    } else {
        BOOST_LOG_TRIVIAL(debug) << "USER id:" << id_ << " send message: " << msg;
        msg.push_back('\n');
        new_message(msg);
    }
}

bool Client::user_is_ok() {
    return !username_.empty();
}

std::string Client::get_username() {
    return username_;
}
