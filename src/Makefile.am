AM_CPPFLAGS =                                   \
    $(WARN_CFLAGS)                              \
    -DGNOMELOCALEDIR=\""$(datadir)/locale"\"    \
    -DLIBEXECDIR=\""$(libexecdir)"\"

bin_PROGRAMS = mousetweaks

mousetweaks_SOURCES =           \
    main.c                      \
    mt_application.c            \
    mt_application.h            \
    mt_click.c                  \
    mt_click.h                  \
    mt_hover_click.c            \
    mt_hover_click.h            \
    mt_listener.c               \
    mt_listener.h               \
    mt_listener_atspi.c         \
    mt_listener_atspi.h         \
    mt_secondary_click.c        \
    mt_secondary_click.h        \
    mt_timer.c                  \
    mt_timer.h

mousetweaks_CFLAGS =            \
    $(AM_CFLAGS)                \
    $(DAEMON_CFLAGS)

mousetweaks_LDADD =             \
    $(DAEMON_LIBS)

libexec_PROGRAMS = hover-click-window

hover_click_window_SOURCES =    \
    $(BUILT_SOURCES)            \
    hover_click_window.c

hover_click_window_CFLAGS =     \
    $(AM_CFLAGS)                \
    $(WINDOW_CFLAGS)

hover_click_window_LDADD =      \
    $(WINDOW_LIBS)

BUILT_SOURCES =                 \
    mousetweaks_resources.c

CLEANFILES =                    \
    $(BUILT_SOURCES)

mousetweaks_resources.c: $(top_srcdir)/data/mousetweaks.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ \
	--sourcedir=$(top_srcdir)/data --generate-source $<

-include $(top_srcdir)/git.mk
