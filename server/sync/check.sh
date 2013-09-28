echo 'logs';
psql device_control_database -c "SELECT * FROM logs ORDER BY id DESC LIMIT 5;";
echo 'new configs';
psql device_control_database -c "SELECT id, computer_id, detail_id, state, created_at, accepted_at, deleted_at FROM computers_details WHERE accepted_at IS NULL;";
echo 'new computers';
psql device_control_database -c "SELECT * FROM computers WHERE accepted_at IS NULL;";
echo 'new details'
psql device_control_database -c "SELECT * FROM details WHERE accepted_at IS NULL;"
