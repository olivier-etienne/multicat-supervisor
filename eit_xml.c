#include <string.h>
#include <stdbool.h>

#include "logs.h"
#include "eit_xml.h"
#include "util.h"

/*****************************************************************************
 * Prototypage
 *****************************************************************************/

static void free_component_desc_xml(componentDescXml_t *component_desc_xml);
static void free_info_section_xml(eitInfoSectionXml_t *section);
static void free_parental_rating_xml(parentalRatingXml_t *parental_rating_xml);
static void free_short_event_xml(shortEventXml_t *short_event_xml);
static void free_xml_conf(xmlConfig_t *xmlConf);
static int init_component_desc_xml(componentDescXml_t *component_desc_xml);
static int init_info_section_xml(eitInfoSectionXml_t *section);
static int init_parental_rating_xml(parentalRatingXml_t *parental_rating_xml);
static int init_short_event_xml(shortEventXml_t *short_event_xml);
static xmlConfig_t *init_xml_conf(char * xmlContent, size_t xmlSize);
static void build_component_desc_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static void build_content_desc_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static int build_eit_section_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static void build_parental_rating_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static int build_program_number(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoXml_t *eitInfoXml);
static int build_sections_event_id(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoXml_t *eitInfoXml);
static void build_short_event_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static int build_starttime_duration_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section);
static json_object *convert_content_desc_xml_to_json(contentDescXml_t *content_desc_xml);
static json_object *convert_component_desc_xml_to_json(eitInfoSectionXml_t *section);
static json_object *convert_duration_to_json(int duration);
static json_object *convert_parental_rating_xml_to_json(parentalRatingXml_t *parental_rating_xml);
static json_object *convert_short_desc_xml_to_json(shortEventXml_t *short_event_xml);
/*static json_object *convert_starttime_to_json(long starttime);*/


static void build_component_desc_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    int i;
    int nbComponent;

    for (i = 0; i < COMPONENT_DESC_SIZE; i++) {
        section->component_desc_xml[i]->stream_content = NULL;
        section->component_desc_xml[i]->component_type = NULL;
        section->component_desc_xml[i]->component_tag = -1;
        section->component_desc_xml[i]->language = NULL;
        section->component_desc_xml[i]->text = NULL;
    }

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/COMPONENT_DESC", eitXml->programNumber, section->event_id);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        nbComponent = (xpathRes->nodesetval->nodeNr < COMPONENT_DESC_SIZE) ? xpathRes->nodesetval->nodeNr : COMPONENT_DESC_SIZE;
        for (i = 0; i < nbComponent; i++) {
            xmlNodePtr node = xpathRes->nodesetval->nodeTab[i];
            xmlAttr* attribute = node->properties;
            while(attribute && attribute->name && attribute->children) {
                if (xmlStrEqual(attribute->name, (const xmlChar *)"stream_content"))
                    section->component_desc_xml[i]->stream_content = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"component_type"))
                    section->component_desc_xml[i]->component_type = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"component_tag"))
                    section->component_desc_xml[i]->component_tag = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
                if (xmlStrEqual(attribute->name, (const xmlChar *)"language"))
                    section->component_desc_xml[i]->language = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"text"))
                    section->component_desc_xml[i]->text = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                attribute = attribute->next;
            }
            xmlFree(attribute);
            xmlFree(node);
        }
    }
    xmlXPathFreeObject(xpathRes);
}

static void build_content_desc_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;

    section->content_desc_xml->content_l1 = -1;
    section->content_desc_xml->content_l2 = -1;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/CONTENT_DESC", eitXml->programNumber, section->event_id);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            if (xmlStrEqual(attribute->name, (const xmlChar *)"content_l1"))
                section->content_desc_xml->content_l1 = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
            if (xmlStrEqual(attribute->name, (const xmlChar *)"content_l2"))
                section->content_desc_xml->content_l2 = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
            attribute = attribute->next;
        }
        xmlFree(attribute);
        xmlFree(node);
    }
    xmlXPathFreeObject(xpathRes);
}

