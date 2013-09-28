BEGIN;
--  0 = NULL, 1 = some timestamp
--  acc_at | alw_del | del_at |
--     0   |    0    |    0   | => Новая непринятая конфигурация
--     0   |    0    |    1   | => (000) помеченная на удаление
--  ---0---+----1----+----0---| => Недопустимая маска
--     0   |    1    |    1   | => (001) с подтверждённым удалением
--     1   |    0    |    0   | => Принятая конфигурация (000)
--     1   |    0    |    1   | => (100) помеченная на удаление
--  ---1---+----1----+----0---| => Недопустимая маска
--     1   |    1    |    1   | => (101) с подтверждённым удалением
CREATE FUNCTION set_config_state() RETURNS trigger AS $set_config_state$
    BEGIN
        NEW.state := 0;
        IF NEW.accepted_at  IS NOT NULL THEN
            NEW.state := NEW.state | 4;     -- 0b100
        END IF;
        IF NEW.allow_del_at IS NOT NULL THEN
            NEW.state := NEW.state | 2;     -- 0b010
        END IF;
        IF NEW.deleted_at   IS NOT NULL THEN
            NEW.state := NEW.state | 1;     -- 0b001
        END IF;
        IF NEW.state = 2 OR NEW.state = 6 THEN -- 0b010 || 0b110
            RAISE EXCEPTION 'Field "allow_del_at" not NULL, whereas "deleted_at" is NULL!';
        END IF;
        RETURN NEW;
    END;
$set_config_state$ LANGUAGE plpgsql;

CREATE TRIGGER set_config_state
    BEFORE INSERT OR UPDATE ON computers_details
    FOR EACH ROW EXECUTE PROCEDURE set_config_state();

COMMIT;
