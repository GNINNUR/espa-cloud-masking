#! /usr/bin/env python
'''
    PURPOSE: Determine which executable to run and then passes all arguments
        through to the appropriate script.
    PROJECT: Land Satellites Data Systems Science Research and Development
             (LSRD) at the USGS EROS

    LICENSE: NASA Open Source Agreement 1.3

    AUTHOR: ngenetzky@usgs.gov

    NOTES:
        This script does not have its own help message and will just return the
            help from underlying executables where appropriate.
        If this script has a required argument then only the usage for that
            argument will be shown if that argument is not included.
        All output from the underlying script will be given to the logger as an
            info message.
'''
import argparse
import os
import logging
import sys
import commands


def get_logger():
        '''Replicated from ESPA logger module'''
        # Setup the Logger with the proper configuration
        logging.basicConfig(format=('%(asctime)s.%(msecs)03d %(process)d'
                                    ' %(levelname)-8s'
                                    ' %(filename)s:%(lineno)d:'
                                    '%(funcName)s -- %(message)s'),
                            datefmt='%Y-%m-%d %H:%M:%S',
                            level=logging.INFO)
        return logging.getLogger(__name__)


class ExecuteError(Exception):
    '''Raised when command in execute_cmd returns with error'''
    def __init__(self, message, *args):
        self.message = message
        Exception.__init__(self, message, *args)


def execute_cmd(cmd_string):
        '''Execute a command line and return the terminal output

        Raises:
            ExecuteError (Stdout/Stderr)

        Returns:
            output:The stdout and/or stderr from the executed command.
        '''
        (status, output) = commands.getstatusoutput(cmd_string)

        if status < 0:
            message = ('Application terminated by signal [{0}]'
                       .format(cmd_string))
            if len(output) > 0:
                message = ' Stdout/Stderr is: '.join([message, output])
            raise ExecuteError(message)

        if status != 0:
            message = 'Application failed to execute [{0}]'.format(cmd_string)
            if len(output) > 0:
                message = ' Stdout/Stderr is: '.join([message, output])
            raise ExecuteError(message)

        if os.WEXITSTATUS(status) != 0:
            message = ('Application [{0}] returned error code [{1}]'
                       .format(cmd_string, os.WEXITSTATUS(status)))
            if len(output) > 0:
                message = ' Stdout/Stderr is: '.join([message, output])
            raise ExecuteError(message)

        return output


def parse_only_xml():
        '''Will only parse --xml XML_FILENAME from cmdline.

        Precondition:
            '--xml FILENAME' exists in command line arguments
        Postcondition:
            returns xml_filename
        Note: Help is not included because the program will return
            the help from the underlying program.
        '''
        # Try to parse out the XML so the exe can be determined
        parse_xml = argparse.ArgumentParser(add_help=False)
        parse_xml.add_argument('--xml', action='store',
                               dest='xml_filename', required=True,
                               help='Input XML metadata file',
                               metavar='FILE')
        (temp, extra_args) = parse_xml.parse_known_args()
        return temp.xml_filename


def is_landsat8(xml_filename):
        '''Reads xml_filename string to determine if satellite is Landsat8.

        Precondition:
            (1) satellite_code is the first 3 characters of input product id.
            (2) satellite_code is ['lc8','lo8'] or ['LT4', 'LT5', 'LE7'].
        Postcondition:
            returns True if this satellite_code is in ['lc8','lo8'].
            returns False if this satellite_code is in ['LT4', 'LT5', 'LE7'].
            raises Exception if precondition 2 is violated.
        '''
        satellite_code = xml_filename[0:3]
        l8_prefixes = ['LC8', 'LO8']
        other_prefixes = ['LT4', 'LT5', 'LE7']
        if satellite_code in l8_prefixes:
            return True
        elif satellite_code in other_prefixes:
            return False
        else:
            raise Exception('Satellite code ({0}) from {1} not understood'
                            .format(satellite_code, xml_filename))


def get_executable(isLandsat8):
    '''Returns name of executable that needs to be called'''
    if(isLandsat8):
        return 'l8cfmask'
    else:
        return 'cfmask'


def main():
    '''Determines executable, and calls it with all input arguments '''
    logger = get_logger()

    xml_filename = parse_only_xml()
    isLandsat8 = is_landsat8(xml_filename)

    cmd = [get_executable(isLandsat8)]
    cmd.extend(sys.argv[1:])  # Pass all arguments through
    cmd_string = ' '.join(cmd)
    try:
        logger.info('>>'+cmd_string)
        output = execute_cmd(cmd_string)

        if len(output) > 0:
            logger.info('\n{0}'.format(output))
    except ExecuteError:
        logger.exception('Error running {0}.'
                         'Processing will terminate.'
                         .format(os.path.basename(__file__)))
        raise  # Re-raise so exception message will be shown.

if __name__ == '__main__':
    main()

