%define name sna-tools
%define ver  1.00pre13
%define rel  1

Name: %{name}
Version: %{ver}
Release: %{rel}
Summary: Linux-SNA node configuration tools.
Copyright: GPL
Group: Networking/Admin
URL: ftp://ftp.linux-sna.org/pub/user-tools/
Source0: ftp.linux-sna.org/pub/user-tools/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-root

%description
snaconfig, snatchd, sense, mksense.

%changelog
* Thu Jun 8 2000 Jay Schulist <jschlst@turbolinux.com>
- Created asuite RPM

%prep
%setup -q -n sna-tools-%{ver}
./configure --prefix=${RPM_BUILD_ROOT}/usr

%build
make

%install
make mandir=$RPM_BUILD_ROOT/usr/share/man install

#%clean
#if [ ! ${RPM_BUILD_ROOT} = / ]; then
#  rm -rf ${RPM_BUILD_ROOT}
#fi

%files
/usr/bin/*
/usr/sbin/*
/usr/share/man/man8/*
%doc BUGS COPYING ChangeLog INSTALL FEATURES NEWS TODO README THANKS
