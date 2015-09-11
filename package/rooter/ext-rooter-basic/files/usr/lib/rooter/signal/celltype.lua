#!/usr/bin/lua

modem = arg[1]
numb = arg[2]
echo = 0

datalist = {}
celllist = {}

datalist[1] = "320u"
celllist[1] = 2
datalist[2] = "330u"
celllist[2] = 2
datalist[3] = "e3276"
celllist[3] = 3
datalist[4] = "e398"
celllist[4] = 3
datalist[5] = "e389"
celllist[5] = 3
datalist[6] = "e392"
celllist[6] = 3
datalist[7] = "e397"
celllist[7] = 3
datalist[8] = "e8278"
celllist[8] = 3
datalist[9] = "mf820"
celllist[9] = 3
datalist[10] = "mf821"
celllist[10] = 3
datalist[11] = "k5005"
celllist[11] = 3
datalist[12] = "k5006"
celllist[12] = 3
datalist[13] = "l800"
celllist[13] = 3
datalist[14] = "e398"
celllist[14] = 3
datalist[15] = "mf880"
celllist[15] = 3
datalist[16] = "e3272"
celllist[16] = 3

printf = function(s,...)
	if echo == 0 then
		io.write(s:format(...))
	else
		ss = s:format(...)
		os.execute("/usr/lib/rooter/logprint.sh " .. ss)
	end
end

found = 1
index = 1
line = datalist[index]
data = string.lower(modem)

while line ~= nil do
	s, e = string.find(data, line)
	if s ~= nil then
		found = celllist[index]
		break
	end
	index = index + 1
	line = datalist[index]
end

file = io.open("/tmp/celltype" .. numb, "w")
cell = string.format("%s%s%s%s", "CELL", "=\"", found, "\"")
file:write(cell.. "\n")
file:close()