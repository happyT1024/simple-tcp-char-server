# AGENTS.md — simple-tcp-char-server

## Сборка

```bash
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build --target SimpleTCPChat -j $(nproc)
```

Таргет бинарника: `SimpleTCPChat` (не `SimpleTCPChat_bin`, несмотря на README).

## Тесты

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build --target SimpleTCPChat_tests -j $(nproc) && ./build/SimpleTCPChat_tests
```

- `TESTS=ON` по умолчанию; создаётся библиотека `SimpleTCPChat_lib` и тестовый экзешник.
- Тесты используют `FRIEND_TEST` для доступа к private-полям `Client`.

## Запуск

```bash
./SimpleTCPChat -p 8001 -l 2
```

Аргументы: `-p PORT` (default 8001), `-l LEVEL`: 0 — все, 1 — debug, 2 — info (default 1).

Логи — в `logs/`, ротация каждые 10 KiB и в полночь.

## Стек

C++20, Boost 1.74+ (asio, thread, log, regex), GTest, GMock. Asio — синхронный.

## Архитектура

- `Server` — статический класс; `accept_thread` — приём подключений, `handle_clients_thread` — обработка сообщений/пингов/удаление клиентов.
- `Client` — обёртка над сокетом; `ClientCfg` (max_msg=1024, timed_out=60000ms, max_username=12).
- `signalHandler.h` — `std::exit` по SIGTERM/SIGINT.
