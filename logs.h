/*****************************************************************************
 * logs.h: Utils for the log
 *****************************************************************************
 *
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
#ifndef LOGS_H_INCLUDED
#define LOGS_H_INCLUDED

#define	LOG_NONE			0
#define	LOG_CRITICAL		1
#define	LOG_ERROR			2
#define	LOG_WARNING			3
#define	LOG_INFO			4
#define	LOG_DEBUG 			5


int stringToLogLevel(char * str);
char * logLevelToString(int loglevel);
void setLogFile(int iLevel,char *szFile);
void Logs(int iLevel,char *file,int line,const char *psz_format, ...);

#endif