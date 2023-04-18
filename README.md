<h1 align="center">simple-tcp-char-server</h1>

## Описание

Простой сервер для TCP чата

Клиент - telnet

## Инструкция

### Запуск

-p: порт

-l: тип логирования, где 0 - все логи (не рекомендуется), 1 - debug, 2 - info в  папку logs

```
./SimpleTCPChat -p 8001 -l 2 &
```

### Подключение
Для подключения достаточно воспользоваться утилитой telnet, например:
```
telnet 127.0.0.1 8001
```


### Выключение
Достаточно воспользоваться утилитой pkill, например:
```
pkill SimpleTCPC
```

## Компиляция
Лучше всего запускать бинарник (отправлю), но если очень хочется, то можно и скомпилировать, но будьте готовы к установке библиотек, так как я не добавлял их как зависимости к Git.

Для компиляции нужен Boost 1.74.0, GTest и CMake
```
clone 
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cd ..
cmake --build build --target SimpleTCPChat_bin -j 8
mv build/src/SimpleTCPChat_bin SimpleTCPChat
```