static int build_eit_section_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    build_short_event_xml(eitXml, xmlConf, section);
    build_parental_rating_xml(eitXml, xmlConf, section);
    build_content_desc_xml(eitXml, xmlConf, section);
    build_component_desc_xml(eitXml, xmlConf, section);
    if (0 != (build_starttime_duration_xml(eitXml, xmlConf, section)))
        return -1;

    return 0;
}

static void build_parental_rating_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;

    section->parental_rating_xml->rating = -1;
    section->parental_rating_xml->country_code = NULL;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/PARENTAL_RATING_DESC", eitXml->programNumber, section->event_id);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            if (xmlStrEqual(attribute->name, (const xmlChar *)"rating"))
                section->parental_rating_xml->rating = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
            if (xmlStrEqual(attribute->name, (const xmlChar *)"country_code"))
                section->parental_rating_xml->country_code = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
            attribute = attribute->next;
        }
        xmlFree(attribute);
        xmlFree(node);
    }
    xmlXPathFreeObject(xpathRes);
}

static int build_program_number(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoXml_t *eitInfoXml) {
    xmlXPathObjectPtr xpathRes;

    eitXml->programNumber = -1;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search for program number
    xpathRes = xmlXPathEvalExpression(BAD_CAST "/TS/PAT/PROGRAM", xmlConf->ctxt);
    if (NULL != xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"number"))
                eitXml->programNumber = atoi((char *)value);
            attribute = attribute->next;
            xmlFree(value);
        }
        xmlFree(attribute);
        xmlFree(node);
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program number not found");
        return -1;
    }
    xmlXPathFreeObject(xpathRes);

    if (-1 == eitXml->programNumber) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program number found but invalid");
        return -1;
    }

    return 0;
}

static int build_sections_event_id(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoXml_t *eitInfoXml) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;

    eitInfoXml->section0->event_id = -1;
    eitInfoXml->section1->event_id = -1;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search section 0 event id
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]", eitXml->programNumber);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        int i = 0;
        for (i=0; i < xpathRes->nodesetval->nodeNr; i++) {
            xmlNodePtr childrenNode = xpathRes->nodesetval->nodeTab[i]->children;
            xmlAttr* attribute = childrenNode->properties;
            while(attribute && attribute->name && attribute->children) {
                xmlChar* value = xmlNodeListGetString(childrenNode->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"id")) {
                    if (eitInfoXml->section0->event_id == -1 || atoi((char *)value) < eitInfoXml->section0->event_id)
                        eitInfoXml->section0->event_id = atoi((char *)value);
                    if (eitInfoXml->section1->event_id == -1 || atoi((char *)value) > eitInfoXml->section1->event_id)
                        eitInfoXml->section1->event_id = atoi((char *)value);
                }
                attribute = attribute->next;
                xmlFree(value);
            }
            xmlFree(attribute);
            xmlFree(childrenNode);
        }
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program event id not found");
        return -1;
    }
    xmlXPathFreeObject(xpathRes);

    if (-1 == eitInfoXml->section0->event_id || -1 == eitInfoXml->section1->event_id) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program event id found but invalid");
        return -1;
    }

    return 0;
}

static void build_short_event_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;

    section->short_event_xml->event_lang = NULL;
    section->short_event_xml->event_name = NULL;
    section->short_event_xml->event_text = NULL;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/SHORT_EVENT_DESC", eitXml->programNumber, section->event_id);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            if (xmlStrEqual(attribute->name, (const xmlChar *)"lang"))
                section->short_event_xml->event_lang = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"event_name")) {
                section->short_event_xml->event_name = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                utf8_to_latin9(section->short_event_xml->event_name);
            }
            if (xmlStrEqual(attribute->name, (const xmlChar *)"text")) {
                section->short_event_xml->event_text = (char *)xmlNodeListGetString(node->doc, attribute->children, 1);
                utf8_to_latin9(section->short_event_xml->event_text);
            }
            attribute = attribute->next;
        }
        xmlFree(attribute);
        xmlFree(node);
    }
    xmlXPathFreeObject(xpathRes);
}

