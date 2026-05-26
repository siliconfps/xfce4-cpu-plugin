plugin_LTLIBRARIES = libxfce4-cpu-plugin.la

libxfce4_cpu_plugin_la_SOURCES = \
	xfce4-cpu-plugin.c

libxfce4_cpu_plugin_la_CFLAGS = \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	-DGETTEXT_PACKAGE=\"xfce4-cpu-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

libxfce4_cpu_plugin_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'

libxfce4_cpu_plugin_la_LIBADD = \
	$(LIBXFCE4PANEL_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(GLIB_LIBS)

desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = xfce4-cpu-plugin.desktop

EXTRA_DIST = \
	xfce4-cpu-plugin.desktop.in

CLEANFILES = $(desktop_DATA)

xfce4-cpu-plugin.desktop: xfce4-cpu-plugin.desktop.in
	$(AM_V_GEN) sed \
		-e 's|@PLUGIN_PATH@|$(libdir)/xfce4/panel/plugins|' \
		$< > $@
