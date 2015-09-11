#!/usr/bin/lua

modemn = arg[1]

local smsresult = "/tmp/smsresult" .. modemn .. ".at"
local t = {}
local tptr
local m_pdu_ptr
local m_pdu
local m_smsc
local m_with_udh
local m_report
local m_number
local m_replace
local m_alphabet
local m_flash
local m_date
local m_time
local m_text
local m_concat
local m_read
local m_index

local max_smsc = 64
local max_number = 64
local max_udh_data = 512

function reset()
	m_smsc = nil
	m_with_udh = 0
	m_report = 0
	m_number = nil
	m_replace = 0
	m_alphabet = -1
	m_flash = 0
	m_date = nil
	m_time = nil
	m_text = nil
	m_concat = nil
end

function hasbit(x, p)
	return x % (p + p) >= p 
end

function bitor(x, y)
	local p = 1; local z = 0; local limit = x > y and x or y
	while p <= limit do
		if hasbit(x, p) or hasbit(y, p) then
			z = z + p
		end
		p = p + p
	end
	return z
end

function bitand(x, y)
	local p = 1; local z = 0; local limit = x > y and x or y
	while p <= limit do
		if hasbit(x, p) and hasbit(y, p) then
			z = z + p
		end
		p = p + p
	end
	return z
end

printf = function(s,...)
	if echo == 0 then
		io.write(s:format(...))
	else
		ss = s:format(...)
		os.execute("/usr/lib/rooter/logprint.sh " .. ss)
	end
end

function isxdigit(digit)
	if digit >= 48 and digit <= 57 then
		return 1
	end
	if digit >= 97 and digit <= 102 then
		return 1
	end
	if digit >= 65 and digit <= 70 then
		return 1
	end
	return 0
end

function isdigit(digit)
	if digit >= 48 and digit <= 57 then
		return 1
	end
	return 0
end

function octet2bin(octet)
	result = 0
	if octet:byte(1) > 57 then
		result = result + octet:byte(1) - 55
	else
		result = result + octet:byte(1) - 48
	end
	result = result * 16
	if octet:byte(2) > 57 then
		result = result + octet:byte(2) - 55
	else
		result = result + octet:byte(2) - 48
	end
	return result
end

function octet2bin_check(octet)
	if octet:byte(1) == 0 then
		return -1
	end
	if octet:byte(2) == 0 then
		return -2
	end
	if isxdigit(octet:byte(1)) == 0 then
		return -3
	end
	if isxdigit(octet:byte(2)) == 0 then
		return -4
	end
	return octet2bin(octet)
end

function swapchars(sstring)
	local length = sstring:len()
	local xstring = nil
	local i = 1
	while i < length do
		c1 = sstring:sub(i, i)
		c2 = sstring:sub(i+1, i+1)
		if xstring == nil then
			xstring = c2 .. c1
		else
			xstring = xstring .. c2 .. c1
		end
		i = i + 2
	end
	return xstring
end

function parseSMSC()
	m_pdu_ptr = m_pdu
	local length = octet2bin_check(m_pdu_ptr)
	if length < 0 then
		return -1
	end
	if length == 0 then
		return 0
	end
	if length < 2 or length > max_smsc then
		return -1
	end
	length = (length * 2) - 2
	local mlen = m_pdu:len()
	if mlen < (length + 4) then
		return -1
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
	local addr_type = octet2bin_check(m_pdu_ptr)
	if addr_type < 0 then
		return -1
	end
	if addr_type < 0x80 then
		return -1
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
	m_smsc = m_pdu_ptr:sub(0, length)
	m_smsc = swapchars(m_smsc)
	if addr_type < 0x90 then
		for j=1,length do
			if isxdigit(m_smsc:byte(j)) == 0 then
				return -1
			end	
		end
	else
		if m_smsc:byte(length) == 70 then
			m_smsc = m_smsc:sub(1, length-1)
		end
		local leng = m_smsc:len()
		for j=1,leng do
			if isdigit(m_smsc:byte(j)) == 0 then
				return -1
			end	
		end
	end
	m_pdu_ptr = m_pdu_ptr:sub(length+1)
	return 1
end

function explainAddressType(octet_char, octet_int)
	local result
	if octet_char ~= nil then
		result = octet2bin_check(octet_char)
	else
		result = octet_int
	end
	return result
