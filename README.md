lua-fb
-------

Use drawbuffers on the linux framebuffer!
This library enables getting information about a framebuffer device, and
drawing to it using drawbuffers. For details about some of the fields
mentioned here, please look at the relevant kernel documentation.

This library exports one function:

	lfb.new(path) --> fb

Path should be a path to a framebuffer device(such as /dev/fb0)
The returned framebuffer has the following metamethods:





	fb:get_var_info()

Get the variable info. Contains e.g. the xres and yres fields.



	fb:get_fix_info()

Gets the fixed info from the framebuffer device.



	fb:set_var_info(varinfo)

Sets the fields in the varinfo table via IOCTL.



	fb:draw_from_drawbuffer(db, x, y)

Draws the drawbuffer db to x,y on the framebuffer
