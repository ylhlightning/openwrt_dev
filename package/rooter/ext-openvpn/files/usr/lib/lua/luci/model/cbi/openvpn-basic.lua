--[[
LuCI - Lua Configuration Interface

Copyright 2008 Steven Barth <steven@midlink.org>
Copyright 2013 Christian Richarz <christian.richarz@multidata.de>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

$Id: openvpn-basic-lua 9999 2013-06-27 13:01
]]--
  
require( "luci.model.uci" )
 
name = arg[1]
m = Map( "openvpn" )
p = m:section( SimpleSection )

p.template = "openvpn/pageswitch"
p.mode     = "basic"
p.instance = name

s = m:section( NamedSection, name, "openvpn" )

-- enabling/disabling the "client" flag toggles the visibility of some parameters
-- specific to either client or server mode
local client = s:option( Flag, "client", translate( "Configure client mode" ) )
  client.value = "1"
  
-- Client-only parameters  
local remote = s:option( Value, "remote", translate("Remote host name or ip address") )
  remote:value( "vpnserver.example.org" )
  remote:depends( "client", "1" )
local nobind = s:option( Flag, "nobind", translate("Do not bind to local address and port") )
  nobind.value = "0"
  nobind:depends( "client", "1" )
local persist_key = s:option( Flag, "persist_key", translate( "Don't re-read key on restart" ) )
  persist_key.optional = true
  persist_key:depends( "client" , "1" )
local persist_tun = s:option( Flag, "persist_tun", translate( "Keep tun/tap device open on restart" ) )
  persist_tun.optional = true
  persist_tun:depends( "client" , "1" )
local remote_cert_tls = s:option( ListValue, "remote_cert_tls", translate( "Require explicit key usage on certificate" ) )  
  remote_cert_tls:value( "server" )
  remote_cert_tls:value( "client" )
  remote_cert_tls:depends( "client" , "1" )
  remote_cert_tls.optional = true
local float  = s:option( Flag, "float", translate( "Allow remote to change its IP or port" ) )
  float:depends( "client", "1" )
local pkcs12 = s:option( FileUpload, "pkcs12", translate("PKCS#12 file containing keys") )
  pkcs12.optional = true
  pkcs12:depends( "client", "1" )
local pull = s:option(  Flag, "pull", translate( "Accept options pushed from server" ) )
    pull.default = "1"
    pull:depends ( "client", "1" )

-- Server-only parameters
local keepalive = s:option( Value,"keepalive", translate("Helper directive to simplify the expression of --ping and --ping-restart in server mode configurations") )
  keepalive.value = "10 60"
  keepalive:depends( "client", "" )
local client_to_client = s:option( Flag, "client_to_client", translate("Allow client-to-client traffic") )
  client_to_client.optional = true
  client_to_client:depends( "client", "" )  
local server = s:option( Value, "server", translate("Configure server mode") )
  server.value = "10.10.0.0 255.255.255.0"
  server:depends( { client = "", dev_type = "tun" } )
  server.description = translate( "Serverside_Network_IP Netmask" )
local server_bridge = s:option( Value,"server_bridge", translate("Configure server bridge") )
  server_bridge.value = "192.168.1.1 255.255.255.0 192.168.1.128 192.168.1.254"
  server_bridge:depends( { client = "", dev_type = "tap" } )
  server_bridge.description = translate( "Server_IP_Address Netmask First_Client_IP Last_Client_IP" )
  
-- Parameters for server and client  
local proto = s:option( ListValue,"proto", translate("Use protocol") )
  proto:value( "tcp" )
  proto:value( "udp" )
local port = s:option( Value, "port", translate("TCP/UDP port # for both local and remote") )
  port.value = "1194"
local dev_type = s:option( ListValue, "dev_type", translate("Type of used device") )
  dev_type:value( "tun" )
  dev_type:value( "tap" )
  dev_type.description = translate( "Use tun for routing based connections and tap for bridging" )
local ifconfig = s:option( Value,"ifconfig", translate("Set tun/tap adapter parameters (ifconfig)") )
  ifconfig.value =  "10.10.0.1 255.0.0.0"
  ifconfig:depends( "pull", "" )
  ifconfig:depends( "client", "" )
  ifconfig.description = translate( "Interface_IP_Address Netmask" )
local secret = s:option( FileUpload, "secret", translate("Enable Static Key encryption mode (non-TLS)") )
  secret.optional = true
local ca = s:option( FileUpload, "ca", translate("Certificate authority") )
  ca.optional = true
local dh = s:option( FileUpload, "dh", translate("Diffie Hellman parameters") )
  dh.optional = true
local cert = s:option( FileUpload, "cert", translate("Local certificate") )
  cert.optional = true
local key = s:option( FileUpload, "key", translate("Local private key") )
  key.optional = true
local comp_lzo = s:option( ListValue, "comp_lzo", translate("Use fast LZO compression") )
  comp_lzo:value( "yes" )
  comp_lzo:value( "no" )
  comp_lzo:value( "adaptive" )
local verb = s:option( ListValue, "verb", translate( "Verbosity") )
    verb:value( "0" )
    verb:value( "1" )
    verb:value( "2" )
    verb:value( "3" )
    verb:value( "4" )
    verb:value( "5" )
    verb:value( "6" )
    verb:value( "7" )
    verb:value( "8" )
    verb:value( "8" )
    verb:value( "10" )
    verb:value( "11" )
    verb.default = "3"
local nice = s:option( Value, "nice", translate("Change process priority") )
  nice.optional = true
local tun_ipv6 = s:option( Flag,"tun_ipv6", translate("Make tun device IPv6 capable") )
  tun_ipv6.optional = true

return m

