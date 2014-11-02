%define revcount %(git rev-list HEAD | wc -l)
%define treeish %(git rev-parse --short HEAD)
%define localmods %(git diff-files --exit-code --quiet  || date +.m%%j%%H%%M%%S)

%define srcdir   %{getenv:PWD}

Summary: Lookout IPEvent Backend
Name: lookout-backend
Version: 1.0
Release: %{revcount}.%{treeish}%{localmods}
Distribution: Redgate/Services
Group: System Environment/Daemons
License: Proprietary
Vendor: Karl Redgate
Packager: Karl Redgate <Karl.Redgate@gmail.com>

Requires: nodejs
Requires: npm
Requires: jq

%description
Config and scripts for the IpEventAPI service and
related command line tools.

%prep
%build

%install
%{__install} --directory --mode=755 $RPM_BUILD_ROOT/etc/init/lookout
%{__install} --mode=755 %{srcdir}/init/lookout/*.conf $RPM_BUILD_ROOT/etc/init/lookout

%{__install} --directory --mode=755 $RPM_BUILD_ROOT/usr/sbin
%{__install} --mode=755 %{srcdir}/lookout-backend $RPM_BUILD_ROOT/usr/sbin
%{__install} --mode=755 %{srcdir}/lookout-rest $RPM_BUILD_ROOT/usr/sbin

%{__install} --directory --mode=755 $RPM_BUILD_ROOT/usr/libexec/lookout/setup
%{__install} --mode=755 %{srcdir}/libexec/lookout/setup/* $RPM_BUILD_ROOT/usr/libexec/lookout/setup

%{__install} --directory --mode=755 $RPM_BUILD_ROOT/var/run/lookout

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0755)
/usr/sbin/lookout-backend
/usr/sbin/lookout-rest
/etc/init/lookout
/usr/libexec/lookout/
%attr(0755,ec2-user,ec2-user) /var/run/lookout/
# There will need to be a "config" entry for the DB file, since it must persist upgrades

%pre

%post
[ "$1" -gt 1 ] && {
    : Upgrading
}

[ "$1" = 1 ] && {
    : New install
}

/usr/libexec/lookout/setup/install-node-syslog | logger --tag %{name}

: ignore test return value

%preun
[ "$1" = 0 ] && {
    : cleanup
}

: ignore test return value

%postun

[ "$1" = 0 ] && {
    : This is really an uninstall
}

: ignore test errs

%changelog

* Sat Nov  1 2014 Karl Redgate <Karl.Redgate@gmail.com>
- Initial release

# dis-vim:syntax=plain
# vim:autoindent expandtab sw=4