end

function pdu2binary(pdu, with_udh)
	local skip_octets
	local octetcounter
	m_text = nil
	local octets = octet2bin_check(pdu)
	if octets < 0 then
		return -1
	end
	if with_udh > 0 then
		local pdu2 = pdu:sub(3)
		local udhsize = octet2bin_check(pdu2)
		if udhsize < 0 then
			return -1
		end
		skip_octets = udhsize + 1
	end
	for octetcounter=0, (octets - skip_octets - 1) do
		local pdu2 = pdu:sub((octetcounter * 2) + 3 + (skip_octets * 2))
		local i = octet2bin_check(pdu2)
		if i < 0 then
			return -1
		end
		if m_text == nil then
			m_text = string.char(i)
		else
			m_text = m_text .. string.char(i)
		end

	end
	return (octets - skip_octets)
end

function pdu2text(pdu, with_udh)
	local result
	local octetcounter
	local skip_characters = 0
	local binary = 0
	m_text = nil
	local septets = octet2bin_check(pdu)
	if septets < 0 then
		return -1
	end
	if with_udh > 0 then
		local pdu2 = pdu:sub(3)
		local udhsize = octet2bin_check(pdu2)
		if udhsize < 0 then
			return -1
		end
		pdu2 = pdu2:sub(3)
		local IEI = octet2bin_check(pdu2)
		if IEI < 0 then
			return -1
		end
		if IEI == 0 then
			pdu2 = pdu2:sub(3)
			local IEDL = octet2bin_check(pdu2)
			if IEDL ~= 3 then
				return -1
			end
			pdu2 = pdu2:sub(3)
			local REFN = octet2bin_check(pdu2)
			if REFN < 0 then
				return -1
			end
			pdu2 = pdu2:sub(3)
			local MAXP = octet2bin_check(pdu2)
			if MAXP < 0 then
				return -1
			end
			pdu2 = pdu2:sub(3)
			local PART = octet2bin_check(pdu2)
			if PART < 0 then
				return -1
			end
			m_concat = string.format("Msg# %d  Part %d/%d  ", REFN, PART, MAXP)
		end
		skip_characters = math.floor((((udhsize+1)*8)+6)/7)
	end
	local octets = math.floor((septets * 7 + 7) / 8)
	local bitposition = 0
	local byteposition
	local byteoffset
	local i
	octetcounter = 0
	for charcounter=0,septets-1 do
		local c = 0
		for bitcounter=0,6 do
			byteposition = math.floor(bitposition / 8)
			byteoffset = bitposition % 8
			while (byteposition >= octetcounter) and (octetcounter < octets) do
				local pdu2 = pdu:sub((octetcounter * 2) + 3)
				i = octet2bin_check(pdu2)
				if i < 0 then
					return -2
				end
				binary = i
				octetcounter = octetcounter + 1
			end
			if bitand(binary, (2^byteoffset)) > 0 then
				c = bitor(c, 128)
			end
			bitposition = bitposition + 1
			c = bitand(math.floor(c / 2), 127)
		end
		if charcounter >= skip_characters then
			if m_text == nil then
				m_text = string.char(c)
			else
				m_text = m_text .. string.char(c)
			end
		end
	end
	return 1
end

function parseDeliver()
	if m_pdu_ptr:len() < 4 then
		return 0
	end
	local padding = 0
	local length = octet2bin_check(m_pdu_ptr)
	if length < 0 or length > max_number then
		return 0
	end
-- Sender Address
	if length == 0 then
		m_pdu_ptr = m_pdu_ptr:sub(5)
	else
		padding = length % 2
		m_pdu_ptr = m_pdu_ptr:sub(3)
		local addr_type = explainAddressType(m_pdu_ptr, 0)
		if addr_type < 0 then
			return 0
		end
		if addr_type < 0x80 then
			return 0
		end
		m_pdu_ptr = m_pdu_ptr:sub(3)
		if bitand(addr_type, 112) == 80 then
			if m_pdu_ptr:len() < (length + padding) then
				return 0
			end
			local htmp = string.format("%x", math.floor((length * 4) / 7))
			if htmp:len() < 2 then
				htmp = "0" .. htmp
			end
			htmp = htmp:upper()
			local tpdu = htmp .. m_pdu_ptr
			local res = pdu2text(tpdu, 0)
			if res  < 0 then
				return 0
			end
			m_number = m_text
			m_text = nil
		else
			m_number = m_pdu_ptr:sub(1, length + padding + 1)
			m_number = swapchars(m_number)
			if m_number:byte(length + padding) == 70 then
				m_number = m_number:sub(1, length + padding - 1)
			end
		end
	end
	m_pdu_ptr = m_pdu_ptr:sub(length + padding + 1)
	if m_pdu_ptr:len() < 20 then
		return 0
	end
