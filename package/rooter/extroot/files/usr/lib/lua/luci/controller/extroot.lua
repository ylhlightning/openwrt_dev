module("luci.controller.extroot", package.seeall)

function index()
	local page

	if nixio.fs.access("/etc/pass1") then
		page = entry({"admin", "system", "features"}, cbi("extroot/features", {autoapply=true}), "Add Features", 15)
		page.dependent = true
	end
	entry({"admin", "system", "install_package"}, call("action_install_package"))
	entry({"admin", "system", "check_package"}, call("action_check_package"))
end

function action_install_package()
	rv = {}
	os.execute("/etc/installpack")
	luci.sys.reboot()
	rv["install"] = "done"
	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end

function action_check_package()
	rv = {}
	os.execute("/etc/checkpack")
	file = io.open("/tmp/packdata", "r")
	if file == nil then
		rv["install"] = "0"
		rv["delay"] = "0"
	else
		tmp = file:read("*line")
		rv["install"] = tmp
		tmp = file:read("*line")
		rv["delay"] = tmp
		file:close()
	end
	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end
