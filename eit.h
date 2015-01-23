/*****************************************************************************
 * eit.h: Utils for the multicat program
 *****************************************************************************
 * Copyright (C) 2009, 2011, 2014 VideoLAN
 * $Id: util.h 59 2014-08-09 14:09:10Z massiot $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef EIT_H_INCLUDED
#define EIT_H_INCLUDED

#define TEXTANDDESC_TEXT_LENGTH			32
#define TEXTANDDESC_DESC_LENGTH			128

#define SHORTEVENTDESC_NAME_LENGTH		64
#define SHORTEVENTDESC_TEXT_LENGTH		128
#define SHORTEVENTDESC_LANG_LENGTH		3

#define EXTEVENTDESC_TEXT_LENGTH		128
#define EXTEVENTDESC_LANG_LENGTH		3
#define EXTEVENTDESC_ITEM_SIZE			3

#define COMPONENTDESC_SIZE				3
#define COMPONENTDESC_TEXT_LENGTH		64
#define COMPONENTDESC_LANG_LENGTH		3

#define CA_SYSTEM_ID_SIZE				10
#define CONTENT_DESC_SIZE				10

//#define STARTTIME_LENGTH				5

#define PARENTALRATINGDESC_COUNTRY_CODE_LENGTH		3

struct TextAndDesc
{
    char text[TEXTANDDESC_TEXT_LENGTH+1];
    char desc[TEXTANDDESC_DESC_LENGTH+1];
};

struct ShortEventDesc
{
    char event_name[SHORTEVENTDESC_NAME_LENGTH+1];
    char event_text[SHORTEVENTDESC_TEXT_LENGTH+1];
    char event_lang[SHORTEVENTDESC_LANG_LENGTH+1];
};

struct ExtEventDesc
{
    char text[EXTEVENTDESC_TEXT_LENGTH+1];
    char lang[EXTEVENTDESC_LANG_LENGTH+1];
    struct TextAndDesc items[EXTEVENTDESC_ITEM_SIZE];
};


struct ComponentDesc
{
	uint8_t stream_content;
	uint8_t component_type;
	uint8_t set_component_tag;
    char text[COMPONENTDESC_TEXT_LENGTH+1];
    char lang[COMPONENTDESC_LANG_LENGTH+1];
};

struct ParentalRatingDesc
{
    char country_code[PARENTALRATINGDESC_COUNTRY_CODE_LENGTH+1];
	uint8_t age;
};

struct ContentDesc 
{
	int level_1;
	int level_2;
	int user;
};

struct CAIdentifierDesc
{
	int CASystemId[CA_SYSTEM_ID_SIZE];
};

struct ShortSmoothingBufferDesc
{
	int sbLeakRate;
};


struct EitInfoSection {
	//char startTime[STARTTIME_LENGTH+1];
	long starttime;
	int duration;
    struct ShortEventDesc short_event_desc;
    struct ExtEventDesc ext_event_desc;
    struct ComponentDesc component_desc[COMPONENTDESC_SIZE];
    struct ParentalRatingDesc parent_rating_desc;
	struct ContentDesc content_desc[CONTENT_DESC_SIZE];
	struct CAIdentifierDesc ca_identifier_desc;
	struct ShortSmoothingBufferDesc short_smoothing_Buffer_desc;
};

struct EitInfo
{
	short initialized;
	short updated;
	uint16_t tsid;
	uint16_t sid;
	uint16_t onid;
	struct EitInfoSection section0;
	struct EitInfoSection section1;
};


/*****************************************************************************
 * Prototypes
 *****************************************************************************/

int add_eit(struct EitInfo * eit_struct,int providerNumber,uint8_t *p_write_buffer,size_t i_payload_size,uint8_t ** output,size_t *ouput_length);

#endif // EIT_H_INCLUDED
