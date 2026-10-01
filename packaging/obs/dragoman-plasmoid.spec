#
# spec file for package dragoman-plasmoid (openSUSE and Fedora targets on OBS)
#
# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# @VERSION@ is stamped by packaging/obs/prepare.sh from the release tag; the
# release tarball carries it in .tarball-version as well, for CMake.

%define applet_id dev.l10n_bg.dragomand.translator

Name:           dragoman-plasmoid
Version:        @VERSION@
Release:        0
Summary:        Plasma widget for offline translation through Dragomand
License:        GPL-3.0-or-later
URL:            https://dragomand.l10n-bg.dev
Source0:        %{name}-%{version}.tar.gz

# The dragomand daemon it talks to exists for these two only.
ExclusiveArch:  x86_64 aarch64

BuildRequires:  cmake >= 3.24
BuildRequires:  gcc-c++
BuildRequires:  gettext
BuildRequires:  cmake(DragomanQt)
BuildRequires:  cmake(KF6Config) >= 6.13
BuildRequires:  cmake(KF6CoreAddons) >= 6.13
BuildRequires:  cmake(KF6I18n) >= 6.13
BuildRequires:  cmake(Plasma) >= 6.3
BuildRequires:  cmake(Qt6Core) >= 6.8
BuildRequires:  cmake(Qt6DBus) >= 6.8
BuildRequires:  cmake(Qt6Gui) >= 6.8
BuildRequires:  cmake(Qt6Qml) >= 6.8
BuildRequires:  cmake(Qt6Quick) >= 6.8
BuildRequires:  cmake(Qt6Test) >= 6.8
%if 0%{?fedora}
BuildRequires:  extra-cmake-modules >= 6.13
BuildRequires:  ninja-build
BuildRequires:  dbus-daemon
# QML modules the widget imports at run time.
Requires:       qt6qml(org.kde.kcmutils)
Requires:       qt6qml(org.kde.kirigami)
Requires:       qt6qml(org.kde.plasma.components)
%else
BuildRequires:  kf6-extra-cmake-modules >= 6.13
BuildRequires:  ninja
BuildRequires:  dbus-1
# QML modules the widget imports at run time.
Requires:       qt6qmlimport(org.kde.kcmutils)
Requires:       qt6qmlimport(org.kde.kirigami)
Requires:       qt6qmlimport(org.kde.plasma.components)
%endif
Requires:       dragomand

%description
An Offline Translator widget for the Plasma 6 panel and desktop. It
translates text fully offline through the Dragomand daemon and Mozilla's
Firefox translation models; a language pair that is not installed yet is
downloaded on first use.

%prep
%setup -q

%build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DKDE_INSTALL_USE_QT_SYS_PATHS=ON \
    -DBUILD_TESTING=ON
cmake --build build %{?_smp_mflags}

%check
ctest --test-dir build --output-on-failure

%install
DESTDIR=%{buildroot} cmake --install build
%find_lang plasma_applet_%{applet_id}

%files -f plasma_applet_%{applet_id}.lang
%license LICENSES/GPL-3.0-or-later.txt
%doc README.md
%dir %{_libdir}/qt6/plugins/plasma
%dir %{_libdir}/qt6/plugins/plasma/applets
%{_libdir}/qt6/plugins/plasma/applets/%{applet_id}.so

%changelog
* @RPM_DATE@ Blagovest Petrov <blagovest@petrovs.info> - @VERSION@
- Release @VERSION@
