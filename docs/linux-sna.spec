%define name linux-sna
%define ver  VERSION
%define rel  1
%define vendor linux-SNA Project
%define distro linux-SNA Enterprise Multiprotocol Suite
%define packager jschlst@linux-sna.org

Vendor: %{vendor}
Distribution: %{distro}
Packager: %{packager}
Name: %{name}
Version: %{ver}
Release: %{rel}
Summary: linux-SNA base network communications software.
Copyright: GPL
Group: Networking
URL: http://www.linux-sna.org/pub/software/
Source0: ftp.linux-sna.org/pub/linux-sna/v0.1/source/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-root

%package asuite
Summary: linux-SNA APPC application software suite.
Group: Networking
Requires: linux-sna

%package sense
Summary: linux-SNA sense error code message decoder.
Group: Networking
Requires: linux-sna

#%package tn3270
#Summary: TN3270 related software.
#Group: Networking
#Requires: linux-sna

#%package tn5250
#Summary: TN5250 related software.
#Group: Networking
#Requires: linux-sna

%description
Various user-space software configuration files and base applications
to enable linux-SNA communications.

%description asuite
APPC application suite, contains such programs as acopy, aftp, aname,
aping, arexec and atell.

%description sense
linux-SNA sense error code message decoder.

#%description tn3270
#TN3270 related software.

#%description tn5250
#TN5250 related software.

%changelog
* Tue Jun 05 2001 Jay Schulist <jschlst@linux-sna.org>
- Created linux-sna RPM

%prep
%setup -q -n linux-sna-%{ver}
./configure --prefix=${RPM_BUILD_ROOT}/usr

%build
make

%install
install -D -m 644 docs/confs/sna.xml $RPM_BUILD_ROOT/etc/sna.xml
install -D -m 644 docs/confs/sna.cfg $RPM_BUILD_ROOT/etc/sna.cfg
install -D -m 644 docs/confs/snatchd.conf $RPM_BUILD_ROOT/etc/snatchd.conf
install -D -m 755 docs/snatchd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/snatchd.init
#install -D -m 755 docs/tn3270d.init $RPM_BUILD_ROOT/etc/rc.d/init.d/tn3270d.init
#install -D -m 755 docs/tn5250d.init $RPM_BUILD_ROOT/etc/rc.d/init.d/tn5250d.init
make mandir=$RPM_BUILD_ROOT/usr/share/man install

%post

if [ "$1" = 1 ]; then
	# add these lines to modules.conf
	if (grep 'linux-sna' /etc/modules.conf >/dev/null); then
                cat <<'_EOD1_' >&2
warning: The module parameters appear to be present in /etc/modules.conf.
warning: Please check them against modules.conf.sna in the documentation.
_EOD1_
                true
        else
                cat <<'_EOD2_' >>/etc/modules.conf
# start of linux-sna module configuration
alias net-pf-22 sna
# end of linux-sna module configuration
_EOD2_
        fi
fi

chkconfig --add snatchd.init

#%post tn3270d
#chkconfig --add tn3270d.init

#%post tn5250d 
#chkconfig --add tn5250d.init

%postun
# do only for the last un-install
if [ "$1" = 0 ]; then
        # remove the linux-sna lines from /etc/modules.conf
        if (grep '^# start of linux-sna module configuration$' /etc/modules.conf >/dev/null && \
            grep '^# end of linux-sna module configuration$'   /etc/modules.conf >/dev/null ); then
          sed -e '/^# start of linux-sna module configuration$/,/^# end of linux-sna module configuration$/d' \
            </etc/modules.conf >/tmp/modules.conf.tmp$$
          cat /tmp/modules.conf.tmp$$ >/etc/modules.conf
          rm /tmp/modules.conf.tmp$$
        else
          cat <<'_EOD3_' >&2
warning: Unable to find the lines `# start of linux-sna module configuration` and
warning: `# end of linux-sna module configuration` in the file /etc/modules.conf
warning: You should remove the linux-sna module configuration from /etc/modules.conf manually.
_EOD3_
        fi
fi

chkconfig --del snatchd.init

#%preun tn3270d
#chkconfig --del tn3270d.init

#%preun tn5250d
#chkconfig --del tn5250d.init

%clean
if [ ! RPM_BUILD_ROOT = / ]; then
	rm -rf ${RPM_BUILD_ROOT}
fi

%files asuite
%defattr(-, root, root)
/usr/bin/acopy
/usr/bin/aftp
/usr/bin/aping
/usr/bin/arexec
/usr/bin/atell
/usr/sbin/aftpd
/usr/sbin/anamed
/usr/sbin/apingd
/usr/sbin/arexecd
/usr/sbin/atelld
/usr/share/man/man8/acopy.8.gz
/usr/share/man/man8/aftp.8.gz
/usr/share/man/man8/aping.8.gz
/usr/share/man/man8/arexec.8.gz
/usr/share/man/man8/atell.8.gz
/usr/share/man/man8/aftpd.8.gz
/usr/share/man/man8/anamed.8.gz
/usr/share/man/man8/apingd.8.gz
/usr/share/man/man8/arexecd.8.gz
/usr/share/man/man8/atelld.8.gz
/usr/lib/libacomm*
/usr/lib/libaftp*
/usr/lib/libaname*
/usr/include/anameapi.h
/usr/include/appfftp.h
/usr/include/asuitecommon.h
/usr/include/cpicsession.h
/usr/include/filemode.h
/usr/include/libaftp.h
/usr/include/libaname.h
/usr/include/port.h
/usr/include/tdb.h
/usr/include/util.h
/usr/include/wildcard.h
%doc docs/arch-notes/asuite.txt

%files sense
%defattr(-, root, root)
/usr/bin/sense
/usr/bin/mksense
/usr/share/man/man1/sense.1.gz
/usr/share/man/man1/mksense.1.gz

#%files tn3270
#%defattr(-, root, root)
#/etc/rc.d/init.d/tn3270d.init
#/usr/sbin/tn3270d
#/usr/include/host3270.h
#/usr/include/codes3270.h
#/usr/include/tn3270d.h

#%files tn5250
#%defattr(-, root, root)
#/etc/rc.d/init.d/tn5250d.init
#/usr/sbin/tn5250d
#/usr/include/host5250.h
#/usr/include/tn5250d.h

%files
%defattr(-, root, root)
%config /etc/sna.cfg
%config /etc/sna.xml
%config /etc/snatchd.conf
/etc/rc.d/init.d/snatchd.init
/usr/sbin/snaconfig
/usr/sbin/snatchd
/usr/lib/libappc*
/usr/lib/libattach*
/usr/lib/libcpic*
/usr/lib/liblua*
/usr/lib/libnof*
/usr/include/appc.h
/usr/include/attach.h
/usr/include/lua.h
/usr/include/nof.h
/usr/share/man/man5/sna.conf.5.gz
/usr/share/man/man7/sna.7.gz
/usr/share/man/man8/snaconfig.8.gz
/usr/share/man/man8/snatchd.8.gz
%doc BUGS COPYING AUTHORS ChangeLog INSTALL NEWS TODO README THANKS
%doc docs/linux-sna.spec docs/modules.conf.sna
%doc docs/confs/sna.cfg docs/confs/sna.xml docs/confs/snatchd.conf
%doc docs/html-guides/begin_snaconfig.html docs/html-guides/get_started.html
%doc docs/html-guides/quick-appendix-a.html
%doc docs/html-guides/quick-node-setup.html
%doc docs/html-guides/quick-sna.html
%doc docs/html-guides/verify-sna-configure.html
%doc docs/html-guides/verify-sna.html
