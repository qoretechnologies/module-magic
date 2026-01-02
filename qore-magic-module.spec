%define module_api %(qore --latest-module-api 2>/dev/null)
%define module_dir %{_libdir}/qore-modules

%if 0%{?sles_version}

%define dist .sles%{?sles_version}

%else
%if 0%{?suse_version}

# get *suse release major version
%define os_maj %(echo %suse_version|rev|cut -b3-|rev)
# get *suse release minor version without trailing zeros
%define os_min %(echo %suse_version|rev|cut -b-2|rev|sed s/0*$//)

%if %suse_version
%define dist .opensuse%{os_maj}_%{os_min}
%endif

%endif
%endif

# see if we can determine the distribution type
%if 0%{!?dist:1}
%define rh_dist %(if [ -f /etc/redhat-release ];then cat /etc/redhat-release|sed "s/[^0-9.]*//"|cut -f1 -d.;fi)
%if 0%{?rh_dist}
%define dist .rhel%{rh_dist}
%else
%define dist .unknown
%endif
%endif

Summary: Libmagic module for Qore
Name: qore-magic-module
Version: 1.0.1
Release: 1%{dist}
License: LGPL-2.0+
Group: Development/Languages/Other
URL: http://qore.org
Source: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: gcc-c++
BuildRequires: qore-devel >= 1.12.4
BuildRequires: qore-stdlib >= 1.12.4
BuildRequires: qore >= 1.0
BuildRequires: cmake
BuildRequires: file-devel
BuildRequires: doxygen

%description
libmagic (file magic) API for the Qore Programming Language

%prep
%setup -q

%build
export CXXFLAGS="%{?optflags}"
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DCMAKE_SKIP_RPATH=1 -DCMAKE_SKIP_INSTALL_RPATH=1 -DCMAKE_SKIP_BUILD_RPATH=1 -DCMAKE_PREFIX_PATH=${_prefix}/lib64/cmake/Qore .
%{__make}
%{__make} docs
sed -i 's/#!\/usr\/bin\/env qore/#!\/usr\/bin\/qore/' test/*.qtest

%install
make DESTDIR=%{buildroot} install

%files
%{module_dir}

%check
qore -l ./magic-api-*.qmod test/magic.qtest

%package doc
Summary: Documentation and examples for the Qore magic module
Group: Development/Languages/Other

%description doc
This package contains the HTML documentation and example programs for the Qore
magic module.

%files doc
%defattr(-,root,root,-)
%doc docs/magic test

%changelog
* Sat Dec 17 2022 David Nichols <david@qore.org> 1.0.1
- updated to version 1.0.1

* Thu Jan 27 2022 David Nichols <david@qore.org> 1.0.0
- updated to version 1.0.0

* Mon Jul 29 2013 Petr Vanek <petr.vanek@qoretechnologies.com> 0.0.1
- initial package for Version 0.0.1

