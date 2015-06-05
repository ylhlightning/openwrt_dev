#!/usr/bin/lua

cmd = arg[1]
netdev = arg[2]
port = arg[3]
modemn = arg[4]
auth = arg[5]
apn = arg[6]
user = arg[7]
pw = arg[8]
pincode = arg[9]

dev = "/dev/" .. port

if user == "NIL" then
	user = nil
end
if pw == "NIL" then
	pw = nil
end
if auth == nil then
	auth = "0"
end

--
-- set debug to 1 for debug output
--
debug = 0

tid = 1
cid = {}
cidname = {}
pinstat = {}
errmsg = {}
pktmsg = {}
wds_handle = nil
state = "/tmp/qmistate." .. netdev
echo = 1
pkterror = nil

QMI_CTL = 0
QMI_WDS = 1
QMI_DMS = 2
QMI_NAS = 3

cidname[QMI_CTL] = "QMI_CTL"
cidname[QMI_WDS] = "QMI_WDS"
cidname[QMI_DMS] = "QMI_DMS"
cidname[QMI_NAS] = "QMI_NAS"

pinstat[0] = "not initialized"
pinstat[1] = "enabled, not verified"
pinstat[2] = "enabled, verified"
pinstat[3] = "disabled"
pinstat[4] = "blocked"
pinstat[5] = "permanently blocked"
pinstat[6] = "unblocked"
pinstat[7] = "changed"

errmsg[0x01] = "malformed message"
errmsg[0x03] = "internal error"
errmsg[0x0e] = "call failed"
errmsg[0x47] = "invalid QMI command"

pktmsg[1] = "disconnected"
pktmsg[2] = "connected"
pktmsg[3] = "suspended"
pktmsg[4] = "authenticating"

-- This format function packs a list of integers into a binary string.
-- The sizes of the integers can be specified, both little and big endian
-- ordering are supported. The format parameter is a string composed of 
-- ASCII digit numbers, the size in bytes of the corresponding value.
-- Example:
--   write_format(true, "421", 0x12345678, 0x432931, 0x61) returns "xV4.1)a",
--     a 7 bytes string whose characters are in hex: 78 56 45 12 31 29 61
function write_format(little_endian, format, ...)
  local res = ''
  local values = {...}
  for i=1,#format do
    local size = tonumber(format:sub(i,i))
    local value = values[i]
    local str = ""
    for j=1,size do
      str = str .. string.char(value % 256)
      value = math.floor(value / 256)
    end
    if not little_endian then
      str = string.reverse(str)
    end
    res = res .. str
  end
  return res
end

-- This format function does the inverse of write_format. It unpacks a binary
-- string into a list of integers of specified size, supporting big and little 
-- endian ordering. Example:
--   read_format(true, "421", "xV4.1)a") returns 0x12345678, 0x2931 and 0x61.
function read_format(little_endian, format, str)
  local idx = 0
  local res = {}
  for i=1,#format do
    local size = tonumber(format:sub(i,i))
    local val = str:sub(idx+1,idx+size)
    local value = 0
    idx = idx + size
    if little_endian then
      val = string.reverse(val)
    end
    for j=1,size do
      value = value * 256 + val:byte(j)
    end
    res[i] = value
  end
  return unpack(res)
end

printf = function(s,...)
	if echo == 0 then
		io.write(s:format(...))
	else
		local ss = s:format(...)
		os.execute("/usr/lib/rooter/logprint.sh " .. ss)
	end
end

function debug_print(pfx, packet)
	local len = string.len(packet)
	local pf = "%s "
	local rf =''
	for i = 1, len do
		pf = pf .. "%02x "
		rf = rf .. "1"
	end
	printf(pf, pfx, read_format(true, rf, packet))
	printf("\n")
end

function printerror(number)
	if errmsg[number] ~= nil then
		printf("\nQMI Error : %s\n", errmsg[number])
	else
		printf("\nQMI Error # %04x\n", number)
	end
end

