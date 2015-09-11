module("luci.controller.nodogsplash.splash", package.seeall)

require "luci.i18n"

function index()
	entry({"admin", "services", "splash"}, cbi("nodogsplash/splash_settings"), _("Captive Portal"), 90).dependent=true
  	entry({"nodogsplash", "splash"}, template("nodogsplash/splash")).dependent=false
end
