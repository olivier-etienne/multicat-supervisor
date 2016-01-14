/*****************************************************************************
 * json_parsing.h: 
 *****************************************************************************
 * $Id: json_parsing.h 
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

#ifndef JSON_PARSING_H_INCLUDED
#define JSON_PARSING_H_INCLUDED

#include <json.h>

#include "eit.h"

/**
 *  @brief 
 *  @param[in] 
 *  @return 
 *
 * Parsing the json object 
 */
json_object *convert_eit_struct_to_json(struct EitInfoSection section);
struct EitInfoSection* json_parse(json_object * jobj);

#endif // JSON_PARSING_H_INCLUDED
