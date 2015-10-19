/*****************************************************************************
 * json_parsing.c: 
 *****************************************************************************
 * $Id: json_parsing.c 
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
#include <ctype.h>

#include "util.h"
#include "eit.h"
#include "json_parsing.h"
#include "logs.h"

/*****************************************************************************
 * Type definition
 *****************************************************************************/


/*****************************************************************************
 * Prototypage
 *****************************************************************************/
static int json_parse_component_descriptor(json_object * jarray,struct EitInfoSection * eitStrInfo);
static int json_parse_short_event_descriptor_items(json_object * jarray,struct EitInfoSection * eitStrInfo);
static int json_parse_short_event_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo);
static int json_parse_extended_event_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo);
static int json_parse_parental_rating_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo);

/*****************************************************************************
 * Local declarations
 *****************************************************************************/

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 * 
 *  printing the value corresponding to boolean, double, integer and strings
 */
 
//static void print_json_value(json_object *jobj){
//  enum json_type type;
//
//  type = json_object_get_type(jobj); /*Getting the type of the json object*/
//  switch (type) {
//    case json_type_boolean: printf("json_type_boolean\n");
//                         printf("value: %sn", json_object_get_boolean(jobj)? "true": "false");
//                         break;
//    case json_type_double: printf("json_type_double\n");
//                        printf("          value: %lfn", json_object_get_double(jobj));
//                         break;
//    case json_type_int: printf("json_type_int\n");
//                        printf("          value: %d\n", json_object_get_int(jobj));
//                         break;
//    case json_type_string: printf("json_type_string\n");
//                         printf("          value: %s\n", json_object_get_string(jobj));
//                         break;
//	default :
//		break;
//  }
//
//}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 * 
 */
// static void json_parse_array( json_object *jobj, char *key) {
//  enum json_type type;
//
//  json_object *jarray = jobj; /*Simply get the array*/
//  if(key) { 
//	json_object_object_get_ex(jobj, key,&jarray);
//	//jarray = json_object_object_get(jobj, key); /*Getting the array if it is a key value pair*/
//  }

//  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
//  printf("Array Length: %d\n",arraylen);
//  int i;
//  json_object * jvalue;

//  for (i=0; i< arraylen; i++){
//    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
//    type = json_object_get_type(jvalue);
//    if (type == json_type_array) {
//      json_parse_array(jvalue, NULL);
//    }
//    else if (type != json_type_object) {
//      printf("value[%d]: ",i);
//     print_json_value(jvalue);
//    }
//    else {
//      json_parse(jvalue);
//    }
//  }
//}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_component_descriptor(json_object * jarray,struct EitInfoSection * eitStrInfo)
{
  enum json_type type;
  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
  int i;
  json_object * jvalue;

  for (i=0; i< arraylen && i < COMPONENTDESC_SIZE; i++) 
  {
    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
    type = json_object_get_type(jvalue);
    if (type != json_type_object) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (component_descriptor item[%d]) bad type, Object expected\n",i);
		return -1;
    }

	json_object_object_foreach(jvalue, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"stream_content") == 0 ) {
			switch(type) {
				case json_type_string :
					sscanf(json_object_get_string(val),"%x",&eitStrInfo->component_desc[i].stream_content);
					break;
				case json_type_int :
					eitStrInfo->component_desc[i].stream_content = json_object_get_int(val);
					break;
				default :
					Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String or integer expected\n",key);
					return -1;
			}
		}
		else if ( strcmp(key,"component_type") == 0 ) {
			switch(type) {
				case json_type_string :
					sscanf(json_object_get_string(val),"%x",&eitStrInfo->component_desc[i].component_type);
					break;
				case json_type_int :
					eitStrInfo->component_desc[i].component_type = json_object_get_int(val);
					break;
				default :
					Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String or integer expected\n",key);
					return -1;
			}
		}
		else if ( strcmp(key,"set_component_tag") == 0 ) {
			if ( type != json_type_int ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, integer expected\n",key);
				return -1;
			}
			eitStrInfo->component_desc[i].set_component_tag = json_object_get_int(val);
		}
		else if ( strcmp(key,"text") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->component_desc[i].text,json_object_get_string(val),COMPONENTDESC_TEXT_LENGTH);
		}
		else if ( strcmp(key,"lang") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->component_desc[i].lang,json_object_get_string(val),COMPONENTDESC_LANG_LENGTH);
		}		
	}
  }
  return 0;

}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_content_descriptor(json_object * jarray,struct EitInfoSection * eitStrInfo)
{
  enum json_type type;
  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
  int i;
  json_object * jvalue;

  for (i=0; i< arraylen && i < CONTENT_DESC_SIZE; i++) 
  {
    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
    type = json_object_get_type(jvalue);
    if (type != json_type_object) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (component_descriptor item[%d]) bad type, Object expected\n",i);
		return -1;
    }

	json_object_object_foreach(jvalue, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"content_nibble_level_1") == 0 ) {
			switch(type) {
				case json_type_string :
					sscanf(json_object_get_string(val),"%x",&eitStrInfo->content_desc[i].level_1);
					break;
				case json_type_int :
					eitStrInfo->content_desc[i].level_1 = json_object_get_int(val);
					break;
				default :
					Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String or integer expected\n",key);
					return -1;
			}
		}
		else if ( strcmp(key,"content_nibble_level_2") == 0 ) {
			switch(type) {
				case json_type_string :
					sscanf(json_object_get_string(val),"%x",&eitStrInfo->content_desc[i].level_2);
					break;
				case json_type_int :
					eitStrInfo->content_desc[i].level_2 = json_object_get_int(val);
					break;
				default :
					Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String or integer expected\n",key);
					return -1;
			}
		}
		else if ( strcmp(key,"user") == 0 ) {
			switch(type) {
				case json_type_string :
					sscanf(json_object_get_string(val),"%x",&eitStrInfo->content_desc[i].user);
					break;
				case json_type_int :
					eitStrInfo->content_desc[i].user = json_object_get_int(val);
					break;
				default :
					Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String or integer expected\n",key);
					return -1;
			}
		}
		
	}
  }
  return 0;

}


