bin_PROGRAMS = huayra-accessibility-settings
huayra_accessibility_settings_SOURCES = 	\
	main.c \
	huayra-hig.c \
	huayra-hig.h \
	populate-cursors.c \
	populate-cursors.h

huayra_accessibility_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(XCURSOR_CFLAGS) \
	$(XFCE4UTILS_CFLAGS)

huayra_accessibility_settings_LDADD = \
	$(GTK_LIBS) \
	$(XCURSOR_LIBS) \
	$(XFCE4UTILS_LIBS)

CLEANFILES = *~