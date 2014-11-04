#!/usr/bin/ruby

require 'ipaddr'
require 'forgery'
require 'socket'
require 'securerandom'

GOOD_RANGE = 14
MAX_BAD_COUNT = 10

@good_ip_base = IPAddr.new(Forgery(:internet).ip_v4 + "/255.255.255.240")

@good_ips = []
GOOD_RANGE.times {|n| @good_ips << IPAddr.new(@good_ip_base.to_i + n + 1, Socket::AF_INET)}

@good_ips.each { |ip| puts "#{ip} --> #{ip.to_i}" }

@bad_ips = []
count = Random.rand(MAX_BAD_COUNT) + 1
count.times { @bad_ips << IPAddr.new(Forgery(:internet).ip_v4) }
@bad_ips.delete_if {|ip| (@good_ip_base.to_i - ip.to_i).abs < (GOOD_RANGE * 2)}

@bad_ips.each { |ip| puts "#{ip} --> #{ip.to_i}" }
