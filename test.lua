#!/usr/bin/env luajit
local ldb = require("lua-db")
local lfb = require("lua-fb")

print("creating db")
local db = ldb.new(100,100)
print("Got drawbuffer", db)

print("opening framebuffer")
local fb = lfb.new(arg[1] or "/dev/fb0")
print("Got framebuffer", fb)

print("getting varinfo")
local vinfo = fb:get_varinfo()
print("Framebuffer resolution ->", vinfo.xres .. "x" .. vinfo.yres)


print("getting fixinfo")
local finfo = fb:get_fixinfo()


print("clearing drawbuffer")
db:clear(0xFF,0x00,0xFF,0xFF)

print("drawing to framebuffer from drawbuffer")
fb:draw_from_drawbuffer(db, 0,0)
