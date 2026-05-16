# Changelog

## [0.2.0] — 2026-05-16

### Added
- TLS 1.3 шифрование трафика (чистый OpenSSL API)
- `SimpleTCPChatClient` — собственный CLI-клиент (два потока: ввод/вывод)
- `cert/server.{crt,key}` — самоподписанный TLS-сертификат
- Функциональные TLS-тесты на socketpair (4 теста)
- AGENTS.md, LICENSE (MIT), CHANGELOG.md

### Changed
- `read_request` — неблокирующий режим через `SSL_get_error`
- CMakeLists.txt — разделение на lib, server, client

### Fixed
- `process_request` — корректная обработка `\r\n` (telnet) и `\n` (наш клиент)

## [0.1.0] — 2025

### Added
- TCP-чат-сервер на Boost.Asio (синхронный)
- Два потока: accept и handle clients
- Логирование с ротацией (Boost.Log)
- Таймауты и ping
- Юнит-тесты Client (GTest)
