-- Предположим, что:
COMP_ID = SELECT id FROM computers WHERE domen_name LIKE 'monster14@msiu.ru';

-- Получить список всех деталей для компьютера
SELECT * FROM computers_details
	WHERE
		computer_id = COMP_ID
	ORDER BY id;

-- Получить список непроверенных конфигураций деталей
SELECT * FROM computers_details
	WHERE
		computer_id = COMP_ID AND
		accepted_at IS NULL
	ORDER BY id;

-- Получить список актуальных деталей (тех, что стоят на нём в данный момент)
SELECT * FROM computers_details
	WHERE
		computer_id = COMP_ID AND
		accepted_at IS NOT NULL AND
		deleted_at IS NULL
	ORDER BY id;

----------------------------------------------------------------------------
-- Получить все машины
SELECT	computers.id,
	computers.domen_name
	FROM computers
	ORDER BY computers.id;

-- Получить список всех деталей, а также
-- когда и какая деталь стояла на той или иной машине.
-- (удалённые спецификации не учитываются)
SELECT	cd.id,
	cd.computer_id,
	cd.detail_id,
	cd.accepted_at,
	cd.deleted_at,
	d.vendor_id,
	d.device_id,
	d.subsystem_id
	FROM computers_details AS cd
	INNER JOIN details AS d
		ON cd.detail_id = d.id
	WHERE
		cd.accepted_at IS NULL OR
		cd.deleted_at IS NULL
	ORDER BY cd.id;

-- Получить список актуальных деталей для конкретной машины для подсчёта текущей md5-суммы
SELECT	cd.computer_id,
	cd.detail_id,
	d.vendor_id,
	d.device_id,
	d.subsystem_id,
	d.serial
	FROM computers_details AS cd
	INNER JOIN details AS d
		ON cd.detail_id = d.id
	WHERE
		cd.computer_id = 14 AND
		cd.accepted_at IS NOT NULL AND
		cd.deleted_at IS NULL
	ORDER BY cd.computer_id, cd.detail_id;