--done
function mk_qmi(sys, cid, msgid, tlvs)
	local ret, tlvlen
	local tlvbytes = ''
	if tlvs == nil then
		tlvlen = 0
	else
		for k, v in pairs(tlvs) do
			local len = string.len(v)
			local pck = write_format(true, "12", k, len)
			tlvbytes = tlvbytes .. pck .. v
		end
		tlvlen = string.len(tlvbytes)
	end
	if sys ~= QMI_CTL then
		ret = write_format(true, "121111222", 1, 12 + tlvlen, 0, sys, cid, 0, tid, msgid, tlvlen) .. tlvbytes
		tid = tid + 1
		return ret
	end
	ret = write_format(true, "121111122", 1, 11 + tlvlen, 0, QMI_CTL, 0, 0, tid, msgid, tlvlen) .. tlvbytes
	tid = tid + 1
	return ret
end
--done
function decode_qmi(packet)
	local ret = {}
	if packet == nil then
		return ret
	end
	ret["tf"], ret["len"], ret["ctrl"], ret["sys"], ret["cid"] = read_format(true, "12111", packet)
	if ret["tf"] ~= 1 then
		return {}
	end
	local substr = string.sub(packet, 7)
	local tlvlen, tlvs, tlv, len
	if ret["sys"] == QMI_CTL then
		ret["flags"], ret["tid"], ret["msgid"], ret["tlvlen"] = read_format(true, "1122", substr)
		tlvlen = ret["tlvlen"]
		tlvs = string.sub(packet, 13)
	else
		ret["flags"], ret["tid"], ret["msgid"], ret["tlvlen"] = read_format(true, "1222", substr)
		tlvlen = ret["tlvlen"]
		tlvs = string.sub(packet, 14)
	end
	local toff = {}
	while tlvlen > 0 do
		tlv, len = read_format(true, "12", tlvs)
		substr = string.sub(tlvs, 4)
		local toft = {}
		for i = 1, len do
			toft[i] = read_format(true, "1", substr)
			substr = string.sub(substr, 2)
		end
		toff[tlv] = toft
		tlvlen = tlvlen - (len + 3)
		tlvs = string.sub(tlvs, len + 4)
	end
	ret["tlvs"] = toff
	return ret
end
--done
function qmi_match(q1, q2)
	if q1["tf"] == nil or q2["tf"] == nil or q1["tf"] ~= q2["tf"] then
		return nil 
	end
	if q1["ctrl"] == nil or q2["ctrl"] == nil or q1["ctrl"] ~= q2["ctrl"] then
		return nil 
	end
	if q1["flags"] == nil or q2["flags"] == nil or q1["flags"] ~= q2["flags"] then
		return nil 
	end
	if q1["sys"] == nil or q2["sys"] == nil or q1["sys"] ~= q2["sys"] then
		return nil 
	end
	if q1["cid"] == nil or q2["cid"] == nil or q1["cid"] ~= q2["cid"] then
		return nil 
	end
	if q1["msgid"] == nil or q2["msgid"] == nil or q1["msgid"] ~= q2["msgid"] then
		return nil 
	end
	return 1
end
--done
function read_match(match, timeout)
	local qmi_in = {}
	local qmi_nil = {}
	local ret = {}
	local raw, raw1, raw2, found

	rserial = assert(io.open(dev, "r+b"))
	for i = 1, 20 do
		raw = rserial:read(6)
		ret["tf"], ret["len"], ret["ctrl"], ret["sys"], ret["cid"] = read_format(true, "12111", raw)
		if ret["sys"] == QMI_CTL then
			raw1 = rserial:read(6)
			ret["flags"], ret["tid"], ret["msgid"], ret["tlvlen"] = read_format(true, "1122", raw1)
		else
			raw1 = rserial:read(7)
			ret["flags"], ret["tid"], ret["msgid"], ret["tlvlen"] = read_format(true, "1222", raw1)
		end
		raw2 = rserial:read(ret["tlvlen"])
		raw = raw .. raw1 .. raw2
		qmi_in = decode_qmi(raw)
		if debug == 1 then
			printf("\nResponse QMUX Header Match\n")
			printf("tf        : %02x %02x\n", match["tf"], qmi_in["tf"])
			printf("ctrl      : %02x %02x\n", match["ctrl"], qmi_in["ctrl"])
			printf("service   : %02x %02x\n", match["sys"], qmi_in["sys"])
			printf("client id : %02x %02x\n", match["cid"], qmi_in["cid"])
			printf("Response QMUX SDU Match\n")
			printf("flags     : %02x %02x\n", match["flags"], qmi_in["flags"])
			printf("msg #     : %02x %02x\n", match["tid"], qmi_in["tid"])
			printf("msg id    : %02x %02x\n", match["msgid"], qmi_in["msgid"])
			debug_print("tlvs data :", raw2)
		end
		if qmi_match(match, qmi_in) == 1 then
			rserial:flush()
			rserial:close()
			return qmi_in
		end
	end
	rserial:flush()
	rserial:close()
	return qmi_nil
