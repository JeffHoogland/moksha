Summary: The Enlightenment window manager
Name: enlightenment
Version: 0.16.999.011
Release: NOT_RELEASE_1.%(date '+%Y%m%d')
License: BSD
Group: User Interface/Desktops
URL: http://www.enlightenment.org/
Source: ftp://ftp.enlightenment.org/pub/enlightenment/%{name}-%{version}.tar.gz
Packager: %{?_packager:%{_packager}}%{!?_packager:Michael Jennings <mej@eterm.org>}
Vendor: %{?_vendorinfo:%{_vendorinfo}}%{!?_vendorinfo:The Enlightenment Project (http://www.enlightenment.org/)}
Distribution: %{?_distribution:%{_distribution}}%{!?_distribution:%{_vendor}}
Prefix: %{_prefix}
#BuildSuggests: xorg-x11-devel
BuildRequires: libjpeg-devel XFree86-devel eet-devel embryo-devel
BuildRequires: evas-devel edb-devel edje-devel imlib2-devel ecore-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
Enlightenment is a window manager.

%package devel
Summary: Development headers for Enlightenment. 
Group: User Interface/Desktops
Requires: %{name} = %{version}
Requires: libjpeg-devel XFree86-devel eet-devel embryo-devel
Requires: evas-devel edb-devel edje-devel imlib2-devel ecore-devel

%description devel
Development headers for Enlightenment.

%prep
%setup -q

%build
%{configure} --prefix=%{_prefix}
%{__make} %{?_smp_mflags} %{?mflags}

%install
%{__make} %{?mflags_install} DESTDIR=$RPM_BUILD_ROOT install
test -x `which doxygen` && sh gendoc || :

%clean
test "x$RPM_BUILD_ROOT" != "x/" && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-, root, root)
%doc AUTHORS COPYING COPYING-PLAIN README
%{_bindir}/*
%{_libdir}/libe.so.*
%{_libdir}/%{name}
%{_datadir}/%{name}
%{_datadir}/locale/*

%files devel
%defattr(-, root, root)
%{_includedir}/enlightenment/*.h
%{_includedir}/E_Lib.h
%{_libdir}/libe.a
%{_libdir}/libe.la
%{_libdir}/libe.so

%changelog
