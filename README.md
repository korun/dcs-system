dcs-system
==========

Система сбора и контроля аппаратных данных с сетевых устройств.

Установка
==========

0. Debian - configure grub to optimal resolution
1. [*] apt-get g++
2. install ZeroMQ - ./configure && make && make install
3. apt-get install postgresql-9.1
4. passwd postgres
5. su postgres
       psql
       ALTER USER postgres WITH PASSWORD '12345';
       CREATE USER dcs_server WITH LOGIN REPLICATION ENCRYPTED PASSWORD '12345';
       CREATE DATABASE dcs_db WITH OWNER dcs_server;