end

--done
function send_recv(cmd, timeout)
	local qmi_in = {}
	if cmd == nil then
		return qmi_in
	end
	if debug == 1 then
		printf("\n")
		debug_print("Message Sent :", cmd)
	end

	rserial = assert(io.open(dev, "r+b"))
	rserial:flush()
	rserial:close()

	wserial = assert(io.open(dev, "w+b"))
	sleep(1)
	wserial:write(cmd)
	sleep(1)
	wserial:flush()
	sleep(1)
	wserial:close()

	local qmi_out = decode_qmi(cmd)
	qmi_out["flags"] = 1
	if qmi_out["sys"] > 0 then
		qmi_out["flags"] = 2
	end
	qmi_out["ctrl"] = 0x80
	qmi_in = read_match(qmi_out, timeout)
	return qmi_in
end
--done
function verify_status(qmi, flag)
	local toff = {}
	local tofi = {}
	if qmi["tf"] == nil then
		return 1
	end
	if qmi["tlvs"] == nil then
		if debug == 1 then
			printf("\nPacket Error Status missing, no error\n")
		end
		return 0
	end
	toff = qmi["tlvs"]
	if toff[2] == nil then
		if debug == 1 then
			printf("\nPacket Error Status missing, no error\n")
		end
		return 0
	end
	tofi = toff[2]
	local ver = (256 * tofi[4]) + tofi[3]
	if debug == 1 then
		if ver == 0 then
			printf("\nNo Packet Error\n")
		else
			printerror(ver)
		end
	end
	return ver
end

function ctl_sync()
	local ret = {}
	local req = mk_qmi(QMI_CTL, 0, 0x0027)

	debug_print("send_recv", req)
	wserial = assert(io.open(dev, "w+b"))
	wserial:write(req)
	wserial:flush()
	wserial:close()
	rserial = assert(io.open(dev, "r+b"))
	rserial:flush()
	rserial:close()
	cid = {}
	wds_handle = nil
	return 0
end

function ctl_sync1()
	local ret = {}
	local req = mk_qmi(QMI_CTL, 0, 0x0027)
	ret = send_recv(req)
	local status = verify_status(ret)
	if status == 0 then
		cid = {}
		wds_handle = nil
	end
	return status
end

function get_cid(sys)
	local ret = {}
	local toff = {}
	local tofi = {}
	if cid[sys] ~= nil then
		return cid[sys]
	end
	local pck = {}
	pck[1] = write_format(true, "1", sys)
-- QMI_CTL request client ID
	local req = mk_qmi(QMI_CTL, 0, 0x0022, pck)
	for i=1,3 do
		ret = send_recv(req, 1)
		local status = verify_status(ret, 1)
		toff = ret["tlvs"]
		if toff == nil then
			return nil
		end
		tofi = toff[1]
		if status == 0 and tofi[1] == sys then
			cid[sys] = tofi[2]
			if debug == 1 then
				printf("\nClient ID for %s is %d\n", cidname[sys], cid[sys])
			end
			return cid[sys]
		end
		if status ~= 5 then
			if debug == 1 then
				printf("%s: CID request for %s failed: %d\n", netdev, cidname[sys], status)
			end
			return nil
		end
		if ctl_sync() ~= 0 then
			if debug == 1 then
				printf("%s: CID request for %s failed: %d\n", netdev, cidname[sys], status)
			end
			return nil
		end
	end
	return nil
