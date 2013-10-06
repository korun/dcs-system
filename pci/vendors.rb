# /* encoding: UTF-8 */

puts "DROP   TABLE IF     EXISTS vendors;"
puts "CREATE TABLE IF NOT EXISTS vendors(id integer PRIMARY KEY, name varchar(128));"
puts "INSERT INTO vendors"
first = true
File.open("/media/7AF6-74C5/Device_Control_System/pci.ids", 'r').each{ |str|
	unless str =~ /^([\s]|#|C)/i
		str = str.split(" ", 2)
		print first ? "\tVALUES\t" : ",\n\t\t"
		print "(#{eval('0x' + str[0])}, '#{str[1].chomp.gsub(/'+/, "''")}')"
		first = false
	end
}.close()
puts ";"
