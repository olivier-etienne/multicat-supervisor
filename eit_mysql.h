/*****************************************************************************
 * eit_mysql.h: 
 *****************************************************************************
 * $Id: eit_mysql.h 
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

#ifndef EIT_MYSQL_H_INCLUDED
#define EIT_MYSQL_H_INCLUDED

#define EIT_MYSQL_DESCRIPTION_LENGTH				128
#define EIT_MYSQL_USER_LENGTH							45
#define EIT_MYSQL_ADDRESS_LENGTH					15
#define EIT_MYSQL_VIDEOFILE_LENGTH					255
#define EIT_MYSQL_STATUS_LENGTH						20

typedef struct EitMysql
{
	int id;
	int lcn;
	char description[EIT_MYSQL_DESCRIPTION_LENGTH+1];
	char user[EIT_MYSQL_USER_LENGTH+1];
	char * section0;
	char * section1;
	short enable;
	long timestamp;
	char video[EIT_MYSQL_VIDEOFILE_LENGTH+1];
	char address[EIT_MYSQL_ADDRESS_LENGTH+1];
	int port;
	int tsid;
	int sid;
	int onid;
	char status[EIT_MYSQL_STATUS_LENGTH+1];
} EitMysql;

typedef struct DataBaseInformations
{
	char * host;
	char * username;
	char * password;
	char * database;
	int port;
} DataBaseInformations;

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
int eit_mysql_init(DataBaseInformations infos);
char * eit_mysql_getLastError();
void eit_mysql_destroy();
int eit_mysql_getList(EitMysql ** list,int * size);
void eit_mysql_getList_free(EitMysql *list,int size);
EitMysql * eit_mysql_cpyList(EitMysql *list,int size);
int eit_mysql_setStatus(int id,char * status);

#endif // EIT_MYSQL_H_INCLUDED
