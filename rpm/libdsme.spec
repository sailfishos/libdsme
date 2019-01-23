Name:       libdsme

Summary:    DSME dsmesock dynamic library
Version:    0.66.0
Release:    0
Group:      System/System Control
License:    LGPLv2.1
URL:        https://git.merproject.org/mer-core/libdsme
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(check)

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
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/%{name}.so.*
%{_libdir}/%{name}_dbus_if.so.*
%{_libdir}/libthermalmanager_dbus_if.so.*
%license COPYING debian/copyright

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/dsme
%{_includedir}/dsme/*
%{_libdir}/%{name}.so
%{_libdir}/%{name}_dbus_if.so
%{_libdir}/libthermalmanager_dbus_if.so
%dir %{_libdir}/pkgconfig
%{_libdir}/pkgconfig/*

%files tests
%defattr(-,root,root,-)
%dir /opt/tests
/opt/tests/%{name}
