Name:           omnitranslate
Version:        1.0.0
Release:        1%{?dist}
Summary:        All in one (NO LANGUAGES)
License:        MIT
URL:            https://github.com/UIFreestyler/omnistranslate
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++, cmake, make

%description
TUI Tool 

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%{_bindir}/omnitranslate

%changelog
* Fri Mar 07 2025 zenity <heliumail7@email.com> - 1.0.0-1
- non stuff stuff
