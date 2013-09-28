
CREATE ROLE device_control_server WITH
	LOGIN
	ENCRYPTED PASSWORD 'dcs';

CREATE DATABASE device_control_database WITH
	OWNER = device_control_server
	ENCODING = 'UTF8';