end
--done
function mk_dms(cmd, tlvs)
	local cidd = get_cid(QMI_DMS)
	if cidd == nil then
		return nil
	end
	local req = mk_qmi(QMI_DMS, cidd, cmd, tlvs)
	return req
end

function mk_nas(parm1, parm2)
	local cidd = get_cid(QMI_NAS)
	if cid == nil then
		return nil
	end
	return mk_qmi(QMI_NAS, cidd, parm1, parm2)
end

local clock = os.clock
function sleep(n) --seconds
	local t0 = clock()
	while clock() - t0 <= n do end
end

--done
function dms_get_device_rev_id()
	local ret = send_recv(mk_dms(0x23), 1)
	sleep(5)
	local toff = {}
	toff = ret["tlvs"]
	if toff == nil then
		return nil
	end
	local v = toff[1]
	local i = 1
	local rev = ''
	repeat
		if v[i] == nil then
			break
		end
		rev = rev ..string.char(v[i])
		i = i + 1
	until 1 == 0
	return rev
end

-- sys is QMI_DMS, QMIWDS or QMI_NAS
-- parm1 is extra parameter for NAS or WDS. May be nil
-- cmd is command to execute
-- tlvs is returned, nil if verify error
--
function get_device_tlvs(sys, cmd, parm1)
	local mk = nil
	if sys == QMI_DMS then
		mk = mk_dms(cmd)
	end
	if sys == QMI_WDS then
		mk = mk_wds(cmd, parm1)
	end
	if sys == QMI_NAS then
		mk = mk_nas(cmd, parm1)
	end
	local ret = send_recv(mk, 1)
	local status = verify_status(ret, 1);
	if status > 0 then
		pkterror = status
		return nil
	end
	local toff = {}
	toff = ret["tlvs"]
	return toff
end

-- toff is table containing data
-- index is index of data chunk
--
function extract_data(toff, index)
	local v = toff[index]
	if v == nil then
		return nil
	end
	local i = 1
	local rev = ''
	repeat
		if v[i] == nil then
			break
		end
		rev = rev ..string.char(v[i])
		i = i + 1
	until 1 == 0
	return rev
end

function wait_for_sync_ind()
	local match = {}
	match["tf"] = 1
	match["sys"] = 0
	match["cid"] = 0
	match["flags"] = 0x02
	match["ctrl"] = 0x80
	match["msgid"] = 0x27
	qmi_in = read_match(match, 1)
end

function dms_enter_pin()
	if pincode ~= nil then
		plen = string.len(pincode)
		pcode = write_format(true, "11", 1, plen) .. pincode
		local tlv = {}
		tlv[1] = pcode
		local ret = send_recv(mk_dms(0x28, tlv), 1)
		local status = verify_status(ret, 1);
		if status > 0 then
			printf("%s:      PIN1 verification failed\n", netdev)
			if status == 3 then
				printf("%s:      SIM card missing?\n", netdev)
			end
			return 0
		end
		sleep(20)
--		wait_for_sync_ind()
		return 1
	end
	return 0
end

function dms_verify_pin()
	local ret = send_recv(mk_dms(0x2b), 1)
	local status = verify_status(ret, 1);
	if status > 0 then
		printf("%s:      PIN1 verification failed\n", netdev)
		if status == 3 then
			printf("%s:      SIM card missing?\n", netdev)
		end
		return 0
	end
	local toff = ret["tlvs"]
	local tlvs = toff[0x11]
	printf("%s:      PIN1 status is : %s\n",netdev, pinstat[tlvs[1]])
	printf("%s:      PIN1 # verify left: %d\n",netdev, tlvs[2])
	printf("%s:      PIN1 # unblock left: %d\n",netdev, tlvs[3])
	if tlvs[1] == 2 or tlvs[1] ==3 then
		return 1
	end
	if tlvs[1] == 1 then
		if tlvs[2] >= 3 then
			return dms_enter_pin()
		end
	end
	return 0
