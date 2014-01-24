dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

##Установка
###Первичная настройка
```bash
apt-get install g++        # если его нет
apt-get install biosdecode # для клиента
apt-get install lspci      # для клиента
apt-get install dmidecode  # для клиента
# Ставим ZeroMQ
tar xzvpf zeromq-4.0.1.tar.gz.tar.gz && cd zeromq-4.0.1 && ./configure && make && make install
apt-get install postgresql-9.1 # версия не ниже 9.1
ldconfig           # если нужно
passwd postgres
su postgres
```
Создание и заполнение базы первичными данными описано в файле
[steps](https://github.com/kia84/dcs-system/blob/master/sql/steps).


###Сборка
За сборку отвечают файлы `compile_server.sh` и `compile_client.sh`

Чтобы всё работало корректно, нужно устанавливать переменную среды `MSG_SALT_PATH`. Она должна указывать на
[файл с 'солью'](https://github.com/kia84/dcs-system/blob/master/salt), например:
```bash
env MSG_SALT_PATH=/home/user1/projects/dcs/salt ./compile_client.sh
```
Соль нужно поменять на свою (например командой `ssh-keygen`).

Для сервера кроме этого ключа можно указать следующие:

```bash
CATCH_SIGNAL= 0 || 1                     # устанавливать ли обработчики сигналов
USR_LIB="/usr/lib"                       # директория с библиотеками (используется для libpq)
POSTGRESQL_LIB="/usr/include/postgresql" # директория с библиотеками (используется для libpq)
```

##Запуск
Основной ключ для отладки: `-v` (be verbose)

Для сервера ключ `-t` устанавливает размер tread-пула.

После ключей для сервера указывается адрес и порт прослушки, а для клиента адрес сервера соответственно.
Поддерживаются адреса [формата ZeroMQ](http://api.zeromq.org/4-0:zmq-connect#toc2).
Эти параметры необязательны, в случае их отсутствия будут установлены параметры по-умолчанию.
