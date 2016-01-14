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

#include "eit.h"

typedef struct eitXml_t {
    size_t size;
    char *content;
} eitXml_t;

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

int extract_eit_xml_to_eit_struct(struct EitInfo * eit_info, const char *tsFilePath);

#endif // EIT_XML_H_INCLUDED
