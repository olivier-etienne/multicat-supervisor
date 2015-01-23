/*****************************************************************************
 * logs.c: Utils for the log
 *****************************************************************************
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
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>

#include <unistd.h>

#include "logs.h"

#define LOG_FILE_LENGTH			255
#define MAX_MSG 1024

#define	STR_NONE			"NONE"
#define	STR_CRITICAL		"CRITICAL"
#define	STR_ERROR			"ERROR"
#define	STR_WARNING			"WARNING"
#define	STR_INFO			"INFO"
#define	STR_DEBUG 			"DEBUG"


/*****************************************************************************
 * Local declarations
 *****************************************************************************/
static char	szLogFile[LOG_FILE_LENGTH+1];				// Nom du fichier de trace
static int	iLogLevel = 0;
static short iIsFile = 0; 
/*****************************************************************************
 * 
 *****************************************************************************/
static int TOOLS_GetTime(int *usec)
{
	struct timeval	tv;
	gettimeofday(&tv,(struct timezone *)0);
	if (usec != NULL) *usec = tv.tv_usec;
	return(tv.tv_sec);
}

/*****************************************************************************
 * 
 *****************************************************************************/
void setLogFile(int iLevel,char *szFile)
{
	memset(szLogFile,'\0',LOG_FILE_LENGTH+1);
	iLogLevel = iLevel;
	if (iLevel) {
		if ( szFile != NULL && *szFile != '\0' ) {
			iIsFile = 1;
			strncpy(szLogFile,szFile,LOG_FILE_LENGTH);
		} else {
			iIsFile = 0;
		}		
		
	}
}

void Logs(int iLevel,char *file,int line,const char *psz_format, ...)
{
    FILE *f;
    struct tm *t;
    time_t thetime;

	if ( iLevel < LOG_CRITICAL && iLevel > LOG_DEBUG ) return;
	
	if (iLevel > iLogLevel ) return;
	
	if ( ! iIsFile ) {
		f = stdout;
	} else {
		f = fopen(szLogFile,"at");
	}
		
    if (f != NULL)
    {
		thetime = TOOLS_GetTime(NULL);
		t = localtime(&thetime);
        va_list args;
        char psz_fmt[MAX_MSG];
        va_start( args, psz_format );
	
        snprintf( psz_fmt, MAX_MSG,"[%02d/%02d/%04d %02d:%02d:%02d] - (%5d) [%s] [%s,%d]: %s\n",
            t->tm_mday,t->tm_mon+1,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec,getpid(),logLevelToString(iLevel),file,line,psz_format);
        vfprintf( f, psz_fmt, args );
		fflush(f);
		if ( iIsFile ) fclose(f);
    }
}

char * logLevelToString(int loglevel)
{
	char* str = NULL;

	switch(loglevel) {
		case LOG_CRITICAL : str = STR_CRITICAL;
			break;
		case LOG_ERROR : str = STR_ERROR;
			break;
		case LOG_WARNING : str = STR_WARNING;
			break;
		case LOG_INFO : str = STR_INFO;
			break;
		case LOG_DEBUG : str = STR_DEBUG;
			break;
		default :
			 str = STR_NONE;
			break;
	}
	return str;
}

int stringToLogLevel(char * str)
{
	int level = 0;
	
	if ( strcmp(str,STR_CRITICAL) == 0 ) {
		level = LOG_CRITICAL;
	} else if ( strcmp(str,STR_ERROR) == 0 ) {
		level = LOG_ERROR;
	} else if ( strcmp(str,STR_WARNING) == 0 ) {
		level = LOG_WARNING;
	} else if ( strcmp(str,STR_INFO) == 0 ) {
		level = LOG_INFO;
	} else if ( strcmp(str,STR_DEBUG) == 0 ) {
		level = LOG_DEBUG;
	} else {
		level = LOG_NONE;
	}
	return level;
}

