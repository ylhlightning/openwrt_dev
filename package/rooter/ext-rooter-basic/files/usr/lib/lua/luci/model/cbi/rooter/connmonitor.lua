local utl = require "luci.util"

m = Map("modem", translate("Modem Connection Monitoring"), translate("Use Pinging to keep the Modem Connection Working"))

m.on_after_commit = function(self)
	--luci.sys.call("/etc/monitor")
	--luci.sys.call("/etc/pingchk")
end

d = m:section(TypedSection, "pinfo", "Ping Monitoring")

c1 = d:option(ListValue, "alive", "MONITOR WITH OPTIONAL RESTART :");
c1:value("0", "Disabled")
c1:value("1", "Enabled")
c1:value("2", "Enabled with Router Reboot")
c1:value("3", "Enabled with Modem Restart")
c1:value("4", "Enabled with Power Toggle or Modem Restart")
c1.default=0

ca3 = d:option(Value, "pingtime", "Ping Interval in Minutes :"); 
ca3.optional=false; 
ca3.rmempty = true;
ca3.datatype = "and(uinteger,min(1))"
ca3.default=2
ca3:depends("alive", "1")
ca3:depends("alive", "2")
ca3:depends("alive", "3")
ca3:depends("alive", "4")

ca4 = d:option(Value, "pingwait", "Ping Wait in Seconds :"); 
ca4.optional=false; 
ca4.rmempty = true;
ca4.datatype = "and(uinteger,min(1))"
ca4.default=10
ca4:depends("alive", "1")
ca4:depends("alive", "2")
ca4:depends("alive", "3")
ca4:depends("alive", "4")

ca5 = d:option(Value, "pingnum", "Number of Packets :"); 
ca5.optional=false; 
ca5.rmempty = true;
ca5.datatype = "and(uinteger,min(1))"
ca5.default=1
ca5:depends("alive", "1")
ca5:depends("alive", "2")
ca5:depends("alive", "3")
ca5:depends("alive", "4")

cb2 = d:option(Value, "pingserv1", "Ping Server 1 :"); 
cb2.rmempty = true;
cb2.optional=false;
cb2.datatype = "ipaddr"
cb2.default="8.8.8.8"
cb2:depends("alive", "1")
cb2:depends("alive", "2")
cb2:depends("alive", "3")
cb2:depends("alive", "4")

cb3 = d:option(Value, "pingserv2", "Ping Server 2 :"); 
cb3.rmempty = true;
cb3.optional=false;
cb3.datatype = "ipaddr"
cb3:depends("alive", "1")
cb3:depends("alive", "2")
cb3:depends("alive", "3")
cb3:depends("alive", "4")

return m

