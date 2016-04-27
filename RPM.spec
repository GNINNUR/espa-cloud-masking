
# This spec file can be used to build an RPM package for installation.
# **NOTE**
#     Version, Release, and tagname information should be updated for the
#     particular release to build an RPM for.

# ----------------------------------------------------------------------------
Name:		espa-cloud-masking
Version:	201605
Release:	2%{?dist}
Summary:	ESPA Cloud Masking Software

Group:		ESPA
License:	Nasa Open Source Agreement
URL:		https://github.com/USGS-EROS/espa-cloud-masking.git

BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildArch:	x86_64
Packager:	USGS EROS LSRD

BuildRequires:	espa-product-formatter
Requires:	espa-product-formatter >= 201605


# ----------------------------------------------------------------------------
%description
Provides science application executables for generating cloud masking products.  These applications are implementated in C and are statically built.


# ----------------------------------------------------------------------------
# Specify the repository tag/branch to clone and build from
%define tagname dev_may2016
# Specify the name of the directory to clone into
%define clonedname %{name}-%{tagname}


# ----------------------------------------------------------------------------
%prep
# We don't need to perform anything here


# ----------------------------------------------------------------------------
%build

# Start with a clean clone of the repo
rm -rf %{clonedname}
git clone --depth 1 --branch %{tagname} %{url} %{clonedname}
# Build the applications
cd %{clonedname}
make BUILD_STATIC=yes


# ----------------------------------------------------------------------------
%install
# Start with a clean installation location
rm -rf %{buildroot}
# Install the applications for a specific path
cd %{clonedname}
make install PREFIX=%{buildroot}/usr/local

# ----------------------------------------------------------------------------
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
/usr/local/%{name}


# ----------------------------------------------------------------------------
%changelog
* Wed Apr 27 2016 Ronald D Dilley <ronald.dilley.ctr@usgs.gov>
- Updated for a recompile against a support library

* Tue Apr 12 2016 Ronald D Dilley <rdilley@usgs.gov>
- Updated for a recompile against a support library
* Mon Mar 07 2016 Ronald D Dilley <rdilley@usgs.gov>
- Updated release number for a recompile against a support library
* Thu Mar 03 2016 Ronald D Dilley <rdilley@usgs.gov>
- Updated release number for a recompile against a support library
* Mon Jan 25 2016 Ronald D Dilley <rdilley@usgs.gov>
- Updated for Mar 2016 release
* Wed Dec 02 2015 Ronald D Dilley <rdilley@usgs.gov>
- Changed release number for a recompile against the product formatter for Dec 2015 release
* Wed Nov 04 2015 Ronald D Dilley <rdilley@usgs.gov>
- Updated for Dec 2015 release
* Wed Sep 03 2015 Ronald D Dilley <rdilley@usgs.gov>
- Build for Oct 2015 release
* Fri Jun 26 2015 William D Howe <whowe@usgs.gov>
- Using git hub now, cleaned up comments
* Fri May 22 2015 Cory B Turner <cbturner@usgs.gov>
- Rebuild to 1.5.0 for May 2015 release
* Fri Jan 16 2015 Adam J Dosch <adosch@usgs.gov>
- Rebuild against espa-common 1.3.1 to make correction for Landsat 4-7 QA band descriptions and fill value in ledaps-2.2.1/espa-common-1.3.1
* Mon Dec 22 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild to build against espa-common 1.3.0
* Sun Dec 07 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild to capture changes to espa-common for December 2014 release
* Thu Dec 04 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for December 2014 release, new code updates to tagged release 0.1.0
* Fri Nov 07 2014 Adam J Dosch <adosch@usgs.gov>
- Build for November 2014 release, version 1.4.2
* Thu Aug 21 2014 Adam J Dosch <adosch@usgs.gov>
- Build for August 2014 release
- Adding svnrelease to go with the new subversion testing/releases structure vs. using tags
- Updated Release conditional macro to expand if exists, if non-exists must be broke?
- Removing lndcal2 and append_cfmask from builds, it's no longer needed
* Fri Jul 18 2014 Adam J Dosch <adosch@usgs.gov>
- Merging RHEL5 and 6 changes together to maintain one spec file
* Tue Jun 10 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.4.0 release for July 2014 release
- Adding LZMALIB, LZMAINC, JBIGLIB, XML2LIB, XML2INC, ESPALIB and ESPAINC for build requirements for raw binary conversion
- Adding BuildRequires and Requires requirements for espa-common packages
* Thu May 08 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.3.1 release for May 2014 release
* Tue Mar 11 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.3.0 release for March 2014 release
* Tue Feb 04 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 'official' 1.2.1 release for off-cycle release into OPS in 02/2014
* Fri Jan 24 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.1.4 for hot-fix into production.  I guess this will replace 1.2.1 for now.
* Fri Jan 10 2014 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.2.1 for January 2014 release
* Thu Nov 07 2013 Adam J Dosch <adosch@usgs.gov>
- Updating cots paths from /data/static-cots to /data/cots
* Tue Sep 17 2013 Adam J Dosch <adosch@usgs.gov>
- Updated URL to point to Google projects page
* Wed Sep 11 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for cfmask 1.1.3 release.  Updating OPENCVINC, OPENCVLIB, HDFINC and HDFLIB env variables.  Added cotspath macro.
  Fixed .el6 macro reference to .el6