/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_ca_identifier_descriptor(json_object * jarray,struct EitInfoSection * eitStrInfo)
{
  enum json_type type;
  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
  int i;
  json_object * jvalue;

  for (i=0; i< arraylen && i < CA_SYSTEM_ID_SIZE; i++) 
  {
    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
    type = json_object_get_type(jvalue);
	switch(type) {
		case json_type_string :
			sscanf(json_object_get_string(jvalue),"%x",&eitStrInfo->ca_identifier_desc.CASystemId[i]);
			break;
		case json_type_int :
			eitStrInfo->ca_identifier_desc.CASystemId[i] = json_object_get_int(jvalue);
			break;
		default :
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (ca_identifier_descriptor) bad type, String or integer expected\n");
			return -1;
	}
  }
  return 0;

}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_short_event_descriptor_items(json_object * jarray,struct EitInfoSection * eitStrInfo)
{
  enum json_type type;
  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
  int i;
  json_object * jvalue;

  for (i=0; i< arraylen && i < EXTEVENTDESC_ITEM_SIZE; i++) 
  {
    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
    type = json_object_get_type(jvalue);
    if (type != json_type_object) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (short_event_descriptor,items[%d]) bad type, Object expected\n",i);
		return -1;
    }

	json_object_object_foreach(jvalue, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"text") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}	
			strncpy(eitStrInfo->ext_event_desc.items[i].text,json_object_get_string(val),TEXTANDDESC_TEXT_LENGTH);
		}
		else if ( strcmp(key,"desc") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->ext_event_desc.items[i].desc,json_object_get_string(val),TEXTANDDESC_DESC_LENGTH);
		}
	}
  }
  return 0;

}
/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_short_event_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo)
{
	enum json_type type;
	if ( jobj == NULL ) {
		return -1;
	}
  
	json_object_object_foreach(jobj, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"event_name") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}	
			strncpy(eitStrInfo->short_event_desc.event_name,json_object_get_string(val),SHORTEVENTDESC_NAME_LENGTH);
		}
		else if ( strcmp(key,"event_text") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->short_event_desc.event_text,json_object_get_string(val),SHORTEVENTDESC_TEXT_LENGTH);
		}
		else if ( strcmp(key,"event_lang") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->short_event_desc.event_lang,json_object_get_string(val),SHORTEVENTDESC_LANG_LENGTH);
		}
	}
	return 0;
}


/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_extended_event_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo)
{
	enum json_type type;
	if ( jobj == NULL ) {
		return -1;
	}
  
	json_object_object_foreach(jobj, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"text") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}	
			strncpy(eitStrInfo->ext_event_desc.text,json_object_get_string(val),EXTEVENTDESC_TEXT_LENGTH);
		}
		else if ( strcmp(key,"lang") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			strncpy(eitStrInfo->ext_event_desc.lang,json_object_get_string(val),EXTEVENTDESC_LANG_LENGTH);
		}
		else if ( strcmp(key,"items") == 0 ) {
			if ( type != json_type_array ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, array expected\n",key);
				return -1;
			}
			if ( json_parse_short_event_descriptor_items(val,eitStrInfo) != 0 ) {
				return -1;
			}
		}
	}
	return 0;
}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
static int json_parse_parental_rating_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo)
{
	enum json_type type;
	if ( jobj == NULL ) {
		return -1;
	}
  
	json_object_object_foreach(jobj, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"country_code") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}	
			strncpy(eitStrInfo->parent_rating_desc.country_code,json_object_get_string(val),PARENTALRATINGDESC_COUNTRY_CODE_LENGTH);
		}
		else if ( strcmp(key,"age") == 0 ) {
			if ( type != json_type_int ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Integer expected\n",key);
				return -1;
			}
			eitStrInfo->parent_rating_desc.age = json_object_get_int(val);
		}
	}
	return 0;
}

