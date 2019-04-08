#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <stdint.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "lua.h"
#include "lauxlib.h"

#include "lua-db.h"



#define VERSION "2.0"

#define LUA_T_PUSH_S_N(S, N) lua_pushstring(L, S); lua_pushnumber(L, N); lua_settable(L, -3);
#define LUA_T_PUSH_S_S(S, S2) lua_pushstring(L, S); lua_pushstring(L, S2); lua_settable(L, -3);
#define LUA_T_PUSH_S_CF(S, CF) lua_pushstring(L, S); lua_pushcfunction(L, CF); lua_settable(L, -3);
#define CHECK_FB(L, I, D) D=(framebuffer_t *)luaL_checkudata(L, I, "framebuffer"); if (D==NULL) { lua_pushnil(L); lua_pushfstring(L, "Argument %d must be a drawbuffer", I); return 2; }



typedef struct {
    int fd;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    char *fbdev;
    uint8_t *data;
} framebuffer_t;



static inline uint16_t getcolor_16bpp(framebuffer_t *lfb, uint8_t r, uint8_t g, uint8_t b) {
	// convert r,g,b values to a 16bpp color value
	return ((r >> (8 - lfb->vinfo.red.length)) << lfb->vinfo.red.offset)  |  ((g >> (8 - lfb->vinfo.green.length)) << lfb->vinfo.green.offset)  |  ((b >> (8 - lfb->vinfo.blue.length))   << lfb->vinfo.blue.offset);
}

static inline uint32_t getcolor_32bpp(framebuffer_t *lfb, uint8_t r, uint8_t g, uint8_t b) {
	// convert r,g,b values to 32bpp color value
	return (r << lfb->vinfo.red.offset) | (g << lfb->vinfo.green.offset) | (b << lfb->vinfo.blue.offset);
}

static void getnumfield32(lua_State *L, const char *key, uint32_t *dest) {
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (lua_isnumber(L, -1)) {
        *dest = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}


static int lfb_framebuffer_close(lua_State *L) {
    framebuffer_t *fb = (framebuffer_t *)lua_touserdata(L, 1);

    if (fb->fd >= 0) {
        close(fb->fd);
        fb->fd = -1;
        free(fb->fbdev);
    }

    return 0;
}

static int lfb_framebuffer_tostring(lua_State *L) {
	
    framebuffer_t *fb;
	CHECK_FB(L, 1, fb)

    if (fb->fbdev) {
        lua_pushfstring(L, "Framebuffer: %s (%s)", fb->fbdev, fb->finfo.id);
    } else {
        lua_pushfstring(L, "Closed framebuffer");
    }

    return 1;
    
}

static int lfb_framebuffer_get_varinfo(lua_State *L) {

    framebuffer_t *fb;
	CHECK_FB(L, 1, fb)
	
    ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo);

    lua_newtable(L);

    LUA_T_PUSH_S_N("xres", fb->vinfo.xres);
    LUA_T_PUSH_S_N("yres", fb->vinfo.yres);
    LUA_T_PUSH_S_N("xres_virtual", fb->vinfo.xres_virtual);
    LUA_T_PUSH_S_N("yres_virtual", fb->vinfo.yres_virtual);
    LUA_T_PUSH_S_N("xoffset", fb->vinfo.xoffset);
    LUA_T_PUSH_S_N("yoffset", fb->vinfo.yoffset);
    LUA_T_PUSH_S_N("bits_per_pixel", fb->vinfo.bits_per_pixel);
    LUA_T_PUSH_S_N("grayscale", fb->vinfo.grayscale);
    LUA_T_PUSH_S_N("nonstd", fb->vinfo.nonstd);
    LUA_T_PUSH_S_N("activate", fb->vinfo.activate);
    LUA_T_PUSH_S_N("width", fb->vinfo.width);
    LUA_T_PUSH_S_N("height", fb->vinfo.height);

    LUA_T_PUSH_S_N("pixclock", fb->vinfo.pixclock);
    LUA_T_PUSH_S_N("left_margin", fb->vinfo.left_margin);
    LUA_T_PUSH_S_N("right_margin", fb->vinfo.right_margin);
    LUA_T_PUSH_S_N("upper_margin", fb->vinfo.upper_margin);
    LUA_T_PUSH_S_N("lower_margin", fb->vinfo.lower_margin);
    LUA_T_PUSH_S_N("hsync_len", fb->vinfo.hsync_len);
    LUA_T_PUSH_S_N("vsync_len", fb->vinfo.vsync_len);
    LUA_T_PUSH_S_N("sync", fb->vinfo.sync);
    LUA_T_PUSH_S_N("vmode", fb->vinfo.vmode);
    LUA_T_PUSH_S_N("rotate", fb->vinfo.rotate);
    LUA_T_PUSH_S_N("colorspace", fb->vinfo.colorspace);

    return 1;
    
}

