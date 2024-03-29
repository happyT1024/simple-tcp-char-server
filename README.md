<h1 align="center">simple-tcp-char-server</h1>

## Описание

Простой сервер для чата на TCP стеке

Клиент - telnet

Пример работы программы:

![Пример работы](https://github.com/happyT1024/simple-tcp-char-server/blob/main/photos/clients.png)


## Инструкция для работы с сервером

### Запуск

```-p``` - порт

```-l``` - тип логирования, где 0 - все логи (не рекомендуется), 1 - debug, 2 - info в папку logs

```bash
./SimpleTCPChat -p 8001 -l 2 &
```

### Подключение

Для подключения достаточно воспользоваться утилитой telnet, например:

```bash
telnet 127.0.0.1 8001
```

### Выключение

Достаточно воспользоваться утилитой pkill, например:

```bash
pkill SimpleTCPC
```

##



## Быстрый гайд по коду или как тут все работает

Важно: в примерах представлен упрощенный код, в проекте может добавиться ```try catch```, логирование или что-то еще, но тем не менее, чтобы быстро разобраться в 90% кода, достаточно прочитать этот раздел.

### Краткая справка

Взаимодействие с сетью происходит при помощи библиотеки boost.asio. Все методы - синхронные. Вдохновила следующая статья: https://habr.com/ru/articles/195794/

Потоки - boost.threads

Логирование - boost.log

Контейнеры - почти всегда stl

Тесты - gtest

### Server

```Server``` - класс, основан на 2 параллельных потоках — accept_thread и handle_clients_thread

Пример работы с классом:

```c++
int port=8001;
boost::thread_group threads;
threads.create_thread(boost::bind( Server::accept_thread, port));
threads.create_thread(Server::handle_clients_thread);
threads.join_all();
```

Потоки взаимодействуют со следующими полями класса:

```c++
static unsigned long long last_id_; // Для id пользователя
static boost::asio::io_service service_;
static std::queue<std::pair<std::string, std::string>> messages_; // Очередь с новыми сообщениями (ее изменяет только handle_clients_thread)
static std::list<std::shared_ptr<Client>>clientsList_; // Список клиентов (общие данные обоих потоков)
static std::mutex mtx; // mutex для clientsList_
```

#### accept_thread

В потоке accept_thread создаются новые пользователи:

```c++
std::shared_ptr<Client> client(new Client(messages_, service_, last_id_));
last_id_++;
acceptor.accept(client->sock());
client->update_ping();
```

Также в этом потоке новые пользователи добавляются в clientsList:

```c++
clientsList_.push_back(client);
```

#### handle_clients_thread

Если упростить, то в потоке handle_clients_thread происходит следующее:
читается потенциальное сообщение пользователя.

```c++
for (const auto &x: clientsList_) {
    x->answer_to_client();
}
```

Каждый экземпляр класса Client сам добавит сообщение в очередь messages_

После этого из clientsList удалаются пользователи, которые слишком долго не пинговались, или при их обработке произошла
ошибка.

```c++
clientsList_.erase(
    std::remove_if(
        clientsList_.begin(),
        clientsList_.end(),
        [&](const std::shared_ptr<Client> &Client) -> bool {
            if (Client->get_user_exit()) {
                messages_.emplace("Server", Client->get_username() + " leave the chat");
            };
            return Client->get_user_exit();
        }),
    clientsList_.end());
```

В конце каждому пользователю отправляются новые сообщения из очереди.

```c++
while (!messages_.empty()) {
    std::string msg = messages_.front().first + ": " + messages_.front().second;
    for (const auto &x: clientsList_) {
        if (x->user_is_ok())
            x->write(msg);
    }
    messages_.pop();
}
```

### Client

В классе ```Client``` много методов, но его главный метод представлен тут:

```c++
void Client::answer_to_client() {
    read_request(); // читает новое сообщение в буффер 
    process_request(); // обрабатывает это сообщение (если его нет, то сделает return)
}
```

```void process_request()``` поступает из следующей логики - если имя пользователя еще пустое, значит его сообщение - его имя. Иначе его сообщение надо добавить в ```messages```.

В принципе, это все что надо знать о классе ```Client```.

## Компиляция

Лучше всего запускать бинарник (отправлю), но если очень хочется, то можно и скомпилировать, но будьте готовы к установке библиотек, так как я не добавлял их как зависимости к Git.

Для компиляции нужен Boost 1.74.0, GTest и CMake.

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cd ..
cmake --build build --target SimpleTCPChat_bin -j 8
mv build/src/SimpleTCPChat_bin SimpleTCPChat
```

## Запуск тестов

Не сильно отличается от компиляции.

Тестов мало, зато они есть :)

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cd ..
cmake --build build --target tests -j 8
./build/tests/tests
```
