dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

##Установка
###Первичная настройка
1. _Debian - configure grub to optimal resolution_
2. _[*]_ apt-get g++
3. _install ZeroMQ_ - ./configure && make && make install _(не забыть ldconfig)_
4. apt-get install postgresql-9.1
5. passwd postgres
6. su postgres
    * psql
    * ALTER USER postgres WITH PASSWORD '12345';
    * CREATE USER dcs_server WITH LOGIN REPLICATION ENCRYPTED PASSWORD '12345';
    * CREATE DATABASE dcs_db WITH OWNER dcs_server;

###Сборка
За сборку отвечают файлы compile_server.sh и compile_client.sh
Для сервера можно указать ключ - устанавливать ли обработчики сигналов CATCH_SIGNAL=[0|1]

##Запуск
Основной ключ для отладки: -v (be verbose)

Для сервера ключ -t устанавливает размер tread-пула.

После ключей для сервера указывается адрес и порт прослушки, а для клиента адрес сервера соответственно.
Поддерживаются адреса формата ZeroMQ.
Эти параметры необязательны, в случае их отсутствия будут установлены параметры по-умолчанию.