static int lfb_framebuffer_get_fixinfo(lua_State *L) {
	
	framebuffer_t *fb;
	CHECK_FB(L, 1, fb)
	
    lua_newtable(L);

    LUA_T_PUSH_S_S("id", fb->finfo.id)
    LUA_T_PUSH_S_N("smem_start", fb->finfo.smem_start);
    LUA_T_PUSH_S_N("type", fb->finfo.type);
    LUA_T_PUSH_S_N("type_aux", fb->finfo.type_aux);
    LUA_T_PUSH_S_N("visual", fb->finfo.visual);
    LUA_T_PUSH_S_N("xpanstep", fb->finfo.xpanstep);
    LUA_T_PUSH_S_N("ypanstep", fb->finfo.ypanstep);
    LUA_T_PUSH_S_N("ywrapstep", fb->finfo.ywrapstep);
    LUA_T_PUSH_S_N("line_length", fb->finfo.line_length);
    LUA_T_PUSH_S_N("mmio_start", fb->finfo.mmio_start);
    LUA_T_PUSH_S_N("mmio_len", fb->finfo.mmio_len);
    LUA_T_PUSH_S_N("accel", fb->finfo.accel);
    LUA_T_PUSH_S_N("capabilities", fb->finfo.capabilities);

    return 1;
    
}

static int lfb_framebuffer_set_varinfo(lua_State *L) {
	framebuffer_t *fb;
	CHECK_FB(L, 1, fb)
	
    ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo);

    getnumfield32(L, "xres", &fb->vinfo.xres);
    getnumfield32(L, "yres", &fb->vinfo.yres);
    getnumfield32(L, "xres_virtual", &fb->vinfo.xres_virtual);
    getnumfield32(L, "yres_virtual", &fb->vinfo.yres_virtual);
    getnumfield32(L, "xoffset", &fb->vinfo.xoffset);
    getnumfield32(L, "yoffset", &fb->vinfo.yoffset);
    getnumfield32(L, "bits_per_pixel", &fb->vinfo.bits_per_pixel);
    getnumfield32(L, "grayscale", &fb->vinfo.grayscale);
    getnumfield32(L, "nonstd", &fb->vinfo.nonstd);
    getnumfield32(L, "activate", &fb->vinfo.activate);
    getnumfield32(L, "width", &fb->vinfo.width);
    getnumfield32(L, "height", &fb->vinfo.height);

    getnumfield32(L, "pixclock", &fb->vinfo.pixclock);
    getnumfield32(L, "left_margin", &fb->vinfo.left_margin);
    getnumfield32(L, "right_margin", &fb->vinfo.right_margin);
    getnumfield32(L, "upper_margin", &fb->vinfo.upper_margin);
    getnumfield32(L, "lower_margin", &fb->vinfo.lower_margin);
    getnumfield32(L, "hsync_len", &fb->vinfo.hsync_len);
    getnumfield32(L, "vsync_len", &fb->vinfo.vsync_len);
    getnumfield32(L, "sync", &fb->vinfo.sync);
    getnumfield32(L, "vmode", &fb->vinfo.vmode);
    getnumfield32(L, "rotate", &fb->vinfo.rotate);
    getnumfield32(L, "colorspace", &fb->vinfo.colorspace);

    return 1;
    
}

