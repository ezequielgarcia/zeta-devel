#!/usr/bin/env lua

require 'v4l'
require 'os'

function saveimg(img)
	file = io.open('image.ppm', 'w+')
	file:write('P3\n'.. w .. ' ' .. h ..'\n255\n') -- RGB IMAGE
	for i=1,#img do
		local p = img[i] .. '\n'  
		file:write(p)
	end
	file:close()
end

device = #arg

if device < 1 then
	device = '/dev/video0'
else
	device = arg[1]
end

-- open device
fd = v4l.open(device)

if fd < 0 then
	print('device not found')
	os.exit(0)
end

w, h = v4l.widht(), v4l.height()

print(device .. ': ' ..w .. 'x' .. h)

print('Controls:')

for ctrl,v in pairs(v4l.list_controls()) do
	print('[' .. ctrl .. ']' .. v4l.control(ctrl))
end

print('Standards (for selected input):')

for std,v in pairs(v4l.list_standards()) do
	print(std)
end

v4l.standard('NTSC_M')
std = v4l.standard()
if std ~= nil then
	print('Current standard: ' .. std)
else
	print('No standard')
end

--for input,v in pairs(v4l.list_inputs()) do
--	print(input)
--end

--v4l.input('CVS0')

fd = v4l.close(fd);

if fd == 0 then
	print("File descriptor closed: " .. fd)
end
