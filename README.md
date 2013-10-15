dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

##Установка
###Первичная настройка
1. _Debian - configure grub to optimal resolution_
2. _[*]_ apt-get g++
3. _[* для клиента]_ apt-get biosdecode
4. _[* для клиента]_ apt-get lspci
5. _[* для клиента]_ apt-get dmidecode
6. _install ZeroMQ_ - ./configure && make && make install _(не забыть ldconfig)_
7. apt-get install postgresql-9.1
8. passwd postgres
9. su postgres
    * psql
    * ALTER USER postgres WITH PASSWORD '12345';
    * CREATE USER dcs_server WITH LOGIN REPLICATION ENCRYPTED PASSWORD '12345';
    * CREATE DATABASE dcs_db WITH OWNER dcs_server;

###Сборка
За сборку отвечают файлы compile_server.sh и compile_client.sh

Чтобы всё работало корректно, нужно устанавливать переменную среды MSG_SALT_PATH. Она должна указывать на
[файл с 'солью'](https://github.com/kia84/dcs-system/blob/master/salt), например:
```bash
env MSG_SALT_PATH=/home/user1/projects/dcs/salt ./compile_client.sh
```
Для сервера кроме этого ключа можно указать следующие:

1. CATCH_SIGNAL=[0|1] # устанавливать ли обработчики сигналов
2. USR_LIB="/usr/lib" # директория с библиотеками (используется для libpq)
3. POSTGRESQL_LIB="/usr/include/postgresql" # директория с библиотеками (используется для libpq)

##Запуск
Основной ключ для отладки: -v (be verbose)

Для сервера ключ -t устанавливает размер tread-пула.

После ключей для сервера указывается адрес и порт прослушки, а для клиента адрес сервера соответственно.
Поддерживаются адреса формата ZeroMQ.
Эти параметры необязательны, в случае их отсутствия будут установлены параметры по-умолчанию.
