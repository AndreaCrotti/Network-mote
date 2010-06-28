#!/usr/bin/ruby
# vim:foldmethod=marker:foldmarker=#[[,#]]:ts=2

LOGS = "logs"

if ARGV.length == 0 then

	values = Dir.entries(LOGS).select{ |f|
		f != "." && f != ".."
	}.map{
		|f| f.gsub("\.log","").split("-")[0,2].join("-")
	}.uniq.sort

	if values.length > 1 then
		puts "Please specify a test date. Possible values:"
		values.each do |v|
			if File.exists?(LOGS+"/"+v+".log") then
				desc = "(" + File.readlines(LOGS+"/"+v+".log").to_s.gsub("\n"," ") + ")"
			else
				desc = ""
			end
			puts "* #{v} #{desc}"
		end
		exit
	else
		date = values.first # [2,values.first.length]
	end

else 
	date = ARGV[0]
end

desc = File.readlines(LOGS+"/"+date+".log").to_s.gsub("\n"," ")

files = (Dir.glob(LOGS+"/"+date+"*").entries - [ "#{LOGS}/#{date}.log" ]).map{ |f| f.gsub("\.log","").gsub(LOGS+"/","").split("-")[2,2] }

exit unless files.length > 0

puts "\nReport for #{date}\n\n"
puts desc
puts

def avg(values)

	return "no data" if values.length == 0

	values = values - [ 0.0, 0 ] 

#	if values.length > 3 then
#		values = values.sort[1..-2]
#	end

	return sprintf("%5.2f", values.inject(0){ |s,i| s+i }.to_f / values.length)

end

associations = {}

files.each do |f|
	if associations[f[0]].nil? then
		associations[f[0]] = [ f[1].to_i ]
	else
		associations[f[0]] = (associations[f[0]] + [ f[1].to_i ]).sort
	end
end

system("rm gnuplot/*")

associations.keys.each do |mode|

	puts "ALPHA-#{mode}"
	f = File.open("gnuplot/#{mode}.dat", "w")

	associations[mode].each do |numa|

		lines = IO.popen("grep Bandwidth -A1 #{LOGS}/#{date}-#{mode}-#{sprintf("%02d",numa)}.log | grep -vE \"Bandwidth|--\"").readlines

		values = lines.map{ |l| l.split(" ")[-2].to_f }
		foo = "#{sprintf("%2d", numa)} #{avg(values)} (#{values.join(", ")})"
		puts foo
		f.puts foo

	end

	puts
	f.close

end