static int build_starttime_duration_xml(eitXml_t *eitXml, xmlConfig_t *xmlConf, eitInfoSectionXml_t *section) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;

    section->starttime = -1;
    section->duration = -1;

    // init context before new XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(eitXml->content, eitXml->size));

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]", eitXml->programNumber, section->event_id);
    xpathRes = xmlXPathEvalExpression(BAD_CAST xPathQuery, xmlConf->ctxt);
    if (NULL != xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            if (xmlStrEqual(attribute->name, (const xmlChar *)"start_time"))
                section->starttime = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
            if (xmlStrEqual(attribute->name, (const xmlChar *)"duration"))
                section->duration = atoi((char *)xmlNodeListGetString(node->doc, attribute->children, 1));
            attribute = attribute->next;
        }
        xmlFree(attribute);
        xmlFree(node);
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program start_time and duration not found");
        return -1;
    }
    xmlXPathFreeObject(xpathRes);

    if (-1 == section->starttime || -1 == section->duration) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program start_time and duration found but invalid");
        return -1;
    }

    return 0;
}

static json_object *convert_content_desc_xml_to_json(contentDescXml_t *content_desc_xml) {
    // create json content desc array
    json_object *jContentDescArray = json_object_new_array();

    if (-1 != content_desc_xml->content_l1 && 
        -1 != content_desc_xml->content_l2) {
        // create content desc json object
        json_object * jContentDesc = json_object_new_object();

        // create sub item of content desc json object
        json_object *jContentDescContentL1 = json_object_new_int(content_desc_xml->content_l1);
        json_object *jContentDescContentL2 = json_object_new_int(content_desc_xml->content_l2);

        // add sub item inside objet    
        json_object_object_add(jContentDesc,"content_nibble_level_1", jContentDescContentL1);
        json_object_object_add(jContentDesc,"content_nibble_level_2", jContentDescContentL2);

        // add objet into array
        json_object_array_add(jContentDescArray, jContentDesc);
    }

    return jContentDescArray;
}

static json_object *convert_component_desc_xml_to_json(eitInfoSectionXml_t *section) {
    int i;

    // create json component desc array
    json_object *jComponentDescArray = json_object_new_array();

    for (i = 0; i < COMPONENT_DESC_SIZE; i++) {
        if (NULL != section->component_desc_xml[i]->stream_content) {
            // create component desc json object
            json_object * jComponentDesc = json_object_new_object();

            // create sub item of component desc json object
            json_object *jComponentDescStreamContent = json_object_new_string(section->component_desc_xml[i]->stream_content);
            json_object *jComponentDescComponentType = json_object_new_string(section->component_desc_xml[i]->component_type);
            json_object *jComponentDescComponentTag = json_object_new_int(section->component_desc_xml[i]->component_tag);
            json_object *jComponentDescLanguage = json_object_new_string(section->component_desc_xml[i]->language);
            json_object *jComponentDescText = json_object_new_string(section->component_desc_xml[i]->text);

            // add sub item inside objet    
            json_object_object_add(jComponentDesc,"stream_content", jComponentDescStreamContent);
            json_object_object_add(jComponentDesc,"component_type", jComponentDescComponentType);
            json_object_object_add(jComponentDesc,"set_component_tag", jComponentDescComponentTag);
            json_object_object_add(jComponentDesc,"lang", jComponentDescLanguage);
            json_object_object_add(jComponentDesc,"text", jComponentDescText);

            json_object_array_add(jComponentDescArray, jComponentDesc);
        }
    }

    return jComponentDescArray;
}

static json_object *convert_duration_to_json(int duration) {
    // create duration json object
    json_object *jDuration = json_object_new_int(duration);

    return jDuration;
}

