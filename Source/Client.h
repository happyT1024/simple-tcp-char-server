//
// Created by matvey on 05.04.23.
//

#ifndef TCPCHATSERVER_CLIENT_H
#define TCPCHATSERVER_CLIENT_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/trivial.hpp>

#include <queue>


class Client {
private:
    boost::asio::ip::tcp::socket sock_;
    enum {
        max_msg = 1024,
        timed_out_ = 60000, // 1 минута
        max_username = 12
    };
    bool user_exit_; // True если вызвали stop(), при этом пользователь еще не удален из списка пользователей
    unsigned long long id_; // используется только для отладки
    int already_read_;
    char buff_[max_msg];
    std::string username_;
    boost::posix_time::ptime last_ping; // время последнего запроса от пользователя
    std::queue<std::pair<std::string, std::string>> & messages_; // всегда ссылается на messages_ в Server
public:
    Client(std::queue<std::pair<std::string, std::string>> & messages, boost::asio::io_service & service,
           unsigned long long & id)
            : sock_(service)
            , messages_(messages)
            , id_(id)
            , already_read_(0)
            , user_exit_(false)
            {}

    void update_ping();
    boost::asio::ip::tcp::socket & sock();
    [[nodiscard]] unsigned long long getUserID() const;
    void answer_to_client(); // запускает read_request() process_request()
    bool get_user_exit() const;
    std::string get_username();
    void write(std::string & msg); // sock_.write_some
    bool user_is_ok(); // True -> пользователь готов получать сообщения
private:
    bool timed_out() const; // True -> пользователь не пинговался более чем timed_out_ (60000 мс.)
    void stop(); // закрытие сокета и user_exit_=True
    void read_request();
    void process_request(); // тут вся логика обработки сообщений, вызывает init_username или new_message
    void init_username(std::string & username); // устанавливает username_, если оно корректно
    void new_message(std::string & msg); // добавляет новое сообщение в очередь messages_
};

#endif //TCPCHATSERVER_CLIENT_H
