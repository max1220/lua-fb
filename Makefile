CFLAGS = -O3 -fPIC -I/usr/include/lua5.1 -Wall -Wextra
LIBS   = -shared -llua5.1
TARGET = lua_fb.so

all: $(TARGET)

$(TARGET): lua-fb.c lua-db.h
	$(CC) -o $(TARGET) lua-fb.c $(CFLAGS) $(LIBS)
	# $(CC) -o $(TARGET) lua-db.c $(CFLAGS) $(LIBS)
	strip $(TARGET)

clean:
	rm -f $(TARGET)
