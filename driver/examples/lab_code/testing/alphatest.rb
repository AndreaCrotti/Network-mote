#!/usr/bin/ruby
# vim:foldmethod=marker:foldmarker=#[[,#]]:ts=2

####################################################
# Alpha Test Script v0.1                           #
# by Florian Weingarten, Johannes Gilger           #
#                                                  #
# Usage:                                           #
#  Client: Run ./alphatest.rb from within testing/ #
#  Server: Run ./alpha and ./iperf -s              #
#                                                  #
# Keep in mind:                                    #
# * run this script as root, otherwise alpha wont  #
#   start (but you will see no error message!)     #
# * compile alpha with "make final", otherwise the #
#   debugging output will slow you down            #
####################################################

#############################################################################
@setup = {

	# Hostname/IP of server (also edit the alpha config file (see :config)!)
	:server => "192.168.1.3",

	# Directory to store log files
	:output => "logs",

	# Config file to use
	:config => "./test.conf",

	# Binary files
	:alpha   => "../alpha",
	:iperf   => "/usr/bin/iperf",
	:ping    => "/bin/ping",
	:killall => "/usr/bin/killall",
	:rm      => "/bin/rm",

	# Modes we want to test, i.e.
	# association configuration for different modes
	# (use 'x' as placeholder for the number of associations)
	:modes => {

		# Default Modes
		"N"   => "x:0:0",
		"C"   => "0:x:0",
		"M"   => "0:0:x",

		# Custom modes
#		"CNa" => "1:x:0",
#		"CNb" => "2:x:0",
#		"MNa" => "1:0:x",
#		"MNb" => "2:0:x",

	},

	# Number of associations (-n)
	:associations => [ 1, 2, 5, 10 ],
#	:associations => [ *1..16 ] +  [ 32 ],
#	:associations => [ *0..6 ].map{ |i| 2**i },
#	:associations => [ 1 ] + [ *1..15 ].map{ |i| 2*i },

	# Number of tests to do for each mode/association combo
	:numtests => 5,

	# Number of associations for the ALPHA-M merkle tree depth test
	# and depths values we want to test (set to [] to disable this test)
	# NOTE: This is still experimental and we dont know yet if this
	# test makes any 
	:treedepth_associations => 4,
#	:treedepths => [ 2, 3, 5, 23, 42, 80 ],
#	:treedepths => [ 16 ],
	:treedepths => [],

	# Parameters for iperf ("-c :server" is added automatically)
	:iperfp => "-f k -t 30",

	# Additional parameters for alpha
	# NOTE: if you set longer hash chains (-l), remember to do so
	# on the server as well, otherwise the servers ack chain will be empty
	# before the clients sign chain is
	:alphap => "",

	# Ping packets for handshake
	:pings => 5,	

}
#############################################################################

date = Time.now.strftime("%Y%m%d-%H%M%S")

unless File.directory?(@setup[:output])
	puts "Logfile directory `#{@setup[:output]}' does not exist. Exiting."
	exit
end

def start_alpha(run, mode, associations, treedepth, logfile)
#[[

	nstr = @setup[:modes][mode]

	if treedepth != nil then
		treecmd = "-t #{treedepth}"
	else
		treecmd = ""
	end

	system("echo Starting run #{run}. mode=#{mode}, associations=#{nstr} x=#{associations}, treedepth=#{treedepth.inspect} >> #{logfile}")
	system("#{@setup[:alpha]} #{@setup[:alphap]} #{treecmd} -d -c #{@setup[:config]} -n #{nstr.gsub("x", associations.to_s)} 2>&1 >> #{logfile}")
	system("#{@setup[:ping]} #{@setup[:server]} -c #{@setup[:pings]} 2>&1 >> #{logfile}")
	system("#{@setup[:iperf]} -c #{@setup[:server]} #{@setup[:iperfp]} 2>&1 >> #{logfile}")
	system("#{@setup[:killall]} alpha 2>&1")
	5.times do system("#{@setup[:killall]} -9 alpha > /dev/null 2>&1") end
	system("#{@setup[:rm]} alpha.sock > /dev/null 2>&1")
	system("echo >> #{logfile}")

#]]
end

puts "\nALPHA TEST SCRIPT\n=================\n\n"
puts "Time of start: #{date}"
puts "Remote server: #{@setup[:server]}"
puts "Tests per run: #{@setup[:numtests]}"
puts

if ARGV.length == 0 then
	puts "Please enter a short one-line description of this test run,"
	puts "for example, the servers association configuration, etc.:"
	desc = STDIN.readline
else
	desc = ARGV.join(" ")
	puts "Setting description:"
	puts desc
end

desc += " iperf=" + @setup[:iperfp]

f = File.new("#{@setup[:output]}/#{date}.log", "w")
f.puts desc.chomp
f.close

puts

@setup[:modes].keys.sort.each do |mode|

	puts "Initiating ALPHA-#{mode} benchmarks (-n #{@setup[:modes][mode]})"

	@setup[:associations].each do |i|

		logfile = @setup[:output] + "/" + date + "-" + mode + "-" + sprintf("%02d", i) + ".log"
		puts "ALPHA-#{mode}: #{sprintf("%2d", i)} associations (#{logfile})"
		@setup[:numtests].times do |run| start_alpha(run, mode, i, nil, logfile) end
		
	end

	puts

	if mode == "M" and
		@setup[:treedepth_associations] > 0 and
		@setup[:treedepths] != nil and
		@setup[:treedepths].length > 0
	then

		puts "Initiating ALPHA-#{mode} benchmarks with variable tree depth"

		@setup[:treedepths].each do |depth|
	
			logfile = "#{@setup[:output]}/#{date}-#{mode}tree-#{sprintf("%02d", depth)}.log"
			puts "ALPHA-#{mode}: #{@setup[:treedepth_associations]} associations, tree depth #{sprintf("%3d", depth)} (#{logfile})"
			@setup[:numtests].times do |run| start_alpha(run, mode, @setup[:treedepth_associations], depth, logfile) end

		end

	end

end

puts "\nAll tests done.\n\n"