static json_object *convert_parental_rating_xml_to_json(parentalRatingXml_t *parental_rating_xml) {
    // create parental rating desc json object
    json_object *jParentalRatingDesc = json_object_new_object();

    if (-1 != parental_rating_xml->rating && 
        NULL != parental_rating_xml->country_code) {

        // create sub item of parental rating desc json object
        json_object *jParentalRatingDescRating = json_object_new_int(parental_rating_xml->rating);
        json_object *jParentalRatingDescCountryCode = json_object_new_string(parental_rating_xml->country_code);

        // add sub item inside objet
        json_object_object_add(jParentalRatingDesc,"age", jParentalRatingDescRating);
        json_object_object_add(jParentalRatingDesc,"country_code", jParentalRatingDescCountryCode);
    }

    return jParentalRatingDesc;
}

static json_object *convert_short_desc_xml_to_json(shortEventXml_t *short_event_xml) {
    // create short desc json object
    json_object *jShortDesc = json_object_new_object();
    
    if (NULL != short_event_xml->event_lang &&
        NULL != short_event_xml->event_name &&
        NULL != short_event_xml->event_text) {

        // create sub item of short desc json object
        json_object *jShortDescEventLang = json_object_new_string(short_event_xml->event_lang);
        json_object *jShortDescEventName = json_object_new_string(short_event_xml->event_name);
        json_object *jShortDescEventText = json_object_new_string(short_event_xml->event_text);

        // add sub item inside objet
        json_object_object_add(jShortDesc,"event_lang", jShortDescEventLang);
        json_object_object_add(jShortDesc,"event_name", jShortDescEventName);
        json_object_object_add(jShortDesc,"event_text", jShortDescEventText);
    }

    return jShortDesc;
}

/*static json_object *convert_starttime_to_json(long starttime) {
    // create starttime json object
    json_object *jStartTimeValue = json_object_new_int(starttime);

    return jStartTimeValue;
}*/

json_object *convert_eit_struct_to_json(eitInfoSectionXml_t *section) {
    //create main json objet
    json_object * jObj = json_object_new_object();

    // starttime  
    //json_object *jStartTimeValue = convert_starttime_to_json(section->starttime);
    //json_object_object_add(jObj, "starttime", jStartTimeValue);

    // duration
    json_object *jDurationValue = convert_duration_to_json(section->duration);
    json_object_object_add(jObj, "duration", jDurationValue);
    
    // short desc
    json_object *jShortDesc = convert_short_desc_xml_to_json(section->short_event_xml);
    json_object_object_add(jObj, "SHORT_EVENT_DESCRIPTOR", jShortDesc);
    
    // parental rating desc
    json_object * jParentalRatingDesc = convert_parental_rating_xml_to_json(section->parental_rating_xml);
    json_object_object_add(jObj, "PARENTAL_RATING_DESCRIPTOR", jParentalRatingDesc);
    
    // content desc
    json_object *jContentDescArray = convert_content_desc_xml_to_json(section->content_desc_xml);
    json_object_object_add(jObj, "CONTENT_DESCRIPTOR", jContentDescArray);
    
    // component desc
    json_object *jComponentDescArray = convert_component_desc_xml_to_json(section);
    json_object_object_add(jObj, "COMPONENT_DESCRIPTOR", jComponentDescArray);
    
    return jObj;
}

int convert_xml_to_eit_struct(eitXml_t *eitXml, eitInfoXml_t *eitInfoXml) {
    xmlConfig_t *xmlConf = NULL;

    // Init XML Conf
    if (NULL == (xmlConf = init_xml_conf(eitXml->content, eitXml->size)))
        return -3;

    // search program number
    if (0 != (build_program_number(eitXml, xmlConf, eitInfoXml)))
        return -3;
    if (0 != (build_sections_event_id(eitXml, xmlConf, eitInfoXml)))
        return -3;
    
    // build sections
    if (0 != (build_eit_section_xml(eitXml, xmlConf, eitInfoXml->section0)))
        return -3;
    if (0 != (build_eit_section_xml(eitXml, xmlConf, eitInfoXml->section1)))
        return -3;  
    
    free_xml_conf(xmlConf);

    return 0;
}

