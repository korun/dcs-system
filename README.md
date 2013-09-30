dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

##Установка
###Первичная настройка
1. _Debian - configure grub to optimal resolution_
2. _[*]_ apt-get g++
3. _install ZeroMQ_ - ./configure && make && make install
4. apt-get install postgresql-9.1
5. passwd postgres
6. su postgres
    * psql
    * ALTER USER postgres WITH PASSWORD '12345';
    * CREATE USER dcs_server WITH LOGIN REPLICATION ENCRYPTED PASSWORD '12345';
    * CREATE DATABASE dcs_db WITH OWNER dcs_server;

###Сборка
За сборку отвечают файлы compile_server.sh и compile_client.sh
