# Note that this is NOT a relocatable package
%define ver      1.0
%define rel      1
%define prefix   /usr/local

Summary: Enlightenment DR0.17's new "bit" editor
Name: etcher
Version: %ver
Release: %rel
Copyright: BSD
Group: X11/Libraries
Source: ftp://ftp.enlightenment.org/pub/enlightenment/etcher-%{ver}.tar.gz
BuildRoot: /var/tmp/etcher-root
Packager: Term <kempler@utdallas.edu>
URL: http://www.enlightenment.org/
Requires: evas >= 0.0.1
Requires: edb >= 1.0.0
Requires: imlib2 >= 1.0.0

Docdir: %{prefix}/doc

%description
Etcher is a new application devised to assist would-be theme developers in
designin "bits", that is, window borders, icons, whatever, for
Enlightenment. Since Enlightenment DR0.17 uses drag-and-drop instead of
texual configuration files, this application will become instrumental for
themeing under the new Enlightenment version.

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
%attr(755,root,root) %{prefix}/bin/etcher
%{prefix}/share/etcher/*

%doc AUTHORS
%doc COPYING
%doc README

%changelog
* Mon Aug 28 2000 Lyle Kempler <kempler@utdallas.edu>
- Created spec file
