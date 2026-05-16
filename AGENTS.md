# AGENTS.md — simple-tcp-char-server

## Сборка

```bash
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build -j $(nproc)
```

Таргеты: `SimpleTCPChat` (сервер), `SimpleTCPChatClient` (клиент), `SimpleTCPChat_tests` (тесты).

## Тесты

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build --target SimpleTCPChat_tests -j $(nproc) && ./build/SimpleTCPChat_tests
```

- `TESTS=ON` по умолчанию; создаётся библиотека `SimpleTCPChat_lib` и тестовый экзешник.
- Юнит-тесты (`tests/Client_test.h`) тестируют `Client` через публичные методы.

## Запуск

Сервер:
```bash
./build/SimpleTCPChat -p 8001 -l 2
```

Клиент:
```bash
./build/SimpleTCPChatClient -h 127.0.0.1 -p 8001
```

Аргументы сервера: `-p PORT` (default 8001), `-l LEVEL`: 0 — все, 1 — debug, 2 — info (default 1).

Аргументы клиента: `-h HOST` (default 127.0.0.1), `-p PORT` (default 8001).

Логи — в `logs/`, ротация каждые 10 KiB и в полночь.

## Стек

C++20, Boost 1.83+ (asio, thread, log, regex), OpenSSL 3.0, GTest, GMock. Asio — синхронный.

## Архитектура

- `Server` — статический класс; `accept_thread` — приём TLS-подключений, `handle_clients_thread` — обработка сообщений/пингов/удаление клиентов.
- `Client` — обёртка над сокетом + SSL; `ClientCfg` (max_msg=1024, timed_out=60000ms, max_username=12).
- `signalHandler.h` — `std::exit` по SIGTERM/SIGINT.
- OpenSSL: чистый API (не boost::asio::ssl), TLS 1.3. Сертификат — `cert/server.{crt,key}` (самоподписанный, CN=localhost).
