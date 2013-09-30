echo 'delete logs';
psql device_control_database -c "TRUNCATE logs;";
echo 'delete new configs';
psql device_control_database -c "DELETE FROM computers_details WHERE accepted_at IS NULL;";
echo 'delete new computers';
psql device_control_database -c "DELETE FROM computers WHERE accepted_at IS NULL;";
echo 'delete new details'
psql device_control_database -c "DELETE FROM details WHERE accepted_at IS NULL;"
