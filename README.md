# simple-tcp-char-server

[![C++20](https://img.shields.io/badge/C++-20-blue)](https://en.cppreference.com/w/cpp/20)
[![Boost 1.83](https://img.shields.io/badge/Boost-1.83-green)](https://www.boost.org/)
[![OpenSSL 3.0](https://img.shields.io/badge/OpenSSL-3.0-red)](https://www.openssl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

Простой TLS-чат-сервер с собственным CLI-клиентом. Асинхронный (синхронный Boost.Asio), многопоточный (Boost.Thread), с ротацией логов (Boost.Log).

## Возможности

- **TLS 1.3** — шифрование трафика через OpenSSL (чистый C API)
- **CLI-клиент** — отдельный бинарник `SimpleTCPChatClient`, два потока (ввод/вывод)
- **Логирование** — ротация каждые 10 KiB и в полночь, три уровня (trace, debug, info)
- **Таймауты** — автоматическое отключение неактивных клиентов (60 с без ping)
- **Тесты** — 9 тестов (юнит + функциональные TLS socketpair)

## Быстрый старт

```bash
# Зависимости
sudo apt install build-essential cmake libboost-all-dev libssl-dev libgtest-dev

# Сборка
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build -j $(nproc)

# Запуск сервера
./build/SimpleTCPChat -p 8001 -l 2

# Подключение клиентом (в другом терминале)
./build/SimpleTCPChatClient -h 127.0.0.1 -p 8001
```

## Использование

### Сервер (`SimpleTCPChat`)

| Флаг | Описание | По умолчанию |
|------|----------|-------------|
| `-p PORT` | Порт для входящих соединений | 8001 |
| `-l LEVEL` | Уровень логирования: 0 — trace, 1 — debug, 2 — info | 1 |

```bash
./build/SimpleTCPChat -p 4443 -l 2
```

Логи пишутся в `logs/SimpleTCPChat_%N.log`, ротация каждые 10 KiB и в полночь.

### Клиент (`SimpleTCPChatClient`)

| Флаг | Описание | По умолчанию |
|------|----------|-------------|
| `-h HOST` | Адрес сервера | 127.0.0.1 |
| `-p PORT` | Порт сервера | 8001 |

```bash
./build/SimpleTCPChatClient -h 192.168.1.10 -p 4443
```

После подключения нужно ввести имя пользователя — оно станет видимым никнеймом в чате.

## Сборка

### Требования

- CMake 3.22+
- C++20 компилятор (GCC 13+, Clang 16+)
- Boost 1.83+ (system, thread, regex, log)
- OpenSSL 3.0+
- GTest / GMock (только для тестов)

### Команды

```bash
# Релизная сборка
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build -j $(nproc)

# Тесты
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build --target SimpleTCPChat_tests -j $(nproc) && ./build/SimpleTCPChat_tests
```

### Таргеты

| Таргет | Описание |
|--------|----------|
| `SimpleTCPChat` | Сервер |
| `SimpleTCPChatClient` | CLI-клиент |
| `SimpleTCPChat_lib` | Библиотека (линкуется в тесты) |
| `SimpleTCPChat_tests` | Тесты |

## Структура проекта

```
├── CMakeLists.txt          # Сборочный файл
├── LICENSE                 # MIT
├── README.md
├── AGENTS.md               # Инструкции для OpenCode
├── cert/                   # TLS сертификаты
│   ├── server.crt
│   └── server.key
├── header/                 # Заголовочные файлы
│   ├── Client.h
│   ├── ClientCfg.h
│   ├── Server.h
│   ├── init_log.h
│   └── signalHandler.h
├── source/                 # Исходники
│   ├── main.cpp            # Точка входа сервера
│   ├── client_main.cpp     # Точка входа клиента
│   ├── Client.cpp
│   └── Server.cpp
└── tests/                  # Тесты
    ├── main.cpp
    ├── Client_test.h       # Юнит-тесты Client
    └── functional_test.cpp # Функциональные TLS-тесты
```

## Архитектура

- **Server** — статический класс с двумя потоками:
  - `accept_thread` — принимает TLS-подключения, выполняет SSL handshake
  - `handle_clients_thread` — читает сообщения, обрабатывает пинги, удаляет отключившихся
- **Client** — обёртка над TCP-сокетом + OpenSSL `SSL*`, конфигурируется через `ClientCfg`
- **OpenSSL** — чистый C API (не boost::asio::ssl), TLS 1.3, самоподписанный сертификат (CN=localhost)

## Тестирование

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build -j $(nproc) && ./build/SimpleTCPChat_tests
```

9 тестов:
- **Client_Test** (5) — юнит-тесты на username, валидацию, состояние
- **TlsSocketPairTest** (4) — функциональные тесты TLS handshake, передача сообщений, фрагментация, двусторонняя связь

Тесты используют `socketpair` — без внешних процессов, всё в одном процессе.

## Лицензия

MIT. См. [LICENSE](LICENSE).
