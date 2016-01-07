/*****************************************************************************
 * multicat-supervisor.c: 
 *****************************************************************************
 * $Id: multicat-supervisor.c 
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
#include <libgen.h>

#include "util.h"
#include "eit.h"
#include "lib_ini.h"
#include "sharedMemoryLib.h"
#include "eit_mysql.h"
#include "eit_xml.h"
#include "json_parsing.h"
#include "logs.h"

/*****************************************************************************
 * Type definition
 *****************************************************************************/

/*****************************************************************************
 * Prototypage
 *****************************************************************************/

/*****************************************************************************
 * Local declarations
 *****************************************************************************/
static volatile sig_atomic_t b_die = 0;
static char iniFile[512];
//struct EitInfo str_eit_info;
static EitMysql * streamingList = NULL;
static int streamingListSize = 0;
static DataBaseInformations infos;
static char * pathdir;

static void usage(void)
{
    msg_Raw( NULL, "Usage: multicat-supervisor [-L <log level> -F <log file>] -I <ini file>" );
    msg_Raw( NULL, "    -I: configuration ini file" );	
    msg_Raw( NULL, "    -L: log level" );	
    msg_Raw( NULL, "    -F: log file" );	
    msg_Raw( NULL, "    -h: help" );		
    exit(EXIT_FAILURE);
}

/*****************************************************************************
 * Signal Handler
 *****************************************************************************/
static void SigHandler( int i_signal )
{
    b_die = 1;
}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 * 
 *  
 */
EitMysql * search_streaming(EitMysql * list,int size,int id) {
	EitMysql * result = NULL;
	int i;
	for(i=0;i<size;i++) {
		if ( list[i].id == id ) {
			result = &list[i];
			break;
		}
	}
	
	return result;
}


/**
 *  @brief 
 *  @param[in] 
 *  @return 
 * 
 *  
 */
static int convertStringsToEitStruct(EitMysql * ptr,struct EitInfo * eit_info)
{
	char videoPath[512];
	eitXml_t *eitXml = NULL; 
	eitInfoXml_t *eitInfoXml = NULL;

	eit_info->tsid = ptr->tsid;
	eit_info->sid = ptr->sid;
	eit_info->onid = ptr->onid;

	if (ptr->to_inject == 0) {
		// Init EIT XML
		if (NULL == (eitXml = init_eit_xml()))
			return -3;

		// load eit xml from ts file
		sprintf(videoPath, "/usr/local/multicat-tools/videos/%s", ptr->video);
		if (0 != extract_eti_xml_from_ts(eitXml, videoPath))
			return -3;

		// Init Eit XML
		if (NULL == (eitInfoXml = init_eit_info_xml()))
			return -3;

		// build eit from xml
		if (0 != (convert_xml_to_eit_struct(eitXml, eitInfoXml)))
			return -3;
	}
	
	if ( *ptr->section0 != '\0' ) {
		json_object * jObjSection0 = json_object_new_object();

		jObjSection0 = (ptr->to_inject == 1) ? json_tokener_parse(ptr->section0) : convert_eit_struct_to_json(eitInfoXml->section0);
		Logs(LOG_INFO,__FILE__,__LINE__,"section 0 json object of %s is : \n%s", ptr->video, json_object_to_json_string(jObjSection0));

		struct EitInfoSection * st_eitSection0 = json_parse(jObjSection0);
		if ( st_eitSection0 != NULL ) {
			memcpy(&eit_info->section0,st_eitSection0,sizeof(struct EitInfoSection));
			free(st_eitSection0);
		}
		else {
			return -2;
		}
		// clean the json object
  		json_object_put(jObjSection0);
	}
	else {
		Logs(LOG_ERROR,__FILE__,__LINE__,"Missing section 0 in database");
		return -1;
	}
	if ( *ptr->section1 != '\0' ) {
		json_object * jObjSection1 = json_object_new_object();
		
		jObjSection1 = (ptr->to_inject == 1) ? json_tokener_parse(ptr->section1) : convert_eit_struct_to_json(eitInfoXml->section1);
		Logs(LOG_INFO,__FILE__,__LINE__,"section 1 json object of %s is : \n%s", ptr->video, json_object_to_json_string(jObjSection1));

		struct EitInfoSection * st_eitSection1 = json_parse(jObjSection1);
		if ( st_eitSection1 != NULL ) {
			memcpy(&eit_info->section1,st_eitSection1,sizeof(struct EitInfoSection));
			free(st_eitSection1);
		}
		else {
			return -2;
		}
		// clean the json object
		json_object_put(jObjSection1);
	}

	if (ptr->to_inject == 0) {
		free_eit_info_xml(eitInfoXml);
		free_eit_xml(eitXml);
	}

	//dump_eit_info(eit_info);
	return 0;
}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 * 
 *  
 */