static int isNumber(char * str)
{
	int size = strlen(str);
	int i;
	for(i=0;i<size;i++) {
		if ( !isdigit(str[i]) ) {
			return 0;
		}
	}
	return 1;
}

static int json_parse_short_smoothing_buffer_descriptor(json_object * jobj,struct EitInfoSection * eitStrInfo)
{
	enum json_type type;
	if ( jobj == NULL ) {
		return -1;
	}
  
	json_object_object_foreach(jobj, key, val) { 
		type = json_object_get_type(val);

		if ( strcmp(key,"sb_leak_rate") == 0 ) {
			if ( type != json_type_string ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected\n",key);
				return -1;
			}
			
			if ( isNumber((char*)json_object_get_string(val)) == 0 ) {
				Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, String expected but it must an integer\n",key);
				return -1;
			}
			
			eitStrInfo->short_smoothing_Buffer_desc.sbLeakRate = atoi(json_object_get_string(val));
		}
	}
	return 0;
}

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
struct EitInfoSection* json_parse(char * str) {
	enum json_type type;
	struct EitInfoSection * eitStrInfo = (struct EitInfoSection *) malloc(sizeof(struct EitInfoSection));

	if ( eitStrInfo == NULL ) {
		Logs(LOG_ERROR,__FILE__,__LINE__,"not enough memory - allocation EitInfoSection failed");
		return NULL;
	}
  
	memset(eitStrInfo,0,sizeof(struct EitInfoSection));
  
	json_object * jobj = json_tokener_parse(str);     
	if ( jobj == NULL ) {
		free(eitStrInfo);
		Logs(LOG_ERROR,__FILE__,__LINE__,"Parse JSON String failed");
		return NULL;
	}
	type = json_object_get_type(jobj);
	if ( type != json_type_object ) {
		free(eitStrInfo);
		Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (main) bad type, Object expected\n");
		return NULL;
	}
  
	json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
	json_object * child;
	type = json_object_get_type(val);
	json_object_object_get_ex(jobj, key,&child);

	if ( strcmp(key,"SHORT_EVENT_DESCRIPTOR") == 0 ) {
		if ( type != json_type_object ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Object expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_short_event_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}
	}
	else if ( strcmp(key,"EXTENDED_EVENT_DESCRIPTOR") == 0 ) {
		if ( type != json_type_object ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Object expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_extended_event_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}
	}
	else if ( strcmp(key,"COMPONENT_DESCRIPTOR") == 0 ) {
		if ( type != json_type_array ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, array expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_component_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}

	}
	else if ( strcmp(key,"PARENTAL_RATING_DESCRIPTOR") == 0 ) {
		if ( type != json_type_object ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Object expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_parental_rating_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}
	}
	else if ( strcmp(key,"CONTENT_DESCRIPTOR") == 0 ) {
		if ( type != json_type_array ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, array expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_content_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}	
	}
	else if ( strcmp(key,"CA_IDENTIFIER_DESCRIPTOR") == 0 ) {
		if ( type != json_type_array ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, array expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		if ( json_parse_ca_identifier_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}	
	}
	else if ( strcmp(key,"SHORT_SMOOTHING_BUFFER_DESCRIPTOR") == 0 ) {
		if ( type != json_type_object ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Object expected\n",key);
			free(eitStrInfo);
			return NULL;
		}	
		if ( json_parse_short_smoothing_buffer_descriptor(child,eitStrInfo) != 0 ) {
			free(eitStrInfo);
			return NULL;
		}	
	}

	else if ( strcmp(key,"starttime") == 0 ) {
		if ( type != json_type_object ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, Object expected\n",key);
			free(eitStrInfo);
			return NULL;
		}	
		eitStrInfo->starttime = json_object_get_int64(val);
	}
	else if ( strcmp(key,"duration") == 0 ) {
		if ( type != json_type_int ) {
			Logs(LOG_ERROR,__FILE__,__LINE__,"[JSON Parsing Error] - (Key,%s) bad type, integer expected\n",key);
			free(eitStrInfo);
			return NULL;
		}
		eitStrInfo->duration = json_object_get_int(val);		
	}

	
    /*switch (type) {
      case json_type_boolean: 
      case json_type_double: 
      case json_type_int: 
      case json_type_string: print_json_value(val);
                           break; 
      case json_type_object: printf("json_type_object\n");
							json_object_object_get_ex(jobj, key,&child);
                           json_parse(child); 
                           break;
      case json_type_array: printf("type: json_type_array, ");
                          json_parse_array(jobj, key);
                          break;
		default : break;
    }*/
  }
	return eitStrInfo;
} 

