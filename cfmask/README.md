## CFmask Version 2.0.2 - Release Notes

Release Date: February 2017

See git tag [cfmask-v2.0.2]

This application produces Cloud Mask products for Landsat data based on the
CFMASK (Function of Mask Algorithm).

## Product Descriptions
See the [Surface Reflectance](http://landsat.usgs.gov/CDR_LSR.php) product guide for information about the CFMASK product.

## Release Notes
- Version changes
- Updated for new brightness temperature band naming

## Installation

### Dependencies
- ESPA raw binary libraries, tools, and its dependencies, found here [espa-product-formatter](https://github.com/USGS-EROS/espa-product-formatter)

### Environment Variables
- Required for building this software
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
- Clone the repository and replace the defaulted version(master) with this
  version of the software
```
git clone https://github.com/USGS-EROS/espa-cloud-masking.git
cd espa-cloud-masking
git checkout cfmask-version_<version>
```
- Build and install the application specific software
```
make all-cfmask
make install-cfmask
```

## Usage
See `cloud_masking.py --help` for command line details.<br>
See `cloud_masking.py --xml <xml_file> --help` for command line details specific to the application.<br>
See `cfmask --help` for command line details when the above wrapper script is not called.

### Environment Variables
- PATH - May need to be updated to include the following
  - `$PREFIX/bin`
- ESUN - Points to a directory containing the EarthSunDistance.txt file which is included with the source and installed into `$PREFIX/espa-cloud-masking/static_data`
```
export ESUN="$PREFIX/espa-cloud-masking/static_data"
```

### Data Processing Requirements
This version of the CFMASK application requires the input products to be in the ESPA internal file format.

The following input data are required to generate the cloud masking products:
- Top of Atmosphere Reflectance
- Brightness Temperature

These products can be generated using the [Surface Reflectance](https://github.com/USGS-EROS/espa-surface-reflectance) software found in our [espa-surface-reflectance](https://github.com/USGS-EROS/espa-surface-reflectance) project.  Or through our ondemand processing system [ESPA](https://espa.cr.usgs.gov), be sure to select the ENVI output format.

This cloud masking product is currently available in the [ESPA](https://espa.cr.usgs.gov) processing system as part of the Surface Reflectance product, as well as individually selectable.

### Data Postprocessing
After compiling the [espa-product-formatter](https://github.com/USGS-EROS/espa-product-formatter) libraries and tools, the `convert_espa_to_gtif` and `convert_espa_to_hdf` command-line tools can be used to convert the ESPA internal file format to HDF or GeoTIFF.  Otherwise the data will remain in the ESPA internal file format, which includes each band in the ENVI file format (i.e. raw binary file with associated ENVI header file) and an overall XML metadata file.

## More Information
This project is provided by the US Geological Survey (USGS) Earth Resources
Observation and Science (EROS) Land Satellite Data Systems (LSDS) Science
Research and Development (LSRD) Project. For questions regarding products
produced by this source code, please contact the
[Landsat Contact Us](https://landsat.usgs.gov/contactus.php) page and
specify USGS CDR/ECV in the "Regarding" section.