end
--done
function mk_wds(parm1, parm2)
	local cidd = get_cid(QMI_WDS)
	if cid == nil then
		return nil
	end
	return mk_qmi(QMI_WDS, cidd, parm1, parm2)
end

function wds_start_network_interface()
	printf("%s:   Checking Pin Status\n", netdev)
	if dms_verify_pin() == 0 then
		printf("%s:   ***Cannot Connect without PIN verification\n", netdev)
		return 1
	end
	printf("%s:   Attempting to Connect ....\n", netdev)
	local tlv = {}
	if apn ~= nil then
		tlv[0x14] = apn
	else
		tlv[0x31] = 0
		tlv[0x32] = 0
	end
	if user ~= nil then
		tlv[0x17] = user
	end
	if pw ~= nil then
		tlv[0x18] = pw
	end
	if auth ~= nil then
		if auth == "1" then
			tlv[0x16] = 1
		end
		if auth == "2" then
			tlv[0x16] = 2
		end
	end
	local req = mk_wds(0x20, tlv)
	local ret = send_recv(req, 1)
	local status = verify_status(ret, 1);
	if status > 0 then
		if errmsg[status] == nil then
			printf("%s:     ***Connection failed - error %04x\n", netdev, status)
		else
			printf("%s:     ***Connection failed because : %s\n", netdev, errmsg[status])
		end
		return status
	end
	local toff = ret["tlvs"]
	local tofi = toff[1]
	local ss = write_format(true, "1111", tofi[1], tofi[2], tofi[3], tofi[4])
	wds_handle = read_format(true, "4", ss)
	if debug == 1 then
		printf("%s: got QMI_WDS handle %08x\n", netdev, wds_handle)
	end
	printf("%s:   Connected to Network\n", netdev)
	return 0
end

function wds_get_pkt_srvc_status()
	local req = mk_wds(0x22)
	local ret = send_recv(req, 1)
	local status = verify_status(ret, 1);
	if status > 0 then
		printerror(status)
		printf("%s:   ***Not connected due to Packet Error\n", netdev)
		return 0
	end
	local toff = ret["tlvs"]
	local v = toff[1]
	if v ~= nil then
		return v[1]
	else
		printf("%s:   ***Not connected due to Packet Error\n", netdev)
		return 0
	end
end

function release_cids()
	printf("%s: Cleaning Up Script\n", netdev)
	local flag = 0
	for sys=0,5 do
		if cid[sys] ~= nil then
			if wds_handle ~= nil and sys == QMI_WDS then
				flag = 1
				if debug == 1 then
					printf("%s: not releasing QMI_WDS cid %d\n", netdev, cid[sys])
				end
			else
				local pck = {}
				pck[1] = write_format(true, "11", sys, cid[sys])
				local req = mk_qmi(QMI_CTL, 0, 0x0023, pck)
				local ret = send_recv(req, 1)
				local status = verify_status(ret, 1)
				cid[sys] = nil
			end
		end
	end
	if flag == 1 then
		os.remove(state)
	end
end

function save_wds_state()
	local file = io.open(state, "w")
	if file == nil then
		if debug == 1 then
			printf("%s: Can't Open file: %s\n",netdev, state)
			wds_handle = nil
			return
		end
	end
	if wds_handle ~= nil and cid[QMI_WDS] ~= nil then
		file:write(cid[QMI_WDS], " ")
		file:write(wds_handle)
		file:close()
	else
		file:close()
		os.remove(state)
	end
end

function read_wds_state()
	local file = io.open(state, "r")
	if file == nil then
		if debug == 1 then
			printf("%s: Can't Open file: %s\n",netdev, state)
		end
		return
	end
--	local pkt = wds_get_pkt_srvc_status()
	cid[QMI_WDS] = file:read("*n")
	wds_handle = file:read("*n")
	local pkt = wds_get_pkt_srvc_status()
	if pkt ~= 2 then
		wds_handle = nil
	end
	if pkt == 0 then
		cid[QMI_WDS] = nil
	end
	if debug == 1 and wds_handle ~= nil and cid[QMI_WDS] ~= nil then
		printf("%s: QMI_WDS cid=%d wds_handle=%08x\n",netdev, cid[QMI_WDS], wds_handle)
	end
	file:close()
