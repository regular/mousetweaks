gsettings_in_files = org.gnome.mousetweaks.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

man_MANS = mousetweaks.1

RESOURCES =                     \
    window.ui                   \
    primary.png                 \
    secondary.png               \
    double.png                  \
    drag.png

EXTRA_DIST =                    \
    mousetweaks.gresource.xml   \
    $(RESOURCES)                \
    $(gsettings_in_files)       \
    $(man_MANS)

DISTCLEANFILES =                \
    $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