* Thu Aug 08 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for cfmask 1.1.2 to fixed the problematic all water scene problem and address fillminima code issues
  for shadow fills
* Mon Aug 05 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for cfmask 1.1.2 to fix code problem with newline escaping for weave C++ inline code in fillminima
- Adding %post and %postun install logic to remove /home/<user>/.python27_compiled/ on instatllation of new cfmask.  We
  didn't add 'force=1' to rebuild shared-object of fillminima every time, so this was the only other solution to
  initially remove it, so on the next run of cfmask, shared object will be built.
* Mon Jul 29 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for cfmask 1.1.1 and append-cfmask 1.0.1 to address code issue with hardcoded MAX limit for cloud detection
* Mon Jul 22 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for cfmask 1.1.1 and append-cfmask 1.0.1
* Wed Jul 03 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild/repackage of 1.1.0 --- Rebuild to fix more code fall-out.  Small code changes, but going to rebuilt anyway.
* Fri Jun 28 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild/repackage of 1.1.0 --- Song forgot to add fillminima.py into trunk code directory.  Jeezis.
* Fri Jun 28 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild/repackage of 1.1.0 to address 'run_fillminima.py' references in cfmask code and it had DOS carriage returns.  Yish.
* Fri Jun 28 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild/repackage of 1.1.0 to address 'run_fillminima.py' not being a part of the install macro of the Makefile.  Song needs to fix.
  And also hand-hack that run_fillminima.py has an incorrect shebang of '/usr/local/bin/env' instead of '/usr/bin/env'.
* Tue Jun 25 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild/repackage of 1.1.0 to fix minor bug in cfmask while running "append_cfmask" for SWE
* Thu Jun 20 2013 Adam J Dosch <adosch@usgs.gov>
- Rebuild for 1.1.0 release with append_cfmask added.
* Wed Jun 19 2013 Adam J Dosch <adosch@usgs.gov>
- Changing SVN build repository from local to Google Projects.  Also adding build support for append_cfmask
  that is need for the new surface reflectance flow
- Taking out the big %post install hack to insert the CFMASK_PATH in espa/espadev environment.  Going
  to manage that with another script outside the RPM build process.  It really doesn't belong.
- Rebuild to version 1.1.0
* Mon Jun 10 2013 Adam J Dosch <adosch@usgs.gov>
- Adding .el6 to do Distribution tagging on RPMs
* Mon May 20 2013 Adam J Dosch <adosch@usgs.gov>
- Adding removal of codepath's for cfmask and lndcal2 to %clean section
* Fri May 17 2013 Adam J Dosch <adosch@usgs.gov>
- Total build make-over.  Doing entire checkout and source code build from .spec for cfmask and lndcal2 (bundled with)
* Tue Apr 23 2013 Adam J Dosch <adosch@usgs.gov>
- Changed cfmaskcodepath to be /data/cfmask/version_201510 instead to keep source consistency with all the other
  CDR/ECV packages out there I am maintaining
* Thu Mar 28 2013 Adam J Dosch <adosch@usgs.gov>
- Added section to add /usr/local/cfmask/bin to PATH if it doesn't exist for espa and espadev user's .bashrc
* Thu Mar 21 2013 Adam J Dosch <adosch@usgs.gov>
- Added 'Requires' dependency of ledaps package since this will run after ledaps processing on a scene (Note: LEDAPS >= 1.2.0 )
- Building 1.0.1 version of cfmask code
* Wed Mar 20 2013 Adam J Dosch <adosch@usgs.gov>
- Hacked together RPM to package CFmask into RPM form.  TODO is to build entire set start-to-end.
