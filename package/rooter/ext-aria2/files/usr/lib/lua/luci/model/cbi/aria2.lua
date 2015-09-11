--[[
RA-MOD
]]--

local fs = require "nixio.fs"
local util = require "nixio.util"

local running=(luci.sys.call("pidof aria2c > /dev/null") == 0)

local_web="&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" value=\" " .. "Local Web" .. " \" onclick=\"window.open('http://'+window.location.host+'/aria2')\"/>"

if running then
	m = Map("aria2", translate("Aria2 Downloader"), translate("HTTP/FTP File Downloader (running)") .. local_web)

else
	m = Map("aria2", translate("Aria2 Downloader"), translate("HTTP/FTP File Downloader (not running)"))
end

m.on_after_commit = function(self)
	enabe = luci.model.uci.cursor():get("aria2", "aria2", "enable")
	if enabe == "1" then
		os.execute("/etc/init.d/aria2 stop")
		os.execute("/etc/init.d/aria2 enable")
		os.execute("/etc/init.d/aria2 start")
	else
		os.execute("/etc/init.d/aria2 stop")
	end
	luci.http.redirect(luci.dispatcher.build_url("admin/services/aria2"))
end

s = m:section(TypedSection, "aria2", translate("Settings"))
s.anonymous = true

s:tab("basic",  translate("Basic Settings"))

switch = s:taboption("basic", Flag, "enable", translate("Enable"))
switch.rmempty = false

download_folder = s:taboption("basic", Value, "dir", translate("Download Folder"), translate("Where Your Files are Saved"))
download_folder.default = "/mnt/sda3/downloads"
download_folder.placeholder = "/mnt/sda3/downloads"

maxjobs = s:taboption("basic", Value, "max_concurrent_downloads", translate("Max Concurrent Queue"), translate("Default 5"))
maxjobs.default = "5"
maxjobs.placeholder = "5"
maxjobs.datatype = "uinteger"

diskcache = s:taboption("basic", ListValue, "disk_cache", translate("Enable Disk Cache"))
diskcache:value("1M")
diskcache:value("2M")
diskcache:value("4M")
diskcache:value("8M")

filealloc = s:taboption("basic", ListValue, "file_allocation", translate("File Allocation Method"))
filealloc:value("none")
filealloc:value("prealloc")
filealloc:value("trunc")
filealloc:value("falloc")

maxdl = s:taboption("basic", Value, "max_overall_download_limit", translate("Max Download Speed"), translate("Unrestricted is 0"))
maxdl.default = "0"

aresume = s:taboption("basic", ListValue, "always_resume", translate("Always Resume Download"))
aresume:value("1", "true")
aresume:value("0", "false")
aresume.default="1"

http=m:section(TypedSection, "http_ftp", translate("HTTP / FTP"))
http.anonymous = true

http:tab("basic",  translate("Basic Settings"))

split = http:taboption("basic", Value, "split", translate("Download Connections"), translate("Default 5"))
split.default = "5"
split.placeholder = "5"
split.datatype = "uinteger"

mtries = http:taboption("basic", Value, "max_tries", translate("Maximum Retries"), translate("Default 5"))
mtries.default = "5"
mtries.placeholder = "5"
mtries.datatype = "uinteger"

rtries = http:taboption("basic", Value, "retry_wait", translate("Wait before Retry"), translate("In Seconds"))
rtries.default = "0"
rtries.placeholder = "0"
rtries.datatype = "uinteger"

maxcon = http:taboption("basic", Value, "max_connection_per_server", translate("Max Connections per Server"), translate("Default 1"))
maxcon.default = "1"
maxcon.placeholder = "1"
maxcon.datatype = "uinteger"

rpc=m:section(TypedSection, "rpc", translate("RPC"))
rpc.anonymous = true

rpc:tab("basic",  translate("Basic Settings"))

rpcswitch = rpc:taboption("basic", Flag, "enable_rpc", translate("Enable"))
rpcswitch.rmempty = false

aorgin = rpc:taboption("basic", ListValue, "rpc_allow_origin_all", translate("Add Access Control Header"))
aorgin:value("1", "true")
aorgin:value("0", "false")

alisten= rpc:taboption("basic", ListValue, "rpc_listen_all", translate("Listen On All Interfaces"))
alisten:value("1", "true")
alisten:value("0", "false")

lport= rpc:taboption("basic", Value, "rpc_listen_port", translate("Listen On Port"), translate("Default is 6800"))
lport.datatype=uninteger
lport.rmempty = true

return m
