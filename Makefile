# Package version
VERSION   := 0.67.0

# Shared object version
SOMAJOR   := .0
SOMINOR   := .3
SORELEASE := .0

SOLINK    := .so
SONAME    := .so$(SOMAJOR)
SOVERS    := .so$(SOMAJOR)$(SOMINOR)$(SORELEASE)

# Files to build / install
TARGETS_DSO    += libdsme$(SOVERS)
TARGETS_DSO    += libdsme_dbus_if$(SOVERS)
TARGETS_DSO    += libthermalmanager_dbus_if$(SOVERS)

INSTALL_HDR    += include/dsme/protocol.h
INSTALL_HDR    += include/dsme/messages.h
INSTALL_HDR    += include/dsme/alarm_limit.h
INSTALL_HDR    += include/dsme/processwd.h
INSTALL_HDR    += include/dsme/state.h
INSTALL_HDR    += include/dsme/state_states.h
INSTALL_HDR    += modules/dsme_dbus_if.h
INSTALL_HDR    += modules/thermalmanager_dbus_if.h

INSTALL_PC     += dsme.pc
INSTALL_PC     += dsme_dbus_if.pc
INSTALL_PC     += thermalmanager_dbus_if.pc

TARGETS_UT_BIN += tests/ut_libdsme
INSTALL_UT_XML += tests/tests.xml

TARGETS_ALL    += $(TARGETS_DSO) $(TARGETS_UT_BIN)

# Dummy default install dir - override from packaging scripts
DESTDIR ?= libdsme-test-install

# Define LIBDIR so we can override it if needed
LIBDIR  ?= /usr/lib

# ----------------------------------------------------------------------------
# Top level targets
# ----------------------------------------------------------------------------

.PHONY: build install clean distclean mostlyclean

build:: $(TARGETS_ALL)

install:: build

clean:: mostlyclean
	$(RM) $(TARGETS_ALL)

distclean:: clean

mostlyclean::
	$(RM) *.o *~ */*.o */*~

install :: install_main install_devel install_tests

install_main::
	# dynamic libraries
	install -m755 -D -t $(DESTDIR)$(LIBDIR) $(TARGETS_DSO)
	for f in $(patsubst %$(SOVERS),%,$(TARGETS_DSO)); do \
	  ln -sf "$$f$(SOVERS)" "$(DESTDIR)$(LIBDIR)/$$f$(SONAME)"; \
	done

install_devel::
	# headers
	install -m644 -D -t $(DESTDIR)/usr/include/dsme $(INSTALL_HDR)
	# pkg config
	install -m644 -D -t $(DESTDIR)$(LIBDIR)/pkgconfig $(INSTALL_PC)
	# dynamic library symlinks
	for f in $(patsubst %$(SOVERS),%,$(TARGETS_DSO)); do \
	  ln -sf "$$f$(SONAME)" "$(DESTDIR)$(LIBDIR)/$$f$(SOLINK)"; \
	done

install_tests::
	# xml
	install -m644 -D -t $(DESTDIR)/opt/tests/libdsme $(INSTALL_UT_XML)
	# binary
	install -m755 -D -t $(DESTDIR)/opt/tests/libdsme $(TARGETS_UT_BIN)

# ----------------------------------------------------------------------------
# Build rules
# ----------------------------------------------------------------------------

%.pic.o     : %.c ; $(CC) -o $@ -c $< -fPIC $(CPPFLAGS) $(CFLAGS)
%.o         : %.c ; $(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)
%$(SOVERS)  :     ; $(CC) -o $@ -shared -Wl,-soname,$*$(SONAME) $^ $(LDFLAGS) $(LDLIBS)
%           : %.o ; $(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ----------------------------------------------------------------------------
# Common options
# ----------------------------------------------------------------------------

CFLAGS   += -std=c99
CFLAGS   += -O2
CFLAGS   += -Wall
CFLAGS   += -Wwrite-strings
CFLAGS   += -Wmissing-prototypes
CFLAGS   += -Werror

CFLAGS   += -g
LDFLAGS  += -g

LDFLAGS  += -Wl,--as-needed

# ----------------------------------------------------------------------------
# libdsme$(SOVERS)
# ----------------------------------------------------------------------------

libdsme_OBJ += protocol.pic.o message.pic.o alarm_limit.pic.o
libdsme_PC  += glib-2.0

libdsme$(SOVERS) : CFLAGS += $$(pkg-config --cflags $(libdsme_PC))
libdsme$(SOVERS) : LDLIBS += $$(pkg-config --libs $(libdsme_PC))
libdsme$(SOVERS) : $(libdsme_OBJ)

# ----------------------------------------------------------------------------
# libdsme_dbus_if$(SOVERS)
# ----------------------------------------------------------------------------

libdsme_dbus_if_OBJ += modules/dsme_dbus_if.pic.o

libdsme_dbus_if$(SOVERS) : $(libdsme_dbus_if_OBJ)

# ----------------------------------------------------------------------------
# libthermalmanager_dbus_if$(SOVERS)
# ----------------------------------------------------------------------------

libthermalmanager_dbus_if_OBJ += modules/thermalmanager_dbus_if.pic.o

libthermalmanager_dbus_if$(SOVERS) : $(libthermalmanager_dbus_if_OBJ)

# ----------------------------------------------------------------------------
# ut_libdsme
# ----------------------------------------------------------------------------

ut_libdsme_OBJ += tests/ut_libdsme.o
ut_libdsme_PC  += glib-2.0 check

tests/ut_libdsme : CFLAGS += $$(pkg-config --cflags $(ut_libdsme_PC))
tests/ut_libdsme : LDLIBS += $$(pkg-config --libs $(ut_libdsme_PC))
tests/ut_libdsme : $(ut_libdsme_OBJ) libdsme$(SOVERS)
