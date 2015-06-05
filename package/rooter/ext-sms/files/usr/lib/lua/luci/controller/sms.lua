module("luci.controller.sms", package.seeall)

function index()
	local page
	page = entry({"admin", "modem", "sms"}, template("rooter/sms"), "SMS Messaging", 6)
	page.dependent = true

	entry({"admin", "modem", "check_read"}, call("action_check_read"))
	entry({"admin", "modem", "del_sms"}, call("action_del_sms"))
	entry({"admin", "modem", "send_sms"}, call("action_send_sms"))
	entry({"admin", "modem", "change_sms"}, call("action_change_sms"))
	entry({"admin", "modem", "change_smsdn"}, call("action_change_smsdn"))
end

function action_send_sms()
	smsnum = luci.model.uci.cursor():get("modem", "general", "smsnum")
	local set = luci.http.formvalue("set")
	number = trim(string.sub(set, 1, 20))
	txt = string.sub(set, 21)
	msg = string.gsub(txt, "\n", " ")
	os.execute("/usr/lib/sms/sendsms.sh " .. smsnum .. " " .. number .. " " .. "\"\13" .. msg .. "\26\"")
end

function action_del_sms()
	local set = tonumber(luci.http.formvalue("set"))
	if set ~= nil and set > 0 then
		set = set - 1;
		smsnum = luci.model.uci.cursor():get("modem", "general", "smsnum")
		os.execute("/usr/lib/sms/delsms.sh " .. set .. " " .. smsnum)
	end
end

function action_check_read()
	local rv ={}
	local file
	smsnum = luci.model.uci.cursor():get("modem", "general", "smsnum")
	conn = "Modem #" .. smsnum
	rv["conntype"] = conn
	support = luci.model.uci.cursor():get("modem", "modem" .. smsnum, "sms")
	rv["ready"] = "0"
	if support == "1" then
		rv["ready"] = "1"
		result = "/tmp/smsresult" .. smsnum .. ".at"
		file = io.open(result, "r")
		if file ~= nil then
			file:close()
			os.execute("/usr/lib/sms/smsread.lua " .. smsnum)
			file = io.open("/tmp/smstext", "r")
			if file == nil then
				rv["ready"] = "3"
			else
				rv["ready"] = "2"
				local tmp = file:read("*line")
				rv["used"] = tmp
				tmp = file:read("*line")
				rv["max"] = tmp
				full = nil
				repeat
					line = file:read("*line")
					if line ~= nil then
						if full == nil then
							full = line
						else
							full = full .. "|" .. line 
						end
					end
				until line == nil
				file:close()

				rv["line"] = full
			end
		end
	end

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

function action_change_sms()
	os.execute("/usr/lib/rooter/luci/modemchge.sh sms 1")
end

function action_change_smsdn()
	os.execute("/usr/lib/rooter/luci/modemchge.sh sms 0")
end