-- PID
	local byte_buf = octet2bin_check(m_pdu_ptr)
	if byte_buf < 0 then
		return 0
	end
	if bitand(byte_buf, 0xF8) == 0x40 then
		m_replace = bitand(byte_buf, 0x07)
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
-- Alphabet
	byte_buf = octet2bin_check(m_pdu_ptr)
	if byte_buf < 0 then
		return 0
	end
	m_alphabet = math.floor(bitand(byte_buf, 0x0C) / 4)
	if m_alphabet == 3 then
		return 0
	end
	if m_alphabet == 0 then
		m_alphabet = -1
	end
-- Flash Msg
	if bitand(byte_buf, 0x10) > 0 then
		if bitand(byte_buf, 0x01) > 0 then
		m_flash = 1
		end
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
-- Date
	local str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. "-" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. "-" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	m_date = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
-- Time
	str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. ":" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. ":" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	m_time = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
	if octet2bin_check(m_pdu_ptr) < 0 then
		return 0
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
-- Text
	local result = 0
	local bin_udh = 1
	if m_alphabet <= 0 then
		result = pdu2text(m_pdu_ptr, m_with_udh)
		return result
	else
		result = pdu2binary(m_pdu_ptr, m_with_udh)
		return result
	end
	return 1
end

function parseStatusReport()
	if m_pdu_ptr:len() < 6 then
		return 0
	end
	local messageid = octet2bin_check(m_pdu_ptr)
	if messageid < 0 then
		return 0
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
	local length = octet2bin_check(m_pdu_ptr)
	if length < 1 or length > max_number then
		return 0
	end
	local padding = length % 2
	m_pdu_ptr = m_pdu_ptr:sub(3)
	local addr_type = explainAddressType(m_pdu_ptr, 0)
	if addr_type < 0x80 then
		return 0
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
	if bitand(addr_type, 112) == 80 then
		if m_pdu_ptr:len() < (length + padding) then
			return 0
		end
		local htmp = string.format("%x", math.floor((length * 4) / 7))
		if htmp:len() < 2 then
			htmp = "0" .. htmp
		end
		local tpdu = htmp .. m_pdu_ptr
		local res = pdu2text(tpdu, 0)
		if res  < 0 then
			return 0
		end
		m_number = m_text
		m_text = nil
	else
		m_number = m_pdu_ptr:sub(1, length + padding + 1)
		m_number = swapchars(m_number)
		if m_number:byte(length + padding) == 70 then
			m_number = m_number:sub(1, length + padding - 1)
		end
	end
	m_pdu_ptr = m_pdu_ptr:sub(length + padding + 1)
	if m_pdu_ptr:len() < 14 then
		return 0
	end
-- Date
	local str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. "-" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. "-" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	m_date = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
-- Time
	str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. ":" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. ":" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	m_time = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
	if octet2bin_check(m_pdu_ptr) < 0 then
		return 0
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
-- Discharge Date
	local str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. "-" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. "-" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	local d_date = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
-- Time
	str_buf = m_pdu_ptr:sub(2,2) .. m_pdu_ptr:sub(1,1) .. ":" .. m_pdu_ptr:sub(4,4) .. m_pdu_ptr:sub(3,3) .. ":" .. m_pdu_ptr:sub(6,6) .. m_pdu_ptr:sub(5,5)
	if (not isdigit(m_pdu_ptr:byte(1))) or (not isdigit(m_pdu_ptr:byte(2))) or (not isdigit(m_pdu_ptr:byte(3))) or (not isdigit(m_pdu_ptr:byte(4))) or (not isdigit(m_pdu_ptr:byte(5))) or (not isdigit(m_pdu_ptr:byte(6))) then
		return 0
	end
	local d_time = str_buf
	m_pdu_ptr = m_pdu_ptr:sub(7)
	if octet2bin_check(m_pdu_ptr) < 0 then
		return 0
	end
	m_pdu_ptr = m_pdu_ptr:sub(3)
	local status = octet2bin_check(m_pdu_ptr)
	if status < 0 then
		return 0
	end
	m_text = string.format("Discharge Timestamp: %s %s  Message ID: %d  Status: %d", d_date, d_time, messageid, status)
	return 1
