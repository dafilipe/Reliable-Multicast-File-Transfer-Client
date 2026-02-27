#/*****************************************************************************\
# * Redes Integradas de Telecomunicacoes
# * MIEEC / MEEC / MERSIM - FCT NOVA  2025/2025
# *
# * Makefile
# *
# * @author  Luis Bernardo
#\*****************************************************************************/
GNOME_INCLUDES= `pkg-config --cflags --libs gtk+-3.0`
CFLAGS= -Wall -g -DDEBUG -Wno-deprecated-declarations 
# CFLAGS= -Wall -g -Wno-deprecated-declarations 
# CFLAGS= -Wall -O3 -Wno-deprecated-declarations 

APP_NAME= fmulticast_client
APP_MODULES= sock.o gui_g3.o callbacks.o receiver_th.o file.o bitmask.o

all: $(APP_NAME)
	
clean: 
	rm -f $(APP_NAME) *.o


$(APP_NAME): main.c $(APP_MODULES) gui.h sock.h callbacks.h file.h
	gcc $(CFLAGS) -o $(APP_NAME) main.c $(APP_MODULES) $(GNOME_INCLUDES) -lm -export-dynamic

sock.o: sock.c sock.h gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) sock.c -export-dynamic

gui_g3.o: gui_g3.c gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) gui_g3.c -export-dynamic
	
callbacks.o: callbacks.c callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) callbacks.c -export-dynamic

receiver_th.o: receiver_th.c receiver_th.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) receiver_th.c -export-dynamic

file.o: file.c file.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) file.c -export-dynamic
		
bitmask.o: bitmask.c bitmask.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) bitmask.c -export-dynamic

