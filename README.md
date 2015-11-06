## ESPA Cloud Masking

Release Date: December XX, 2015

See git tag [2015_Dec]

This project contains application source code for producing Cloud Mask products.

## Implemented Algorithms

### CFMASK - Function of Mask (Algorithm)
* Implemented in C
* This software is based on the Matlab code developed by Zhe Zhu, and
  Curtis E. Woodcock
  * Zhu, Z. and Woodcock, C. E., Object-based cloud and cloud shadow detection in Landsat imagery, Remote Sensing of Environment (2012), doi:10.1016/j.rse.2011.10.028 
  * More information from the algorithm developers can be found [here](https://github.com/prs021/fmask).

* See folder <b>l4-7_cfmask</b> for the Landsat 4, 5, and 7 version.
* See folder <b>not-validated-prototype-l8_cfmask</b> for the Landsat 8
  version.

## Release Notes
Please see the Wiki pages or Release Tags for notes related to past versions.

- Replaced fillminima (python/C) code with code developed by the Landsat Team.
- Replaced cloud labeling code with code developed by the Landsat Team.
- Added minimal support for multi-threading at compile time.
- Many other enhancements due to the above two items.  Specifically/mostly the memory allocation and usage.
- Added a helper script for future use with multiple sensor and/or algorithm applications. Helper script is intended to be called by the espa-processing project code.
- Enhanced the Makefiles for build and installation.

## Installation Notes

### Installation of Specific Algorithms
Please see the installation instructions within the algorithm sub-directory.

### Installation of All Algorithms

### Dependencies
* ESPA raw binary libraries, tools, and it's dependencies, found here [espa-product-formatter](https://github.com/USGS-EROS/espa-product-formatter)

### Environment Variables
* Required for building this software
```
export PREFIX="path_to_Installation_Directory"
export XML2INC="path_to_LIBXML2_include_files"
export XML2LIB="path_to_LIBXML2_libraries_for_linking"
export LZMALIB="path_to_LZMA_libraries_for_linking"
export ZLIBLIB="path_to_ZLIB_libraries_for_linking"
export ESPAINC="path_to_ESPA_PRODUCT_FORMATTER_include_files"
export ESPALIB="path_to_ESPA_PRODUCT_FORMATTER_libraries_for_linking"
```

### Build Steps
* Clone the repository and replace the defaulted version(master) with this
  version of the software
```
git clone https://github.com/USGS-EROS/espa-cloud-masking.git
cd espa-cloud-masking
git checkout <version>
```
* Build and install the application specific software
```
make
make install
```

## Usage
See the algorithm specific sub-directories for details on usage.

## More Information
This project is provided by the US Geological Survey (USGS) Earth Resources
Observation and Science (EROS) Land Satellite Data Systems (LSDS) Science
Research and Development (LSRD) Project. For questions regarding products
produced by this source code, please contact the Landsat Contact Us page and
specify USGS CDR/ECV in the "Regarding" section.
https://landsat.usgs.gov/contactus.php 