int extract_eti_xml_from_ts(eitXml_t *eitXml, const char *tsFilePath) {
    FILE *fd;
    char cmd[512];

    sprintf(cmd, "/usr/local/multicat-tools/bin/dvb_print_si -x xml < %s", tsFilePath);
    fd = popen(cmd, "r");
    if (!fd) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"dvb_print_si cmd execution failed");
        return -1;
    }

    char   buffer[256];
    size_t chread;
    size_t comalloc = 256;
    eitXml->size = 0;
    eitXml->content = malloc(comalloc);
    /* Use fread so binary data is dealt with correctly */
    while ((chread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
        if (eitXml->size + chread >= comalloc) {
            comalloc *= 2;
            eitXml->content = realloc(eitXml->content, comalloc);
        }
        memmove(eitXml->content + eitXml->size, buffer, chread);
        eitXml->size += chread;
    }
    pclose(fd);

    return 0;
}

static void free_component_desc_xml(componentDescXml_t *component_desc_xml) {
    if (NULL != component_desc_xml->stream_content) {
        free(component_desc_xml->stream_content);
    }
    if (NULL != component_desc_xml->component_type) {
        free(component_desc_xml->component_type);
    }
    if (NULL != component_desc_xml->language) {
        free(component_desc_xml->language);
    }
    if (NULL != component_desc_xml->text) {
        free(component_desc_xml->text);
    }
    free(component_desc_xml);
}

void free_eit_info_xml(eitInfoXml_t *eitInfoXml) {
    if (NULL != eitInfoXml->section0) {
        free_info_section_xml(eitInfoXml->section0);
    }
    if (NULL != eitInfoXml->section1) {
        free_info_section_xml(eitInfoXml->section1);
    }
    free(eitInfoXml);
}

void free_eit_xml(eitXml_t *eitXml) {
    if (NULL != eitXml->content) {
        free(eitXml->content);
    }
    free(eitXml);
}

static void free_info_section_xml(eitInfoSectionXml_t *section) {
    int i;

    if (NULL != section->short_event_xml) {
        free_short_event_xml(section->short_event_xml);
    }
    if (NULL != section->parental_rating_xml) {
        free_parental_rating_xml(section->parental_rating_xml);
    }    
    for (i = 0 ; i < COMPONENT_DESC_SIZE; i++) {
        if (NULL != section->component_desc_xml[i]) {
            free_component_desc_xml(section->component_desc_xml[i]);
        }
    }

    free(section);
}

static void free_parental_rating_xml(parentalRatingXml_t *parental_rating_xml) {
    if (NULL != parental_rating_xml->country_code) {
        free(parental_rating_xml->country_code);
    }
    free(parental_rating_xml);
}

static void free_short_event_xml(shortEventXml_t *short_event_xml) {
    if (NULL != short_event_xml->event_lang) {
        free(short_event_xml->event_lang);
    }
    if (NULL != short_event_xml->event_name) {
        free(short_event_xml->event_name);
    }
    if (NULL != short_event_xml->event_text) {
        free(short_event_xml->event_text);
    }
    free(short_event_xml);
}

static void free_xml_conf(xmlConfig_t *xmlConf) {
    if (NULL != xmlConf->ctxt) {
        xmlXPathFreeContext(xmlConf->ctxt);
    }
    free(xmlConf);
}

