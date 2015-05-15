all:
	gcc -g \
	`pkg-config --cflags gtk+-3.0 libxfce4util-1.0` \
	`pkg-config --libs gtk+-3.0 libxfce4util-1.0 xcursor` \
	-o huayra-accessibility-settings \
	main.c \
	huayra-hig.c \
	huayra-hig.h \
	populate-cursors.c \
	populate-cursors.h
