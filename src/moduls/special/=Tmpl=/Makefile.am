EXTRA_DIST = *.h po/*

#!!! Include module to OpenSCADA core process. It is typical!
if TmplIncl
#!!! The module result library name
noinst_LTLIBRARIES = spec_Tmpl.la
spec_Tmpl_la_CXXFLAGS = -DMOD_INCL -fpic
#!!! Libtool util's flags. It is typical!
spec_Tmpl_la_LIBTOOLFLAGS = --tag=disable-shared
#!!! The module link flags
spec_Tmpl_la_LDFLAGS = -module
else
#!!! The module result library name
oscd_modul_LTLIBRARIES = spec_Tmpl.la
spec_Tmpl_la_CXXFLAGS =
#!!! Libtool util's flags. It is typical!
spec_Tmpl_la_LIBTOOLFLAGS = --tag=disable-static
#!!! The module link flags
spec_Tmpl_la_LDFLAGS = -module -avoid-version $(top_builddir)/src/liboscada.la
endif

#!!! The module compiler flags
spec_Tmpl_la_CXXFLAGS += $(tmpl_CFLAGS)
#!!! The module link flags
spec_Tmpl_la_LDFLAGS += $(tmpl_LDLAGS)
#!!! The module source files list
spec_Tmpl_la_SOURCES = module.cpp


#!!! The module name for translation files
I18N_mod = $(oscd_modulpref)Tmpl
include ../../../../I18N.mk