static int lfb_framebuffer_draw_from_drawbuffer(lua_State *L) {
    
    framebuffer_t *fb;
	CHECK_FB(L, 1, fb)
    
    drawbuffer_t *db;
	CHECK_DB(L, 2, db)
	
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    int cx;
    int cy;
    pixel_t p;
    uint32_t pixel;
    int location;

    for (cy=0; cy < db->h; cy=cy+1) {
        for (cx=0; cx < db->w; cx=cx+1) {
            p = db->data[cy*db->w+cx];
            if (fb->vinfo.bits_per_pixel == 16) {
				pixel = getcolor_16bpp(fb, p.r, p.g, p.b);
			}
			else if (fb->vinfo.bits_per_pixel == 32) {
				pixel = getcolor_32bpp(fb, p.r, p.g, p.b);
			}
            
            if (x+cx < 0 || y+cy < 0 || x+cx > (int)fb->vinfo.xres || y+cy > (int)fb->vinfo.yres || p.a <= 1) {
                continue;
            } else {
                location = (x + cx + fb->vinfo.xoffset) * (fb->vinfo.bits_per_pixel/8) + (y + cy + fb->vinfo.yoffset) * fb->finfo.line_length;
                switch (fb->vinfo.bits_per_pixel) {
                    case 16:
                        *(uint16_t*)(fb->data + location) = pixel;
                        break;
                    case 32:
                        *(uint32_t*)(fb->data + location) = pixel;
                        break;
                }
            }
        }
    }

    return 0;
    
}

static int lfb_new(lua_State *L) {
    framebuffer_t *lfb = (framebuffer_t *)lua_newuserdata(L, sizeof(framebuffer_t));

    lfb->fd = open(luaL_checkstring(L, 1), O_RDWR);
    if (lfb->fd < 0) {
        return luaL_error(L, "Couldn't open framebuffer: %s", strerror(errno));
    }

    ioctl(lfb->fd, FBIOGET_FSCREENINFO, &lfb->finfo);
    ioctl(lfb->fd, FBIOGET_VSCREENINFO, &lfb->vinfo);
    if (lfb->vinfo.bits_per_pixel != 16 && lfb->vinfo.bits_per_pixel != 32) {
        close(lfb->fd);
        lfb->fd = -1;
        return luaL_error(L, "Only 16 & 32 bpp are supported, not: %d", lfb->vinfo.bits_per_pixel);
    }
    lfb->data = mmap(0, (lfb->vinfo.yres_virtual * lfb->finfo.line_length), PROT_READ | PROT_WRITE, MAP_SHARED, lfb->fd, (off_t)0);
    lfb->fbdev = strdup(luaL_checkstring(L, 1));

    if (luaL_newmetatable(L, "framebuffer")) {
		
		lua_pushstring(L, "__index");
		lua_newtable(L);

		LUA_T_PUSH_S_CF("close", lfb_framebuffer_close)
		LUA_T_PUSH_S_CF("get_fixinfo", lfb_framebuffer_get_fixinfo)
		LUA_T_PUSH_S_CF("get_varinfo", lfb_framebuffer_get_varinfo)
		LUA_T_PUSH_S_CF("set_varinfo", lfb_framebuffer_set_varinfo)
		LUA_T_PUSH_S_CF("draw_from_drawbuffer", lfb_framebuffer_draw_from_drawbuffer)
		// LUA_T_PUSH_S_CF("draw_to_drawbuffer", lfb_framebuffer_draw_to_drawbuffer)
		
		lua_settable(L, -3);
		
		LUA_T_PUSH_S_CF("__gc", lfb_framebuffer_close)
		LUA_T_PUSH_S_CF("__tostring", lfb_framebuffer_tostring)
	}
    lua_setmetatable(L, -2);

    return 1;
    
}



LUALIB_API int luaopen_lfb(lua_State *L) {
    lua_newtable(L);

    LUA_T_PUSH_S_S("version", VERSION)
    LUA_T_PUSH_S_CF("new", lfb_new)

    return 1;
}
