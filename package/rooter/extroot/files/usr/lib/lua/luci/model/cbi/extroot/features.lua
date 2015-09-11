local utl = require "luci.util"

local maxfeat = luci.model.uci.cursor():get("repository", "package", "number")

m = Map("repository", translate("Add New Features to the Router"), translate("Install new router features"))

m.on_after_commit = function(self)
	os.execute("/etc/checkpack")
	file = io.open("/tmp/packdata", "r")
	if file ~= nil then
		file:close()
		luci.http.redirect(luci.template.render("admin_system/restart"))
	end
end

if maxfeat == 0 then
	return m
end

pack = m:section(TypedSection, "package", translate("Repository Version"))
pack.anonymous = true
version = pack:option(DummyValue, "version")

local totfeat = luci.model.uci.cursor():get("repository", "feature", "total")
local totinst = luci.model.uci.cursor():get("repository", "installed", "total")

ft = {}
ma = {}
it = {}

di = m:section(TypedSection, "flg", "Availiable Features")

for i=1,totfeat do
	stri = string.format("%d", i)
	finfo = "f" .. stri
	ft[i] =  luci.model.uci.cursor():get("repository", "feature", finfo)
	ma[i] = di:option(ListValue, finfo, ft[i])
	ma[i]:value("0", "Don't Install")
	ma[i]:value("1", "Install")
end

dx = m:section(TypedSection, "repos", " ")

if totinst ~= "0" then
	for j=1,totinst do
		stri = string.format("%d", j)
		finfo = "f" .. stri
		it[j] = dx:option(DummyValue, finfo, "")
	end
end

return m
