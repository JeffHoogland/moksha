# Note that this is NOT a relocatable package
%define ver      0.17.pre_0
%define rel      1
%define prefix   /usr/local

Summary: Enlightenment DR0.17 CVS
Name: enlightenment
Version: %ver
Release: %rel
Copyright: BSD
Group: X11/Libraries
Source: ftp://ftp.enlightenment.org/pub/enlightenment/enlightenment-%{ver}.tar.gz
BuildRoot: /var/tmp/enlightenment-root
Packager: The Rasterman <raster@rasterman.com>
URL: http://www.enlightenment.org/
Requires: evas >= 0.0.2
Requires: edb >= 1.0.0
Requires: imlib2 >= 1.0.0
Requires: ebits >= 0.0.0
Requires: ecore >= 0.0.0

Docdir: %{prefix}/doc

%description
Errrrrrr - E17. You go figure.

%prep
%setup

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
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%defattr(-,root,root)
%doc README COPYING ChangeLog
%attr(755,root,root) %{prefix}/bin/enlightenment
%{prefix}/share/enlightenment/*

%doc AUTHORS
%doc COPYING
%doc README

%changelog
* Sat Jan 6 2001 Lyle Kempler <term@twistedpath.org>
- Fixed spec file. :)
* Sat Dec 11 2000 The Rasterman <raster@rasterman.com>
- Created spec file

