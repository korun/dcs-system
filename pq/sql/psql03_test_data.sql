BEGIN;
-- Все компьютеры:
INSERT INTO computers(id, domain_name, created_at, accepted_at, deleted_at)
    VALUES
    (1,  'ice01@msiu.ru',      '1992-11-01 12:52:35', '1992-12-09 02:41:28', NULL),
    (18, 'lib18@msiu.ru',      '2003-02-21 09:17:59', '2003-06-19 20:39:30', NULL),
    (7,  'fire07@msiu.ru',     '1998-05-24 22:45:56', '1998-09-04 14:28:23', NULL),
    (14, 'monster14@msiu.ru',  '2000-11-04 09:29:42', '2000-11-20 22:41:37', NULL),
    (5,  'gorynich05@msiu.ru', '1993-07-07 05:20:55', '1993-07-10 18:49:03', '1995-09-05 03:03:52');

-- Далее детали из всех машин:
-- ice01
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at)
    VALUES
    (11, 4098,  31006, 640, '1992-11-01 12:52:35', '1992-12-09 02:41:28'), -- Video
    (12, 9798,  NULL, NULL, '1992-11-01 12:52:35', '1992-12-09 02:41:28'), -- Memory
    (13, 32902, 1538, NULL, '1992-11-01 12:52:35', '1992-12-09 02:41:28'); -- Proc

-- gorynich05
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at, deleted_at)
    VALUES
    (14, 4098,  38081, 650, '1993-07-07 05:20:55', '1993-07-10 18:49:03', NULL), -- Video
    (15, 9798,  NULL, NULL, '1993-07-07 05:20:55', '1993-07-10 18:49:03', NULL), -- Memory
    (16, 32902, 1538, NULL, '1993-07-07 05:20:55', '1993-07-10 18:49:03', '1995-09-05 03:03:52'); -- Proc (X.X)

-- fire07
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at)
    VALUES(17, 32902, 1538, NULL, '1998-05-24 22:45:56', '1998-09-04 14:28:23'); -- Proc

-- monster14
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at)
    VALUES
    (18, 4098,  38223, NULL, '2000-11-04 09:29:42', '2000-11-20 22:41:37'), -- Video
    (19, 9798,  NULL,  NULL, '2000-11-04 09:29:42', '2000-11-20 22:41:37'), -- Memory
    (20, 32902, 1538,  NULL, '2000-11-04 09:29:42', '2000-11-20 22:41:37'); -- Proc

-- lib18
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at)
    VALUES(21, 4098, 38083, 652, '2003-02-21 09:17:59', '2003-06-19 20:39:30'); -- Video

-- new for monster14 (2003)
INSERT INTO details(id, vendor_id, device_id, subsystem_id, created_at, accepted_at)
    VALUES
    (22, 9798,  NULL, NULL, '2003-02-21 09:17:59', '2003-06-19 20:39:30'), -- Memory
    (23, 32902, 1538, NULL, '2003-02-21 09:17:59', '2003-06-19 20:39:30'); -- Proc

-- Далее базовые конфиурации машин:

-- ice01 base configuration
-- Этот компьютер поступил на вооружение в 1992 году.
-- В 2003 году из него вынули память и процессор (и поместили в lib18).
INSERT INTO computers_details(id, computer_id, detail_id, created_at, deleted_at, accepted_at, allow_del_at)
    VALUES
    (24, 1, 11, '1992-11-01 12:52:35', NULL, '1992-12-09 02:41:28', NULL),
    (25, 1, 12, '1992-11-01 12:52:35', '2003-02-21 09:17:59', '1992-12-09 02:41:28', '2003-06-19 20:39:30'),
    (26, 1, 13, '1992-11-01 12:52:35', '2003-02-21 09:17:59', '1992-12-09 02:41:28', '2003-06-19 20:39:30');

-- gorynich05 base configuration
-- Этот компьютер поступил на вооружение в 1993 году.
-- Отслужив два года, он был успешно разобран по причине сгоревшего процессора.
INSERT INTO computers_details(id, computer_id, detail_id, created_at, deleted_at, accepted_at, allow_del_at)
    VALUES
    (30, 5, 14, '1993-07-07 05:20:55', '1995-09-05 03:03:52', '1993-07-10 18:49:03', '1995-09-07 12:52:01'),
    (31, 5, 15, '1993-07-07 05:20:55', '1995-09-05 03:03:52', '1993-07-10 18:49:03', '1995-09-07 12:52:01'),
    (32, 5, 16, '1993-07-07 05:20:55', '1995-09-05 03:03:52', '1993-07-10 18:49:03', '1995-09-07 12:52:01');/* Proc dead (X.X) */

-- fire07 base configuration
-- Этот компьютер поступил на вооружение в 1998 году.
-- Кроме процессора в базовую комплектацию ничего не входило...
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES(40, 7, 17, '1998-05-24 22:45:56', '1998-09-04 14:28:23');

-- monster14 base configuration
-- Этот компьютер поступил на вооружение в 2000 году.
-- В 2003 году из него вынули память и процессор (и поместили в ice01).
INSERT INTO computers_details(id, computer_id, detail_id, created_at, deleted_at, accepted_at, allow_del_at)
    VALUES
    (44, 14, 18, '2000-11-04 09:29:42', NULL, '2000-11-20 22:41:37', NULL),
    (45, 14, 19, '2000-11-04 09:29:42', '2003-02-21 09:17:59', '2000-11-20 22:41:37', '2003-06-19 20:39:30'),
    (46, 14, 20, '2000-11-04 09:29:42', '2003-02-21 09:17:59', '2000-11-20 22:41:37', '2003-06-19 20:39:30');

-- lib18 base configuration
-- Этот компьютер поступил на вооружение в 2003 году.
-- Кроме видеокарты в базовую комплектацию, увы, ничего не входило...
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES (53, 18, 21, '2003-02-21 09:17:59', '2003-06-19 20:39:30');

-- В день поступления fire07 в него добавляют 2 детали со склада
-- (видеокарту и память) которые ранее были в gorynich05.
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES
    (61, 7, 14, '1998-05-24 22:45:56', '1998-09-04 14:28:23'),
    (62, 7, 15, '1998-05-24 22:45:56', '1998-09-04 14:28:23');

-- В связи с поступлением lib18 (в котором кроме видеокарты ничего нет),
-- было принято решение снять с ice01 процессор и память и переставить в lib18...
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES
    (68, 18, 12, '2003-02-21 09:17:59', '2003-06-19 20:39:30'),
    (69, 18, 13, '2003-02-21 09:17:59', '2003-06-19 20:39:30');

-- А в ice01 переместить память и процессор, отнятые у monster14...
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES
    (70, 1,  19, '2003-02-21 09:17:59', '2003-06-19 20:39:30'),
    (71, 1,  20, '2003-02-21 09:17:59', '2003-06-19 20:39:30');

-- Для которого (monster14), в свою очередь, были закуплены новые
-- комплектующие (память и процессор).
INSERT INTO computers_details(id, computer_id, detail_id, created_at, accepted_at)
    VALUES
    (72, 14, 22, '2003-02-21 09:17:59', '2003-06-19 20:39:30'),
    (73, 14, 23, '2003-02-21 09:17:59', '2003-06-19 20:39:30');
COMMIT;
