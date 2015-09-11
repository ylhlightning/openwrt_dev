module("luci.controller.admin.modem", package.seeall)

function index() 
	entry({"admin", "modem"}, firstchild(), "Modem", 30).dependent=false
	entry({"admin", "modem", "cinfo"}, cbi("rooter/connection", {autoapply=true}), "Connection Info", 1)
	entry({"admin", "modem", "conmon"}, cbi("rooter/connmonitor"), "Connection Monitoring", 2)
	entry({"admin", "modem", "nets"}, template("rooter/net_status"), "Network Status", 3)
	entry({"admin", "modem", "debug"}, template("rooter/debug"), "Debug Information", 4)
	entry({"admin", "modem", "cust"}, cbi("rooter/customize"), "Custom Modem Ports", 5)
	entry({"admin", "modem", "log"}, template("rooter/log"), "Connection Log", 7)
	entry({"admin", "modem", "misc"}, template("rooter/misc"), "Miscellaneous", 8)

	entry({"admin", "modem", "get_csq"}, call("action_get_csq"))
	entry({"admin", "modem", "change_port"}, call("action_change_port"))
	entry({"admin", "modem", "change_mode"}, call("action_change_mode"))
	entry({"admin", "modem", "change_modem"}, call("action_change_modem"))
	entry({"admin", "modem", "change_modemdn"}, call("action_change_modemdn"))
	entry({"admin", "modem", "change_misc"}, call("action_change_misc"))
	entry({"admin", "modem", "change_miscdn"}, call("action_change_miscdn"))
	entry({"admin", "modem", "get_log"}, call("action_get_log"))
	entry({"admin", "modem", "check_misc"}, call("action_check_misc"))
	entry({"admin", "modem", "pwrtoggle"}, call("action_pwrtoggle"))
	entry({"admin", "modem", "disconnect"}, call("action_disconnect"))
	entry({"admin", "modem", "connect"}, call("action_connect"))
	entry({"admin", "modem", "get_atlog"}, call("action_get_atlog"))
	entry({"admin", "modem", "send_atcmd"}, call("action_send_atcmd"))
end

function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

function action_get_atlog()
	local file
	local rv ={}

	file = io.open("/tmp/atlog", "r")
	if file ~= nil then
		local tmp = file:read("*all")
		rv["log"] = tmp
		file:close()
	else
		rv["log"] = "No entries in log file"
	end

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

function action_get_log()
	local file
	local rv ={}

	file = io.open("/usr/lib/rooter/log/connect.log", "r")
	if file ~= nil then
		local tmp = file:read("*all")
		rv["log"] = tmp
		file:close()
	else
		rv["log"] = "No entries in log file"
	end

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

function action_disconnect()
	local set = luci.http.formvalue("set")
	os.execute("/usr/lib/rooter/connect/disconnect.sh")
end

function action_connect()
	local set = luci.http.formvalue("set")
	miscnum = luci.model.uci.cursor():get("modem", "general", "miscnum")
	os.execute("/usr/lib/rooter/links/reconnect" .. miscnum .. " " .. miscnum)
end

function action_pwrtoggle()
	local set = luci.http.formvalue("set")
	os.execute("/usr/lib/rooter/pwrtoggle.sh " .. set)
end

function action_send_atcmd()
	local set = luci.http.formvalue("set")
	fixed = string.gsub(set, "\"", "~")
	os.execute("/usr/lib/rooter/luci/atcmd.sh \"" .. fixed .. "\"")
end

function action_check_misc()
	local rv ={}
	local file
	local active
	local connect

	miscnum = luci.model.uci.cursor():get("modem", "general", "miscnum")
	conn = "Modem #" .. miscnum
	rv["conntype"] = conn
	empty = luci.model.uci.cursor():get("modem", "modem" .. miscnum, "empty")
	if empty == "1" then
		active = "0"
	else
		active = luci.model.uci.cursor():get("modem", "modem" .. miscnum, "active")
		if active == "1" then
			connect = luci.model.uci.cursor():get("modem", "modem" .. miscnum, "connected")
			if connect == "0" then
				active = "1"
			else
				active = "2"
			end
		end
	end
	rv["active"] = active
	file = io.open("/tmp/gpiopin", "r")
	if file == nil then
		rv.gpio = "0"
	else
		rv.gpio = "1"
		line = file:read("*line")
		line = file:read("*line")
		if line ~= nil then
			rv.gpio = "2"
		end
		file:close()
	end

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

