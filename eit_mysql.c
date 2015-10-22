#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql/mysql.h>

#include "eit_mysql.h"

/*****************************************************************************
 * Local declarations
 *****************************************************************************/
static 	MYSQL *conn = NULL;
 static char * error = NULL;
 
 /*****************************************************************************
 * Functions
 *****************************************************************************/
 int eit_mysql_init(DataBaseInformations infos)
 {
   conn = mysql_init(NULL); 
   
   /* Connect to database */
   if (!mysql_real_connect(conn, infos.host,infos.username,infos.password,infos.database,infos.port, NULL, 0)) {
      error = (char*)mysql_error(conn);
      return 1;
   }
   
   return 0;
}

char * eit_mysql_getLastError()
{
	return error;
}

void eit_mysql_destroy()
{
	if ( conn != NULL ) {
		mysql_close(conn);	
	}
}

static char * strcpyWithNull(char *src) 
{
	int strsize = 0;
	char * strptr = "";
	char * dst = NULL;
	
	if ( src == NULL ) { 
		strsize = 0;
	}else {
		strsize = strlen(src);
		strptr = src;
	}
	
	dst = (char*) malloc(sizeof(char) * (strsize + 1));
	memset(dst,'\0',(strsize + 1));
	strcpy(dst,strptr);
	return dst;
}

static int atoiWithNull(char * src) {
	if ( src == NULL ) 
		return 0;
	
	return  atoi(src);
}

static long atolWithNull(char * src) {
	if ( src == NULL ) 
		return 0;
	return  atol(src);
}

void eit_mysql_getList_free(EitMysql *list,int size)
{
	int i;
	for(i=0;i<size;i++) {
		free(list[i].section0);
		free(list[i].section1);
	}
	
	free(list);
}

EitMysql * eit_mysql_cpyList(EitMysql *list,int size)
{	
	EitMysql * cpy = NULL;
	int i;

	cpy = (EitMysql *) malloc(sizeof(EitMysql) * size );
   
	for(i=0;i<size;i++) 
	{
		cpy[i].id = list[i].id;
		cpy[i].lcn = list[i].lcn;
		strcpy(cpy[i].description,list[i].description);
		strcpy(cpy[i].user,list[i].user);	
		cpy[i].section0 = strcpyWithNull(list[i].section0);
		cpy[i].section1 = strcpyWithNull(list[i].section1);
		cpy[i].enable = list[i].enable;
		cpy[i].timestamp = list[i].timestamp;
		strcpy(cpy[i].video,list[i].video);
		strcpy(cpy[i].address,list[i].address);
		cpy[i].port = list[i].port;
		cpy[i].tsid = list[i].tsid;
		cpy[i].sid = list[i].sid;
		cpy[i].onid = list[i].onid;
		strcpy(cpy[i].status,list[i].status);
	}
	
	return cpy;
}

int eit_mysql_getList(EitMysql **list,int * size)
{ 
	MYSQL_RES *res;
	MYSQL_ROW row;
	int index = 0;
	int rowIndex;
	
	if ( conn == NULL ) {
		return -1;
	}
   
	if (mysql_query(conn, "SELECT eit.ideit, eit.lcn, eit.description, eit.user1, eit.section_0, eit.section_1, eit.enable, eit.eit_ts, video.filename, eit.address, eit.port, eit.tsid, eit.sid, eit.onid, eit.status FROM eit INNER JOIN video ON eit.videoid=video.idvideo WHERE eit.enable=1")) {
		error = (char*)mysql_error(conn);
		return -2;
	}

	res = mysql_store_result(conn);
	
	*size = mysql_num_rows(res);	
	if ( *size > 0 ) {
		*list = (EitMysql *) malloc(sizeof(EitMysql) * (*size) );
		memset(*list,0,sizeof(EitMysql) * (*size));
   
		while ((row = mysql_fetch_row(res)) != NULL) {
			rowIndex = 0;
		
			(*list)[index].id = atoi(row[rowIndex++]);
			(*list)[index].lcn = atoiWithNull(row[rowIndex++]);
			strncpy((*list)[index].description,row[rowIndex++],EIT_MYSQL_DESCRIPTION_LENGTH);
			strncpy((*list)[index].user,row[rowIndex++],EIT_MYSQL_USER_LENGTH);	
			(*list)[index].section0 = strcpyWithNull(row[rowIndex++]);
			(*list)[index].section1 = strcpyWithNull(row[rowIndex++]);
			(*list)[index].enable = (short)atoiWithNull(row[rowIndex++]);
			(*list)[index].timestamp = atolWithNull(row[rowIndex++]);
			strncpy((*list)[index].video,row[rowIndex++],EIT_MYSQL_VIDEOFILE_LENGTH);
			strncpy((*list)[index].address,row[rowIndex++],EIT_MYSQL_ADDRESS_LENGTH);
			(*list)[index].port = atoiWithNull(row[rowIndex++]);
			(*list)[index].tsid  = atoiWithNull(row[rowIndex++]);
			(*list)[index].sid = atoiWithNull(row[rowIndex++]);
			(*list)[index].onid  = atoiWithNull(row[rowIndex++]);
			strncpy((*list)[index].status,row[rowIndex++],EIT_MYSQL_STATUS_LENGTH);
			
			index++;
		}
	}
	mysql_free_result(res);
   
   return *size;
}

int eit_mysql_setStatus(int id,char * status)
{
	if ( conn == NULL ) {
		return -1;
	}
 
	char request[255];
	sprintf(request,"UPDATE eit SET status='%s',lastUpdate=NOW() WHERE ideit=%d",status,id);
	if (mysql_query(conn, request)) {
		error = (char*)mysql_error(conn);
		//printf("Erreur MYSQL %s\n",error);
		return -2;
	}
	
	return 0;
}
