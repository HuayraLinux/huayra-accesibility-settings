bin_PROGRAMS = huayra-accessibility-settings
huayra_accessibility_settings_SOURCES = 	\
	main.c \
	huayra-hig.c \
	huayra-hig.h \
	populate-cursors.c \
	populate-cursors.h \
	mate-session.c \
	mate-session.h

huayra_accessibility_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(XCURSOR_CFLAGS)

huayra_accessibility_settings_LDADD = \
	$(GTK_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(XCURSOR_LIBS)

CLEANFILES = *~