/*****************************************************************************
 * eit_xml.h: 
 *****************************************************************************
 * $Id: eit_xml.h 
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

#ifndef EIT_XML_H_INCLUDED
#define EIT_XML_H_INCLUDED

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <json.h>

#define COMPONENT_DESC_SIZE  3

typedef struct shortEventXml_t {
    char *event_name;   
    char *event_lang;   
    char *event_text;
} shortEventXml_t;

typedef struct parentalRatingXml_t {
    int rating;   
    char *country_code;
} parentalRatingXml_t;

typedef struct componentDescXml_t {
    char *stream_content;
    char *component_type;
    int component_tag;
    char *language;
    char *text;
} componentDescXml_t;

typedef struct contentDescXml_t {
    int content_l1;   
    int content_l2;
} contentDescXml_t;

typedef struct eitInfoSectionXml_t {
    int event_id;
    long starttime;
    int duration;
    shortEventXml_t *short_event_xml;
    parentalRatingXml_t *parental_rating_xml;
    contentDescXml_t *content_desc_xml;
    componentDescXml_t *component_desc_xml[COMPONENT_DESC_SIZE];
} eitInfoSectionXml_t;

typedef struct eitInfoXml_t {
    eitInfoSectionXml_t *section0;
    eitInfoSectionXml_t *section1;
} eitInfoXml_t;

typedef struct eitXml_t {
    size_t size;
    char *content;
    int programNumber; 
} eitXml_t;

typedef struct xmlConfig_t {
    xmlXPathContextPtr ctxt;
} xmlConfig_t;


/*****************************************************************************
 * Prototypes
 *****************************************************************************/

json_object *convert_eit_struct_to_json(eitInfoSectionXml_t *section);
int convert_xml_to_eit_struct(eitXml_t *eitXml, eitInfoXml_t *eitInfoXml);
int extract_eti_xml_from_ts(eitXml_t *eitXml, const char *tsFilePath);
void free_eit_info_xml(eitInfoXml_t *eitInfoXml);
void free_eit_xml(eitXml_t *eitXml);
eitInfoXml_t *init_eit_info_xml();
eitXml_t *init_eit_xml();

#endif // EIT_XML_H_INCLUDED
