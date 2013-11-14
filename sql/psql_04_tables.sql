BEGIN;
CREATE TABLE IF NOT EXISTS computers(
	id		SERIAL	PRIMARY KEY, -- SERIAL
--	db_id		integer,	    -- Local cache only
--	md5sum		varchar(32),	-- Local cache only
	domain_name	varchar(256) NOT NULL CHECK (trim(both from domain_name) NOT LIKE ''),
	created_at	timestamp NOT NULL DEFAULT current_timestamp,
	accepted_at	timestamp CHECK (accepted_at >= created_at),
	deleted_at	timestamp CHECK (deleted_at  >= created_at)
);

CREATE TABLE IF NOT EXISTS details(
	id		SERIAL	PRIMARY KEY, -- SERIAL
	vendor_id	integer,
	device_id	integer,
	subsystem_id	integer,
	class_code	integer,
	revision	integer,
	serial		varchar(128),
	created_at	timestamp NOT NULL DEFAULT current_timestamp,
	accepted_at	timestamp CHECK (accepted_at >= created_at),
	deleted_at	timestamp CHECK (deleted_at  >= created_at),
	params		text
);

CREATE TABLE IF NOT EXISTS computers_details(
	id		    SERIAL	PRIMARY KEY, --SERIAL
	computer_id	integer REFERENCES computers(id),
	detail_id	integer REFERENCES details(id),
	state		integer,
	created_at	timestamp NOT NULL DEFAULT current_timestamp,
	accepted_at	timestamp CHECK (accepted_at >= created_at),
	deleted_at	timestamp CHECK (deleted_at  >= created_at),
	allow_del_at timestamp CHECK (allow_del_at >= deleted_at),
	validated_at timestamp CHECK (validated_at >= created_at)
);
CREATE TABLE IF NOT EXISTS logs(
    id           SERIAL,
    domain_name  varchar(256),
    md5sum       varchar(32),
    created_at   timestamp NOT NULL DEFAULT current_timestamp
);
COMMIT;