static int init_component_desc_xml(componentDescXml_t *component_desc_xml) {

    if (NULL == (component_desc_xml->stream_content = malloc(sizeof(*component_desc_xml->stream_content)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(component_desc_xml->stream_content, 0, sizeof(*component_desc_xml->stream_content));

    if (NULL == (component_desc_xml->component_type = malloc(sizeof(*component_desc_xml->component_type)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(component_desc_xml->component_type, 0, sizeof(*component_desc_xml->component_type));
    
    if (NULL == (component_desc_xml->language = malloc(sizeof(*component_desc_xml->language)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(component_desc_xml->language, 0, sizeof(*component_desc_xml->language));

    if (NULL == (component_desc_xml->text = malloc(sizeof(*component_desc_xml->text)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(component_desc_xml->text, 0, sizeof(*component_desc_xml->text));

    return 0;
}

eitInfoXml_t *init_eit_info_xml() {
    eitInfoXml_t *eitInfoXml = NULL;

    if (NULL == (eitInfoXml = malloc(sizeof(*eitInfoXml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(eitInfoXml, 0, sizeof(*eitInfoXml));

    if (NULL == (eitInfoXml->section0 = malloc(sizeof(*eitInfoXml->section0)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(eitInfoXml->section0, 0, sizeof(*eitInfoXml->section0));
    
    if (0 != (init_info_section_xml(eitInfoXml->section0)))
        return NULL;

    if (NULL == (eitInfoXml->section1 = malloc(sizeof(*eitInfoXml->section1)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(eitInfoXml->section1, 0, sizeof(*eitInfoXml->section1));

    if (0 != (init_info_section_xml(eitInfoXml->section1)))
        return NULL;

    return eitInfoXml;
}

eitXml_t *init_eit_xml() {
    eitXml_t *eitXml = NULL;

    if (NULL == (eitXml = malloc(sizeof(*eitXml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(eitXml, 0, sizeof(*eitXml));

    return eitXml;
}

static int init_info_section_xml(eitInfoSectionXml_t *section) {
    int i;

    if (NULL == (section->short_event_xml = malloc(sizeof(*section->short_event_xml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(section->short_event_xml, 0, sizeof(*section->short_event_xml));

    if (0 != init_short_event_xml(section->short_event_xml))
        return -1;

    if (NULL == (section->parental_rating_xml = malloc(sizeof(*section->parental_rating_xml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(section->parental_rating_xml, 0, sizeof(*section->parental_rating_xml));

    if (0 != init_parental_rating_xml(section->parental_rating_xml))
        return -1;

    if (NULL == (section->content_desc_xml = malloc(sizeof(*section->content_desc_xml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(section->content_desc_xml, 0, sizeof(*section->content_desc_xml));

    for (i = 0 ; i < COMPONENT_DESC_SIZE; i++) {
        if (NULL == (section->component_desc_xml[i] = malloc(sizeof(*section->component_desc_xml[i])))) {
            Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
            return -1;
        }
        memset(section->component_desc_xml[i], 0, sizeof(*section->component_desc_xml[i]));

        if (0 != init_component_desc_xml(section->component_desc_xml[i]))
            return -1;
    }

    return 0;
}

static int init_parental_rating_xml(parentalRatingXml_t *parental_rating_xml) {
    
    if (NULL == (parental_rating_xml->country_code = malloc(sizeof(*parental_rating_xml->country_code)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(parental_rating_xml->country_code, 0, sizeof(*parental_rating_xml->country_code));

    return 0;
}

static int init_short_event_xml(shortEventXml_t *short_event_xml) {
    
    if (NULL == (short_event_xml->event_lang = malloc(sizeof(*short_event_xml->event_lang)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(short_event_xml->event_lang, 0, sizeof(*short_event_xml->event_lang));

    if (NULL == (short_event_xml->event_name = malloc(sizeof(*short_event_xml->event_name)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(short_event_xml->event_name, 0, sizeof(*short_event_xml->event_name));

    if (NULL == (short_event_xml->event_text = malloc(sizeof(*short_event_xml->event_text)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return -1;
    }
    memset(short_event_xml->event_text, 0, sizeof(*short_event_xml->event_text));

    return 0;
}

static xmlConfig_t *init_xml_conf(char * xmlContent, size_t xmlSize) {
    xmlConfig_t *xmlConf = NULL;

    if (NULL == (xmlConf = malloc(sizeof(*xmlConf)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(xmlConf, 0, sizeof(*xmlConf));
    
    // Create DOM tree and init XPath env
    xmlKeepBlanksDefault(0);
    xmlXPathInit();

    // Create context for XPath query
    xmlConf->ctxt = xmlXPathNewContext(xmlParseMemory(xmlContent, xmlSize));
    if (NULL == xmlConf->ctxt) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"xml context creation failed");
        free_xml_conf(xmlConf);
        return NULL;
    }

    return xmlConf;
}
