# Note that this is NOT a relocatable package
%define ver      0.17.0_pre10
%define rel      NOT_RELEASE_1
%define prefix   /usr/local

Summary: enlightenment
Name: enlightenment
Version: %ver
Release: %rel
Copyright: BSD
Group: System Environment/Desktops
Source: ftp://ftp.enlightenment.org/pub/enlightenment/enlightenment-%{ver}.tar.gz
BuildRoot: /var/tmp/enlightenment-root
Packager: The Rasterman <raster@rasterman.com>
URL: http://www.enlightenment.org/
BuildRequires: evas-devel
BuildRequires: edje-devel
BuildRequires: ecore-devel
BuildRequires: embryo-devel
BuildRequires: eet-devel
Requires: edje
Requires: evas
Requires: ecore
Requires: embryo
Requires: eet

Docdir: %{prefix}/doc

%description

Enlightenment is a window manager

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
./configure --prefix=%prefix

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi
###########################################################################

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%defattr(-,root,root)
%attr(755,root,root) %{prefix}/lib/enlightenment
%attr(755,root,root) %{prefix}/bin/*
%attr(755,root,root) %{prefix}/share/enlightenment
%doc AUTHORS
%doc COPYING
%doc COPYING-PLAIN
%doc README

%changelog
* Sat Jun 23 2001 The Rasterman <raster@rasterman.com>
- Created spec file