end

function wds_stop_network_interface()
	if wds_handle == nil then
		return 0
	end
	local tlv = {}
	tlv[0x01] = write_format(true, "4", wds_handle)
	local req = mk_wds(0x0021, tlv)
	local ret = send_recv(req, 1)
	wds_handle = nil
	return verify_status(ret)
end

-- main

--
-- To disable echoing output to System Log when the command is start or stop
-- change echo = 1 to echo = 0
--
if cmd == "start" or cmd == "stop" then
	echo = 1
end

printf("%s: ===Starting QMI script===\n", netdev)
printf("%s: Will use %s as management port\n", netdev, dev)

revid = dms_get_device_rev_id()
if revid == nil then
	printf("%s: Revision ID has failed\n",netdev)
else
	printf("%s: Qmi Revision: %s\n",netdev, revid)
end
	printf("%s: Script Command : %s\n",netdev, cmd)
	read_wds_state()

	if cmd == "start" then
		printf("%s: Attempt to Start Network\n",netdev)
		stat = wds_start_network_interface()
		sleep(5)
		if stat == 0 then
			pkt = wds_get_pkt_srvc_status()
			if pkt > 0 then
				printf("%s:   Connection Status is : %s\n", netdev, pktmsg[pkt])
			end
			if pkt == 2 then
				file = io.open("/tmp/qmigood", "w")
				file:write("1")
				file:close()
			end
		end
	end

	if cmd == "status" then
		pkt = wds_get_pkt_srvc_status()
		printf("%s: Connection Status is : %s\n", netdev, pktmsg[pkt])
		save_wds_state()
		release_cids()
		return pkt
	end

	if cmd == "stop" then
		printf("%s: Attempt to Stop Network\n",netdev)
		if wds_handle ~= nil then
			local dis = wds_stop_network_interface()
			if dis > 0 then
				printf("%s: Error While Disconnecting\n", netdev)
			end
		end
	end

	if cmd == "other" then
		printf("%s:    SubScript Command : %s\n",netdev, apn)
		if apn == "cdma" then
--Device Capabilities
			local tlvs = {}
			tlvs = get_device_tlvs(QMI_DMS, 0x20)
			if tlvs ~= nil then
				rev = extract_data(tlvs, 0x01)
				max_tx, max_rx, dsc, sim, rf_len, rf_list = read_format(true, "441111", rev)
				printf("%.2f %.2f %d %d %d %d\n", max_tx/1000000, max_rx/1000000, dsc, sim, rf_len, rf_list)
			else
				printerror(pkterror)
			end
		end
		if apn == "mfr" then
			local tlvs = {}
			tlvs = get_device_tlvs(QMI_DMS, 0x21)
			mfr = extract_data(tlvs, 0x01)
			if mfr ~= nil then
				tlvs = get_device_tlvs(QMI_DMS, 0x2c)
				mod = extract_data(tlvs, 0x01)
				printf("%s: Manufacturer: %s Model: %s\n",netdev, mfr, mod)
			end
			debug = 1
-- signal strength
			local tlv = {}
			tlv[0x10] = write_format(true, "2", 0x01)

			tlvs = get_device_tlvs(QMI_NAS, 0x20, tlv)
			rev = extract_data(tlvs, 0x01)
			sig, serv = read_format(true, "11", rev)
			printf("Server Type : %d Signal Value : %d\n", serv, sig)

-- tx, rx speed in bits per sec
--			tlvs = get_device_tlvs(QMI_WDS, 0x23)
--			rev = extract_data(tlvs, 0x01)
--			tx, rx, txm, rxm = read_format(true, "4444", rev)
--			printf("%d %d %d %d\n", tx, rx, txm, rxm)
			debug = 0
		end
	end

save_wds_state()
release_cids()
printf("%s: ===End of QMI Script===\n", netdev)