function lshift(x, by)
  return x * 2 ^ by
end

function rshift(x, by)
  return math.floor(x / 2 ^ by)
end

function action_change_mode()
	local set = tonumber(luci.http.formvalue("set"))
	local modemtype = rshift(set, 4)
	local temp = lshift(modemtype, 4)
	local netmode = set - temp
	os.execute("/usr/lib/rooter/luci/modechge.sh " .. modemtype .. " " .. netmode)
end

function action_change_port()
	local set = tonumber(luci.http.formvalue("set"))
	if set ~= nil and set > 0 then
		if set == 1 then
			os.execute("/usr/lib/rooter/luci/portchge.sh dwn")
		else
			os.execute("/usr/lib/rooter/luci/portchge.sh up")
		end
	end
end

function action_change_misc()
	os.execute("/usr/lib/rooter/luci/modemchge.sh misc 1")
end

function action_change_miscdn()
	os.execute("/usr/lib/rooter/luci/modemchge.sh misc 0")
end

function action_change_modem()
	os.execute("/usr/lib/rooter/luci/modemchge.sh modem 1")
end

function action_change_modemdn()
	os.execute("/usr/lib/rooter/luci/modemchge.sh modem 0")
end

function action_get_csq()
	modnum = luci.model.uci.cursor():get("modem", "general", "modemnum")
	local file
	stat = "/tmp/status" .. modnum .. ".file"
	file = io.open(stat, "r")

	local rv ={}

	rv["port"] = file:read("*line")
	rv["csq"] = file:read("*line")
	rv["per"] = file:read("*line")
	rv["rssi"] = file:read("*line")
	rv["modem"] = file:read("*line")
	rv["cops"] = file:read("*line")
	rv["mode"] = file:read("*line")
	rv["lac"] = file:read("*line")
	rv["lacn"] = file:read("*line")
	rv["cid"] = file:read("*line")
	rv["cidn"] = file:read("*line")
	rv["mcc"] = file:read("*line")
	rv["mnc"] = file:read("*line")
	rv["rnc"] = file:read("*line")
	rv["rncn"] = file:read("*line")
	rv["down"] = file:read("*line")
	rv["up"] = file:read("*line")
	rv["ecio"] = file:read("*line")
	rv["rscp"] = file:read("*line")
	rv["ecio1"] = file:read("*line")
	rv["rscp1"] = file:read("*line")
	rv["netmode"] = file:read("*line")
	rv["cell"] = file:read("*line")
	rv["modtype"] = file:read("*line")
	rv["conntype"] = file:read("*line")
	rv["channel"] = file:read("*line")

	file:close()

	cmode = luci.model.uci.cursor():get("modem", "modem" .. modnum, "cmode")
	if cmode == "0" then
		rv["netmode"] = "10"
	end	

	rssi = rv["rssi"]
	ecio = rv["ecio"]
	rscp = rv["rscp"]
	ecio1 = rv["ecio1"]
	rscp1 = rv["rscp1"]

	if ecio ~= "-" then
		rv["ecio"] = ecio .. " dB"
	end
	if rscp ~= "-" then
		rv["rscp"] = rscp .. " dBm"
	end
	if ecio1 ~= " " then
		rv["ecio1"] = " (" .. ecio1 .. " dB)"
	end
	if rscp1 ~= " " then
		rv["rscp1"] = " (" .. rscp1 .. " dBm)"
	end

	result = "/tmp/result" .. modnum .. ".at"
	file = io.open(result, "r")
	if file ~= nil then
		rv["result"] = file:read("*all")
		file:close()
		os.execute("/usr/lib/rooter/luci/luaops.sh delete /tmp/result" .. modnum .. ".at")
	else
		rv["result"] = " "
	end

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

