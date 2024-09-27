Name:       libdsme

Summary:    DSME dsmesock dynamic library
Version:    0.66.8
Release:    0
License:    LGPLv2
URL:        https://github.com/sailfishos/libdsme
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(check)

%global makeflags \\\
	DESTDIR=%{buildroot} \\\
	LIBDIR=%{_libdir}

%description
This package contains dynamic libraries for programs that communicate with the
Device State Management Entity.

%package devel
Summary:    Development files for dsme
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains headers and static libraries needed to develop programs
that want to communicate with the Device State Management Entity.

%package tests
Summary:    Test suite for dsme
Requires:   %{name} = %{version}-%{release}

%description tests
This package contains test suite for libdsme.

%prep
%setup -q -n %{name}-%{version}

%build
./verify_version
unset LD_AS_NEEDED
%{make_build} %{makeflags}

%install
%{make_install} %{makeflags}
# remove static libs
rm  %{buildroot}%{_libdir}/*.a

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/%{name}.so.*
%{_libdir}/%{name}_dbus_if.so.*
%{_libdir}/libthermalmanager_dbus_if.so.*
%license COPYING debian/copyright

%files devel
%dir %{_includedir}/dsme
%{_includedir}/dsme/*
%{_libdir}/%{name}.so
%{_libdir}/%{name}_dbus_if.so
%{_libdir}/libthermalmanager_dbus_if.so
%dir %{_libdir}/pkgconfig
%{_libdir}/pkgconfig/*

%files tests
%dir /opt/tests
/opt/tests/%{name}
