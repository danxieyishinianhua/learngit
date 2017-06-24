LIBS = -lpthread

OBJS = rs232.c main.c tcp.c check_link.c  serial.c led_log.c mod_zigbee.c zigbeemsg.c nodearray.c tcpserver.c objectc.c unpack.c version.c vrefbuffer.c zone.c fun_msgpack.c send_list.c 
EXEC = tcp_rs232
INCLUDES = -I include
all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) -o $(EXEC) $(OBJS) $(LIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~

