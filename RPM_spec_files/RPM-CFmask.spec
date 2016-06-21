
# This spec file can be used to build an RPM package for installation.
# **NOTE**
#     Version, Release, and tagname information should be updated for the
#     particular release to build an RPM for.

%define project espa-cloud-masking
%define algorithm cfmask
%define build_timestamp %(date +"%%Y%%m%%d%%H%%M%%S")
# Specify the repository tag/branch to clone and build from
%define tagname dev_aug2016
# Specify the name of the directory to clone into
%define clonedname %{name}-%{tagname}
# Change the default rpm name format for the rpm built by this spec file
%define _build_name_fmt %%{NAME}.%%{VERSION}.%%{RELEASE}%{?dist}.%{ARCH}.rpm


# ----------------------------------------------------------------------------
Name:		%{project}-%{algorithm}
Version:	2.0.1
Release:	1.%{build_timestamp}
Summary:	ESPA Cloud Masking Software - CFmask

Group:		ESPA
License:	Nasa Open Source Agreement
URL:		https://github.com/USGS-EROS/espa-cloud-masking.git

BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildArch:	x86_64
Packager:	USGS EROS LSRD

#BuildRequires:	espa-product-formatter >= 201608
Requires:	espa-product-formatter >= 201608


# ----------------------------------------------------------------------------
%description
Provides science application executables for generating cloud masking products for Landsat 4, 5, 7, and 8.  These applications are implementated in C and Python.


# ----------------------------------------------------------------------------
%prep
# We don't need to perform anything here

%build
# Start with a clean clone of the repo
rm -rf %{clonedname}
git clone --depth 1 --branch %{tagname} %{url} %{clonedname}
# Build the applications
cd %{clonedname}
make BUILD_STATIC=yes

%install
# Start with a clean installation location
rm -rf %{buildroot}
# Install the applications for a specific path
cd %{clonedname}
make install PREFIX=%{buildroot}/usr/local

%clean
# Cleanup our cloned repository
rm -rf %{clonedname}
# Cleanup our installation location
rm -rf %{buildroot}


# ----------------------------------------------------------------------------
%files
%defattr(-,root,root,-)
# All sub-directories are automatically included
/usr/local/bin/*
/usr/local/%{project}
/usr/local/%{project}/%{algorithm}


# ----------------------------------------------------------------------------
%changelog
* Tue Jun 21 2016 Ronald D Dilley <ronald.dilley.ctr@usgs.gov>
- Initial Version for August 2016 release
