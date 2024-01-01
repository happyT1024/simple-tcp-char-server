#pragma once

#include <boost/asio.hpp>

#include <gtest/gtest.h>

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
    /**
     * Конструктор
     * @param messages - ссылка на очередь с сообщениями
     * @param service - ссылка на io_service
     * @param id - индивидуальный идентификатор пользователя
     */
    Client(std::queue<std::pair<std::string, std::string>> & messages, boost::asio::io_service & service,
           unsigned long long & id)
            : sock_(service)
            , messages_(messages)
            , id_(id)
            , already_read_(0)
            , user_exit_(false)
            {}

    /**
     * Обновляет last_ping
     */
    void update_ping();

    boost::asio::ip::tcp::socket & sock();

    [[nodiscard]] unsigned long long get_id() const;

    /**
     * Запускает read_request() и process_request()
     */
    void answer_to_client();

    bool get_user_exit() const;

    std::string get_username();

    /**
     * Делает sock_.write_some(msg)
     * @param msg
     */
    void write(std::string & msg);

    /**
     * Готов ли пользователь получать сообщения
     * @return !username_.empty();
     */
    bool user_is_ok();
private:
    FRIEND_TEST(Client, getUsername);
    FRIEND_TEST(Client, init_username);
    FRIEND_TEST(Client, user_is_ok);

        /**
         * @return True если пользователь не пинговался более чем timed_out_ (60000 мс.)
         */
    bool timed_out() const; // True -> пользователь не пинговался более чем timed_out_ (60000 мс.)

    /**
     * Функция закрывает сокет и помечает user_exit_ как True
     */
    void stop();

    /**
     * Делает sock_.read_some
     */
    void read_request();

    /**
     * Сначала ищет \n в buff_, если не найдет то закончит выполнение
     * Если найдет \n, то записывает сообщение в локальную переменную msg
     * Далее запускает init_username если username(msg) пусто, иначе new_message(msg)
     */
    void process_request();

    /**
     * Устанавливает username_, если оно корректно
     * Если не корректно, то отправит пользователю сообщение об этом
     * @param username - имя пользователя
     */
    void init_username(std::string & username);

    /**
     * Добавляет новое сообщение в очередь messages_
     * @param msg - сообщение от пользователя
     */
    void new_message(std::string & msg);
};