require 'typelib'
require 'pocolog'
require 'pp'

include Typelib
include Pocolog

registry = Registry.import('laser_readings.h')

laser_t = registry.get('/DFKI/LaserReadings')
pp laser_t
pp laser_t.memory_layout

file         = Logfiles.create('laser_scans.log')
laser_stream = file.stream("Hokuyo", laser_t, true)

sample = laser_t.new
(1..100).each do |i|
    sample.stamp = i
    sample.ranges.push(i)
    laser_stream.write(Time.at(1, 2), Time.at(3, 4), sample)
end

