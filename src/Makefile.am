bin_PROGRAMS = huayra-accesibility-settings
huayra_accesibility_settings_SOURCES = 	\
	main.c \
	huayra-hig.c \
	huayra-hig.h \
	populate-cursors.c \
	populate-cursors.h

huayra_accesibility_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(XCURSOR_CFLAGS) \
	$(XFCE4UTILS_CFLAGS)

huayra_accesibility_settings_LDADD = \
	$(GTK_LIBS) \
	$(XCURSOR_LIBS) \
	$(XFCE4UTILS_LIBS)

CLEANFILES = *~