end

function convert(tocode, fromcode)

-- convert from 16 bit Unicode to 8 bit 

end

function parse()
	local flag = parseSMSC()
	if flag ~= 1 then
		return 0
	end
	local tmp = octet2bin_check(m_pdu_ptr)
	if tmp < 0 then
		return 0
	end
	if bitand(tmp, 0x40) > 0 then
		m_with_udh = 1
	end
	if bitand(tmp, 0x20) > 0 then
		m_report = 1
	end
	local type = bitand(tmp, 3)
	if type == 0 then
		m_pdu_ptr = m_pdu_ptr:sub(3)
		local result = parseDeliver()
		if result < 1 then
			return 0
		end
		if m_alphabet == 2 then
			convert("UTF8", "UTF16BE")
		end
	else
		if type == 2 then
			m_pdu_ptr = m_pdu_ptr:sub(3)
			local result = parseStatusReport()
			return result
		else
			return 0
		end
	end
	return 1
end

function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

function readpdu(pdu)
	m_pdu = pdu
	m_pdu_ptr = m_pdu
	reset()
	local flag = parse()
	if flag > 0 then
		t[tptr] = m_index
		t[tptr+1] =  m_read
		t[tptr+2] = m_number
		t[tptr+3] = m_date
		t[tptr+4] = m_time
		if m_concat ~= nil then
			m_text = m_concat .. m_text
		end
		t[tptr+5] = m_text
		tptr = tptr + 6
	end
end

local max_msg = "0"
local used_msg = "0"
tptr = 3
t[1] = used_msg
t[2] = max_msg
local file = io.open(smsresult, "r")
if file ~= nil then
	repeat
		local s, e, cs, ce, ms, me
		local line = file:read("*line")
		if line == nil then
			break
		end
		s, e = line:find("+CPMS:")
		if s ~= nil then
			cs, ce = line:find(",", e)
			if cs ~= nil then
				used_msg = trim(line:sub(e+1, cs-1))
				t[1] = used_msg
				ms, me = line:find(",", ce+1)
				if ms ~= nil then
					max_msg = trim(line:sub(ce+1, ms-1))
					t[2] = max_msg
				end
			end
			line = file:read("*line")
			if line == nil then
				break
			end
		end
		s, e = line:find("+CMGL:")
		if s ~= nil then
			m_index = "0"
			cs, ce = line:find(",", e)
			if cs ~= nil then
				m_index = trim(line:sub(e+1, cs-1))
			end
			m_read = "**"
			ds, de = line:find(",", ce+1)
			if ds ~= nil then
				m_r = trim(line:sub(ce+1, ds-1))
				if m_r == "1" then
					m_read = "--"
				end
			end
			line = file:read("*line")
			if line == nil then
				break
			end
			if string.len(line) < 3 then
				line = file:read("*line")
			end
			if line == nil then
				break
			end
			readpdu(line)
		end
	until 1==0
	file:close()
end

local tfile = io.open("/tmp/smstext", "w")
if tonumber(used_msg) == 0 then
	tfile:close()
else
	tfile:write(t[1] .. "\n")
	tfile:write(t[2] .. "\n")
	i = 3
	while t[i] ~= nil do
		tfile:write(t[i] .. "\n")
		tfile:write(t[i + 2] .. "\n")
		tfile:write(t[i + 5] .. "\n")
		local mn = t[i+2] .. "                    "
		mn = mn:sub(1,20)
		local stxt = t[i + 5]
		stxt = stxt:sub(1,20) .. "  ..."
		local msg = t[i+1] .. " " .. mn .. t[i + 3] .. " " .. t[i + 4] .. "  " .. stxt
		tfile:write(msg .. "\n")
		i = i + 6
	end
	tfile:close()
end