static void check_streaming_list_with_db(EitMysql * bdList,int bdListSize)
{
	int i=0;
	int action=0;
	
	for(i=0;i<streamingListSize;i++) {
	
		EitMysql * ptr = search_streaming(bdList,bdListSize,streamingList[i].id);
		if ( ptr == NULL ) {
			// DELETE STREAMING
			eit_mysql_setStatus(streamingList[i].id,"STOPPING");
			Logs(LOG_INFO,__FILE__,__LINE__,"DELETE STREAMING [%d,%d,%s,%s,%s,%d]",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
			char cmd[512];
			sprintf(cmd,"%s/../sh/multicat.sh --provider %d stop",pathdir,streamingList[i].id);
			Logs(LOG_DEBUG,__FILE__,__LINE__,"cmd : %s",cmd);
			int ret = system(cmd);
			if ( ret != 0 ) {
				eit_mysql_setStatus(streamingList[i].id,"STOP FAILURE");
				Logs(LOG_ERROR,__FILE__,__LINE__,"STOP STREAMING [%d,%d,%s,%s,%s,%d] FAILED",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
			}else {				
				eit_mysql_setStatus(streamingList[i].id,"STOPPED");
				Logs(LOG_INFO,__FILE__,__LINE__,"STOP STREAMING [%d,%d,%s,%s,%s,%d] DONE",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
			}
			action=1;
		} else {
			if ( ptr->timestamp > streamingList[i].timestamp ) {
				action=1;
				Logs(LOG_DEBUG,__FILE__,__LINE__,"UDPATE EIT [%d,%d,%s]",ptr->id,ptr->lcn,ptr->description);
				// UPDATE EIT
				struct EitInfo eit_info;
				switch (convertStringsToEitStruct(ptr, &eit_info)) {
					case 0:
						sharedMemory_set(ptr->id,&eit_info);
						break;
					case -1:
						eit_mysql_setStatus(bdList[i].id,"DATABASE FAILURE");
						break;
					case -2 :
						eit_mysql_setStatus(bdList[i].id,"PARSING FAILURE");
						break;
					case -3 :
						eit_mysql_setStatus(bdList[i].id,"EIT FAILURE");
						break;						
				}
			}
		}
	
	}
	
	for(i=0;i<bdListSize;i++) {
	
		EitMysql * ptr = search_streaming(streamingList,streamingListSize,bdList[i].id);
		if ( ptr == NULL ) {
			// NEW STREAMING
			eit_mysql_setStatus(bdList[i].id,"STARTING");
			Logs(LOG_INFO,__FILE__,__LINE__,"NEW STREAMING [%d,%d,%s,%s,%s,%d]",bdList[i].id,bdList[i].lcn,bdList[i].description,bdList[i].video,bdList[i].address,bdList[i].port);
			struct EitInfo eit_info;
			switch (convertStringsToEitStruct(&bdList[i],&eit_info)) {
				case 0:
					sharedMemory_set(bdList[i].id,&eit_info);
					if ( *bdList[i].video != '\0' && *bdList[i].address != '\0' ) {
						char cmd[512];
						sprintf(cmd,"%s/../sh/multicat.sh --provider %d --video %s --address %s --port %d start",pathdir,bdList[i].id,bdList[i].video,bdList[i].address,bdList[i].port);
						Logs(LOG_DEBUG,__FILE__,__LINE__,"cmd : %s",cmd);
						int ret = system(cmd);
						if ( ret != 0 ) {
							eit_mysql_setStatus(bdList[i].id,"START FAILURE");
							Logs(LOG_ERROR,__FILE__,__LINE__,"START STREAMING [%d,%d,%s,%s,%s,%d] FAILED",bdList[i].id,bdList[i].lcn,bdList[i].description,bdList[i].video,bdList[i].address,bdList[i].port);
						} else {
							Logs(LOG_INFO,__FILE__,__LINE__,"START STREAMING [%d,%d,%s,%s,%s,%d] DONE",bdList[i].id,bdList[i].lcn,bdList[i].description,bdList[i].video,bdList[i].address,bdList[i].port);
							eit_mysql_setStatus(bdList[i].id,"STARTED");
						}
						action=1;
					}
					break;
				case -1:
					eit_mysql_setStatus(bdList[i].id,"DATABASE FAILURE");
					break;
				case -2 :
					eit_mysql_setStatus(bdList[i].id,"PARSING FAILURE");
					break;
				case -3 :
					eit_mysql_setStatus(bdList[i].id,"EIT FAILURE");
					break;						
			}
		}
	}
	
	if ( action == 0 ) {
		eit_mysql_getList_free(bdList,bdListSize);
		Logs(LOG_DEBUG,__FILE__,__LINE__,"NOTHING TO DO");
	}
	else {
		if ( streamingList != NULL ) {
			eit_mysql_getList_free(streamingList,streamingListSize);
			streamingList = NULL;
			streamingListSize = 0;
		}
		streamingList = bdList;
		streamingListSize = bdListSize;
	}
}

// static void dump_eit_mysql(EitMysql * st,int size)
// {
// 	int i;	
	
// 	for(i=0;i<size;i++) {
		
// 		printf("--------------------------------------------------------------------------------\n");
// 		printf("- ID			: %d\n",st[i].id);
// 		printf("- LCN			: %d\n",st[i].lcn);
// 		printf("- DESCRIPTION	: %s\n",st[i].description);
// 		printf("- USER			: %s\n",st[i].user);
// 		printf("- SECTION 0		: %s\n",st[i].section0);
// 		printf("- SECTION 1		: %s\n",st[i].section1);
// 		printf("- ENABLE		: %d\n",st[i].enable);
// 		printf("- TO_INJECT		: %d\n",st[i].to_inject);
// 		printf("- TIMESTAMP		: %ld\n",st[i].timestamp);
// 		printf("- VIDEO			: %s\n",st[i].video);
// 		printf("- ADDRESS		: %s\n",st[i].address);
// 		printf("- PORT			: %d\n",st[i].port);
// 		printf("- TSID			: %d\n",st[i].tsid);
// 		printf("- SID			: %d\n",st[i].sid);
// 		printf("- ONID			: %d\n",st[i].onid);
// 		printf("- STATUS		: %s\n",st[i].status);
// 		printf("--------------------------------------------------------------------------------\n");
		
// 	}


// }

static void stop_all_streaming()
{
	int i;	
	
	if ( streamingList == NULL ) return;
	
	for(i=0;i<streamingListSize;i++) {
		Logs(LOG_INFO,__FILE__,__LINE__,"STOP STREAMING [%d,%d,%s,%s,%s,%d]",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
		char cmd[512];
		sprintf(cmd,"%s/../sh/multicat.sh --provider %d stop",pathdir,streamingList[i].id);
		Logs(LOG_DEBUG,__FILE__,__LINE__,"cmd : %s",cmd);
		int ret = system(cmd);
		if ( ret != 0 ) {
			eit_mysql_setStatus(streamingList[i].id,"STOP FAILURE");		
			Logs(LOG_ERROR,__FILE__,__LINE__,"STOP STREAMING [%d,%d,%s,%s,%s,%d] FAILED",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
		} else {
			eit_mysql_setStatus(streamingList[i].id,"STOPPED");
			Logs(LOG_INFO,__FILE__,__LINE__,"STOP STREAMING [%d,%d,%s,%s,%s,%d] DONE",streamingList[i].id,streamingList[i].lcn,streamingList[i].description,streamingList[i].video,streamingList[i].address,streamingList[i].port);
		}
	}
	
	eit_mysql_getList_free(streamingList,streamingListSize);
	streamingList = NULL;
	streamingListSize = 0;

}

/*****************************************************************************
 * Entry point
 *****************************************************************************/
int main( int i_argc, char **pp_argv )
{
    int c;
    struct sigaction sa;
    sigset_t set;
	EitMysql * bdList = NULL;
	int bdListSize = 0;
	int i_sleep = 10;
	int hasIniFile=0;
	int logLevel = LOG_CRITICAL;
	char logfile[129];
	
	memset(logfile,'\0',129);

	pathdir = dirname(pp_argv[0]);
	
    /* Parse options */
    while ( (c = getopt( i_argc, pp_argv, "I:hL:F:" )) != -1 )
    {
        switch ( c )
        {
		case 'I' :
			hasIniFile=1;
			strncpy(iniFile,optarg,512);
			break;
		case 'h':
        default:
            usage();
            break;
        }
    }
    if ( hasIniFile == 0 )
        usage();
		
	if ( ChargerFichierIni(iniFile) != INI_SUCCESS ) {
		fprintf(stderr,"Load ini file failed");
        exit(EXIT_FAILURE);	
	}

	logLevel = stringToLogLevel(rechercherValeur("SUPERVISOR_LOGS","level"));
	strncpy(logfile,rechercherValeur("SUPERVISOR_LOGS","file"),128);
		
	setLogFile(logLevel,logfile);

	char * map_file = rechercherValeur("SHARED_MEMORY","file");
	int streamingProvider = atoi(rechercherValeur("SHARED_MEMORY","streamingProvider"));
	i_sleep = atoi(rechercherValeur("SUPERVISOR","sleep"));	
	infos.host = rechercherValeur("SUPERVISOR","host");
	infos.username = rechercherValeur("SUPERVISOR","username");
	infos.password = rechercherValeur("SUPERVISOR","password");
	infos.database = rechercherValeur("SUPERVISOR","database");
	infos.port = atoi(rechercherValeur("SUPERVISOR","port"));
	
	if ( sharedMemory_init(map_file,streamingProvider) != 0 ) {
		Logs(LOG_CRITICAL,__FILE__,__LINE__,"Load shared memory mapping file failed" );
		exit(EXIT_FAILURE);	
	}


    /* Set signal handlers */
    memset( &sa, 0, sizeof(struct sigaction) );
    sa.sa_handler = SigHandler;
    sigfillset( &set );

    if ( sigaction( SIGTERM, &sa, NULL ) == -1 ||
         sigaction( SIGHUP, &sa, NULL ) == -1 ||
         sigaction( SIGINT, &sa, NULL ) == -1 ||
         sigaction( SIGPIPE, &sa, NULL ) == -1 )
    {
		Logs(LOG_CRITICAL,__FILE__,__LINE__,"Couldn't set signal handler: %s", strerror(errno) );
        exit(EXIT_FAILURE);
    }
	
	if ( eit_mysql_init(infos) != 0 ) {
		char * error = eit_mysql_getLastError();
		if ( error != NULL ) {
			Logs(LOG_CRITICAL,__FILE__,__LINE__,"Connection Database informations EIT failed / %s",error);
		} else {
			Logs(LOG_CRITICAL,__FILE__,__LINE__,"Connection Database informations EIT failed");
		}
        exit(EXIT_FAILURE);
	}
 
    /* Main loop */
    while ( !b_die )
    {
		Logs(LOG_DEBUG,__FILE__,__LINE__,"GET LIST INTO DB");
		eit_mysql_getList(&bdList,&bdListSize);
		//dump_eit_mysql(bdList,bdListSize);
		check_streaming_list_with_db(bdList,bdListSize);
		bdList = NULL;
		bdListSize = 0;
		Logs(LOG_DEBUG,__FILE__,__LINE__,"SLEEP");
		sleep(i_sleep);
    }

	Logs(LOG_DEBUG,__FILE__,__LINE__,"STOP");
	stop_all_streaming();
	eit_mysql_destroy();
	Logs(LOG_DEBUG,__FILE__,__LINE__,"MYSQL CLOSES");
	sharedMemory_close();
	Logs(LOG_DEBUG,__FILE__,__LINE__,"SHARED MEMORY CLOSED");
	Logs(LOG_DEBUG,__FILE__,__LINE__,"BYE");


    return b_die ? EXIT_FAILURE : EXIT_SUCCESS;
}

