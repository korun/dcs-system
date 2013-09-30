dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

Установка
==========

0. Debian - configure grub to optimal resolution
1. [*] apt-get g++
2. install ZeroMQ - ./configure && make && make install
3.1 apt-get install postgresql-9.1
3.2   passwd postgres
3.3   su postgres
3.3.1       psql
3.3.2       ALTER USER postgres WITH PASSWORD '12345';
3.3.3       CREATE USER dcs_server WITH LOGIN REPLICATION ENCRYPTED PASSWORD '12345';
3.3.4       CREATE DATABASE dcs_db WITH OWNER dcs_server;
