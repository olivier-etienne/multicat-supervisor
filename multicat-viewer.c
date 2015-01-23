/*****************************************************************************
 * multicat-viewer.c: netcat-equivalent for multicast
 *****************************************************************************
 * Copyright (C) 2009, 2011-2012 VideoLAN
 * $Id: multicat-viewer.c 
 *
 * Authors: 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/* POLLRDHUP */
#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "util.h"
#include "eit.h"
#include "lib_ini.h"
#include "sharedMemoryLib.h"
#include "logs.h"

/*****************************************************************************
 * Local declarations
 *****************************************************************************/
static char iniFile[512];
struct EitInfo str_eit_info;
static int providerNumber;

static void usage(void)
{
    msg_Raw( NULL, "Usage: multicat-viewer -l [-I <ini file>] [-L <log level> -F <log file>] [ -P <provider number>] " );
    msg_Raw( NULL, "    -I: configuration ini file" );	
    msg_Raw( NULL, "    -P: streaming provider number" );		
    msg_Raw( NULL, "    -L: log level" );	
    msg_Raw( NULL, "    -F: log file" );	
    msg_Raw( NULL, "    -l: load EIT information from ini file" );	
    exit(EXIT_FAILURE);
}

/*****************************************************************************
 * Entry point
 *****************************************************************************/
int main( int i_argc, char **pp_argv )
{
	providerNumber=1;
	int logLevel = LOG_CRITICAL;
	char logfile[129];
    int c;
	int load=0;

	memset(logfile,'\0',129);
	
    /* Parse options */
    while ( (c = getopt( i_argc, pp_argv, "lL:F:I:hP:" )) != -1 )
    {
        switch ( c )
        {		
		case 'I' :
			strncpy(iniFile,optarg,512);
			break;
			
		case 'P' :
			providerNumber = atoi(optarg);
			break;
		case 'L' : 
			logLevel = stringToLogLevel(optarg);
			break;
		case 'F' :
			strncpy(logfile,optarg,128);
			break;			
		case 'l' :
			load = 1;
			break;
        case 'h':
        default:
            usage();
            break;
        }
    }
	
	if ( providerNumber < 1 ) 
		usage();
		
	setLogFile(logLevel,logfile);
	//Logs(LOG_INFO,__FILE__,__LINE__,"Starting streaming provider %d",providerNumber);
		
	if ( ChargerFichierIni(iniFile) != INI_SUCCESS ) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"Load ini file failed provider %d",providerNumber);
		exit(EXIT_FAILURE);	
	}

	char * map_file = rechercherValeur("SHARED_MEMORY","file");
	int streamingProvider = atoi(rechercherValeur("SHARED_MEMORY","streamingProvider"));
	
	//Logs(LOG_DEBUG,__FILE__,__LINE__,"Shared Memory Init provider %d",providerNumber);
	if ( sharedMemory_init(map_file,streamingProvider) != 0 ) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"Load shared memory mapping file failed provider %d",providerNumber);
		exit(EXIT_FAILURE);	
	}

	if ( load == 1 ) {
		load_eit(&str_eit_info);
		sharedMemory_set(providerNumber,&str_eit_info);
	} else {
		sharedMemory_getWithoutUpdate(providerNumber,&str_eit_info);
		if ( str_eit_info.initialized == 0 ) {
			printf("Provider %d : Shared Memory not initialized\n",providerNumber);
			load_eit(&str_eit_info);
			sharedMemory_set(providerNumber,&str_eit_info);
		}
		dump_eit_info(&str_eit_info);

	}

	sharedMemory_close();
	//Logs(LOG_INFO,__FILE__,__LINE__,"multicat-viewer Provider %d STOPPED",providerNumber);	

    return EXIT_SUCCESS;
}

