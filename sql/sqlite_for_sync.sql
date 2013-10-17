-- Запрос, получающий таблицу компьютеров + последнюю md5 сумму из таблицы
-- логов. Возможно понадобится сравнивающей нити.
SELECT
    computers.id,
    computers.db_id,
    computers.domain,
    computers.md5sum     AS comp_md5,
    logs_md5.md5sum      AS log_md5,
    computers.created_at AS comp_cr_at,
    logs_md5.created_at  AS log_cr_at
    FROM computers
    INNER JOIN (
        SELECT
            id,
            domain,
            md5sum,
            created_at
            FROM logs
            GROUP BY domain
            ORDER BY created_at, id
        ) AS logs_md5
    ON computers.domain LIKE logs_md5.domain;

-- Результат:
-- id|db_id|     domain       |          comp_md5             |             log_md5             |    comp_cr_at     |     log_cr_at     
-- --+-----+------------------+-------------------------------+---------------------------------+-------------------+-------------------
--  1|    1|ice01@msiu.ru     |QAGE5AK65CQOI66KMIOB4I9IOQDI39G|5N4O8BOVMUQ36DD4NFCKBG8QOIQ7QACID|1992-11-01 12:52:35|2013-01-22 10:55:02
--  2|   18|lib18@msiu.ru     |WBIYOC9M94OQI8Q4IM9M4N9GMI4I35C|ZB7CK3QA75U6X8UJ6A6C5ACBK9ME9QEKM|2003-02-21 09:17:59|2013-01-22 10:55:02
--  3|    7|fire07@msiu.ru    |GINEEAG6IKOM94K4O79BBE8EQKOJIXE|GO7OG4QG8AIA7M3BOTUI4A5OQQ4I6B7BK|1998-05-24 23:45:56|2013-01-22 10:55:02
--  4|   14|monster14@msiu.ru |KMBE64NO439EBO553W4O4QGBGM9O3KG|OK499Q96867TG5CO8G4EAQ6KEDMC6BIOG|2000-11-04 09:29:42|2013-01-22 10:55:02
--  5|    5|gorynich05@msiu.ru|98OOGO4Q4EMBA4454D4GGPI4K796C7O|BZDMK8R8A6IN57IA7QM3974974A5BWBO3|1993-07-07 06:20:55|2013-01-22 10:55:02
-- Отсюда наглядно видно, что что-то не так, суммы не совпадают.
-- Поэтому:
--   1) записываем в компьютеры новую md5 (из логов);
--   2) записываем в хэш новую md5;
--   3) синхронизируемся с базой данных.

SELECT
	configs.id,
	configs.computer_id,
	configs.detail_id,
	configs.created_at AS config_cr_at,
	details.vendor_id,
	details.device_id,
	details.subsystem_id,
	details.serial,
	details.created_at
	FROM configs
	INNER JOIN details
		ON configs.detail_id = details.id
	WHERE configs.computer_id = 4 AND configs.deleted_at IS NULL;
-- id|computer_id|detail_id|config_cr_at       |vendor_id|device_id|subsystem_id|serial                          |created_at
-- --+-----------+---------+-------------------+---------+---------+------------+--------------------------------+-------------------
-- 12|          4|        8|2013-01-24 16:57:36|4098     |38223    |577836935   |D6A5KBJO7YAMAGEEM5UOEIJ8D8QC95C9|2013-01-24 16:57:35
-- 15|          4|       12|2013-01-24 16:57:36|9798     |0        |0           |7BEIEXEF8I8OIGO6MENQ5QOUK33G66UM|2013-01-24 16:57:35
-- 16|          4|       13|2013-01-24 16:57:36|32902    |1538     |0           |4E6OM8BTA6CMBMMB43MG69OM9C5Q55GG|2013-01-24 16:57:36
