Summary: The Enlightenment window manager
Name: enlightenment
Version: 0.17.0_pre10
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
%{_libdir}/%{name}
%{_datadir}/%{name}

%changelog
