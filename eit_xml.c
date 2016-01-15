#include <string.h>
#include <stdbool.h>

#include "logs.h"
#include "stdint.h"
#include "inttypes.h"
#include "eit_xml.h"
#include "util.h"

/*****************************************************************************
 * Prototypage
 *****************************************************************************/

static void build_component_desc_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static void build_content_desc_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static int build_eit_section_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static void build_parental_rating_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static int build_program_number(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info);
static int build_sections_event_id(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info);
static void build_short_event_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static int build_starttime_duration_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb);
static int convert_xml_to_eit_struct(eitXml_t *eitXml, struct EitInfo * eit_info);
static int extract_eti_xml_from_ts(eitXml_t *eitXml, const char *tsFilePath);
static void free_eit_xml(eitXml_t *eitXml);
static int get_section_event_id(struct EitInfo * eit_info, int sectionNb);
static eitXml_t *init_eit_xml();


xmlDocPtr get_xml_doc(eitXml_t *eitXml) {
    xmlDocPtr doc;
    doc = xmlParseMemory(eitXml->content, eitXml->size);
    
    if (doc == NULL ) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Document not parsed successfully");
        return NULL;
    }

    return doc;
}

xmlXPathObjectPtr get_xml_node_set(xmlDocPtr doc, xmlChar *xpath){
    
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Error in xmlXPathNewContext");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Error in xmlXPathEvalExpression");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
        xmlXPathFreeObject(result);
        Logs(LOG_ERROR,__FILE__,__LINE__,"No result");
        return NULL;
    }
    return result;
}


static void build_component_desc_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;
    struct ComponentDesc componentDesc[COMPONENTDESC_SIZE];
    int i;
    int nbComponent;
    int sectionEventId;    

    // init section component desc
    for (i = 0; i < COMPONENTDESC_SIZE; i++) { 
        componentDesc[i].stream_content = 0;            
        componentDesc[i].component_type = 0;            
        componentDesc[i].set_component_tag = 0;            
        memset(componentDesc[i].lang,'\0',COMPONENTDESC_LANG_LENGTH+1);
        memset(componentDesc[i].text,'\0',COMPONENTDESC_TEXT_LENGTH+1);      
        if (0 == sectionNb) {
            eit_info->section0.component_desc[i].stream_content = 0;            
            eit_info->section0.component_desc[i].component_type = 0;            
            eit_info->section0.component_desc[i].set_component_tag = 0;            
            memset(eit_info->section0.component_desc[i].lang,'\0',COMPONENTDESC_LANG_LENGTH+1);
            memset(eit_info->section0.component_desc[i].text,'\0',COMPONENTDESC_TEXT_LENGTH+1);
        }
        else {
            eit_info->section1.component_desc[i].stream_content = 0;            
            eit_info->section1.component_desc[i].component_type = 0;            
            eit_info->section1.component_desc[i].set_component_tag = 0;            
            memset(eit_info->section1.component_desc[i].lang,'\0',COMPONENTDESC_LANG_LENGTH+1);
            memset(eit_info->section1.component_desc[i].text,'\0',COMPONENTDESC_TEXT_LENGTH+1);
        }
    }

    // get corresponding section event id
    sectionEventId = get_section_event_id(eit_info, sectionNb);

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/COMPONENT_DESC", eit_info->programNumber, sectionEventId);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        
        nbComponent = (xpathRes->nodesetval->nodeNr < COMPONENTDESC_SIZE) ? xpathRes->nodesetval->nodeNr : COMPONENTDESC_SIZE;
        
        for (i = 0; i < nbComponent; i++) {
            xmlNodePtr node = xpathRes->nodesetval->nodeTab[i];
            xmlAttr* attribute = node->properties;
            
            while(attribute && attribute->name && attribute->children) {
                keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"stream_content"))
                    sscanf((char *)keyword,"%x",&componentDesc[i].stream_content);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"component_type"))
                    sscanf((char *)keyword,"%x",&componentDesc[i].component_type);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"component_tag"))
                    componentDesc[i].set_component_tag = atoi((char *)keyword);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"language"))
                    strncpy(componentDesc[i].lang, (char *)keyword, COMPONENTDESC_LANG_LENGTH);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"text"))
                    strncpy(componentDesc[i].text, (char *)keyword, COMPONENTDESC_TEXT_LENGTH);
                xmlFree(keyword);
                attribute = attribute->next;
            }

            if (0 == sectionNb)
                eit_info->section0.component_desc[i] = componentDesc[i];
            else
                eit_info->section1.component_desc[i] = componentDesc[i];
        }        
        xmlXPathFreeObject(xpathRes);
    }
}

static void build_content_desc_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;
    struct ContentDesc contentDesc[CONTENT_DESC_SIZE];
    int i;
    int nbComponent;
    int sectionEventId;

    // init section content desc
    for (i = 0; i < CONTENT_DESC_SIZE; i++) {
        contentDesc[i].level_1 = 0;
        contentDesc[i].level_2 = 0;
        contentDesc[i].user = 0;
        if (0 == sectionNb) {
            eit_info->section0.content_desc[i].level_1 = 0;
            eit_info->section0.content_desc[i].level_2 = 0;
            eit_info->section0.content_desc[i].user = 0;
        }
        else {
            eit_info->section1.content_desc[i].level_1 = 0;
            eit_info->section1.content_desc[i].level_2 = 0;
            eit_info->section1.content_desc[i].user = 0;
        }
    }   

    // get corresponding section event id
    sectionEventId = get_section_event_id(eit_info, sectionNb);

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/CONTENT_DESC", eit_info->programNumber, sectionEventId);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {

        nbComponent = (xpathRes->nodesetval->nodeNr < COMPONENTDESC_SIZE) ? xpathRes->nodesetval->nodeNr : COMPONENTDESC_SIZE;       

        for (i = 0; i < nbComponent; i++) {
            xmlNodePtr node = xpathRes->nodesetval->nodeTab[i];
            xmlAttr* attribute = node->properties;
            
            while(attribute && attribute->name && attribute->children) {
                keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"content_l1"))
                    contentDesc[i].level_1 = atoi((char *)keyword);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"content_l2"))
                    contentDesc[i].level_2 = atoi((char *)keyword);
                xmlFree(keyword);
                attribute = attribute->next;
            }

            if (0 == sectionNb)
                eit_info->section0.content_desc[i] = contentDesc[i];
            else
                eit_info->section1.content_desc[i] = contentDesc[i];

        }     
        xmlXPathFreeObject(xpathRes);
    }
}

static int build_eit_section_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {    
    if (0 != (build_starttime_duration_xml(eitXml, xmlDoc, eit_info, sectionNb)))
        return -1;
    build_short_event_xml(eitXml, xmlDoc, eit_info, sectionNb);
    build_parental_rating_xml(eitXml, xmlDoc, eit_info, sectionNb);
    build_content_desc_xml(eitXml, xmlDoc, eit_info, sectionNb);
    build_component_desc_xml(eitXml, xmlDoc, eit_info, sectionNb);

    return 0;
}

static void build_parental_rating_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;
    int sectionEventId;
    struct ParentalRatingDesc parentRatingDesc;

    parentRatingDesc.age = 0;
    memset(parentRatingDesc.country_code,'\0',PARENTALRATINGDESC_COUNTRY_CODE_LENGTH+1);
    if (0 == sectionNb) {
        eit_info->section0.parent_rating_desc.age = 0;
        memset(eit_info->section0.parent_rating_desc.country_code,'\0',PARENTALRATINGDESC_COUNTRY_CODE_LENGTH+1);
    }
    else {
        eit_info->section1.parent_rating_desc.age = 0;
        memset(eit_info->section1.parent_rating_desc.country_code,'\0',PARENTALRATINGDESC_COUNTRY_CODE_LENGTH+1);
    }

    // get corresponding section event id
    sectionEventId = get_section_event_id(eit_info, sectionNb);

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/PARENTAL_RATING_DESC", eit_info->programNumber, sectionEventId);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;;

        while(attribute && attribute->name && attribute->children) {
            keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"rating"))
                parentRatingDesc.age = atoi((char *)keyword);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"country_code"))
                strncpy(parentRatingDesc.country_code, (char *)keyword, PARENTALRATINGDESC_COUNTRY_CODE_LENGTH+1);
            xmlFree(keyword);
            attribute = attribute->next;
        }

        if (0 == sectionNb)
            eit_info->section0.parent_rating_desc = parentRatingDesc;
        else
            eit_info->section1.parent_rating_desc = parentRatingDesc;

        xmlXPathFreeObject(xpathRes);
    }
}

static int build_program_number(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info) {
    xmlChar *xpath = (xmlChar*) "/TS/PAT/PROGRAM";
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;

    // init program number
    eit_info->programNumber = 0;

    // search for program number
    xpathRes = get_xml_node_set(xmlDoc, xpath);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"number"))
                eit_info->programNumber = atoi((char *)keyword);
            xmlFree(keyword);
            attribute = attribute->next;
        }
        xmlXPathFreeObject(xpathRes);
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program number not found");
        return -1;
    }    

    if (0 == eit_info->programNumber) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program number found but invalid");
        return -1;
    }

    return 0;
}

static int build_sections_event_id(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info) {   
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;

    // init sections event id
    eit_info->section0.event_id = 0;
    eit_info->section1.event_id = 0;    

    // search section 0 event id
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT", eit_info->programNumber);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        int i = 0;
        for (i=0; i < xpathRes->nodesetval->nodeNr; i++) {
            xmlNodePtr node = xpathRes->nodesetval->nodeTab[i];
            xmlAttr* attribute = node->properties;
            while(attribute && attribute->name && attribute->children) {
                keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
                if (xmlStrEqual(attribute->name, (const xmlChar *)"id")) {
                    if (eit_info->section0.event_id == 0 || atoi((char *)keyword) < eit_info->section0.event_id)
                        eit_info->section0.event_id = atoi((char *)keyword);
                    if (eit_info->section1.event_id == 0 || (atoi((char *)keyword) <= (eit_info->section0.event_id + 1)))
                        eit_info->section1.event_id = atoi((char *)keyword);
                }
                xmlFree(keyword);
                attribute = attribute->next;
            }
        }
        xmlXPathFreeObject(xpathRes);
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program event id not found");
        return -1;
    }

    if (0 == eit_info->section0.event_id || 0 == eit_info->section1.event_id) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program event id found but invalid");
        return -1;
    }

    return 0;
}

static void build_short_event_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;
    int sectionEventId;
    struct ShortEventDesc shortEventDesc;

    memset(shortEventDesc.event_lang,'\0',SHORTEVENTDESC_LANG_LENGTH+1);
    memset(shortEventDesc.event_name,'\0',SHORTEVENTDESC_NAME_LENGTH+1);
    memset(shortEventDesc.event_text,'\0',SHORTEVENTDESC_TEXT_LENGTH+1);
    if (0 == sectionNb) {
        memset(eit_info->section0.short_event_desc.event_lang,'\0',SHORTEVENTDESC_LANG_LENGTH+1);
        memset(eit_info->section0.short_event_desc.event_name,'\0',SHORTEVENTDESC_NAME_LENGTH+1);
        memset(eit_info->section0.short_event_desc.event_text,'\0',SHORTEVENTDESC_TEXT_LENGTH+1);
    }
    else {
        memset(eit_info->section1.short_event_desc.event_lang,'\0',SHORTEVENTDESC_LANG_LENGTH+1);
        memset(eit_info->section1.short_event_desc.event_name,'\0',SHORTEVENTDESC_NAME_LENGTH+1);
        memset(eit_info->section1.short_event_desc.event_text,'\0',SHORTEVENTDESC_TEXT_LENGTH+1);
    }

    // get corresponding section event id
    sectionEventId = get_section_event_id(eit_info, sectionNb);

    // search short desc
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]/DESC/SHORT_EVENT_DESC", eit_info->programNumber, sectionEventId);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes  && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"lang"))
                strncpy(shortEventDesc.event_lang, (char *)keyword, SHORTEVENTDESC_LANG_LENGTH+1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"event_name")) {
                strncpy(shortEventDesc.event_name, (char *)keyword, SHORTEVENTDESC_NAME_LENGTH+1);
                utf8_to_latin9(shortEventDesc.event_name);
            }
            if (xmlStrEqual(attribute->name, (const xmlChar *)"text")) {
                strncpy(shortEventDesc.event_text, (char *)keyword, SHORTEVENTDESC_TEXT_LENGTH+1);
                utf8_to_latin9(shortEventDesc.event_text);
            }
            xmlFree(keyword);
            attribute = attribute->next;
        }

        if (0 == sectionNb)
            eit_info->section0.short_event_desc = shortEventDesc;
        else
            eit_info->section1.short_event_desc = shortEventDesc;

        xmlXPathFreeObject(xpathRes);
    }
}

static int build_starttime_duration_xml(eitXml_t *eitXml, xmlDocPtr xmlDoc, struct EitInfo * eit_info, int sectionNb) {
    char xPathQuery[512];
    xmlXPathObjectPtr xpathRes;
    xmlChar *keyword;
    int sectionEventId;
    long starttime;
    int duration;

    // get corresponding section event id
    sectionEventId = get_section_event_id(eit_info, sectionNb);

    starttime = 0;
    duration = 0;
    if (0 == sectionNb) {
        eit_info->section0.starttime = 0;
        eit_info->section0.duration = 0;
    }
    else {
        eit_info->section1.starttime = 0;
        eit_info->section1.duration = 0;
    }

    // search lang, event_name and text
    sprintf(xPathQuery,"/TS/EIT[@service_id=\"%d\"]/EVENT[@id=\"%d\"]", eit_info->programNumber, sectionEventId);
    xpathRes = get_xml_node_set(xmlDoc, (xmlChar*) xPathQuery);
    if (xpathRes && xpathRes->type == XPATH_NODESET && xpathRes->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathRes->nodesetval->nodeTab[0];
        xmlAttr* attribute = node->properties;
        while(attribute && attribute->name && attribute->children) {
            keyword = xmlNodeListGetString(node->doc, attribute->children, 1);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"start_time"))
                starttime = atoi((char *)keyword);
            if (xmlStrEqual(attribute->name, (const xmlChar *)"duration"))
                duration = atoi((char *)keyword);
            xmlFree(keyword);
            attribute = attribute->next;
        }
        xmlXPathFreeObject(xpathRes);
    }
    else {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program start_time and duration not found");
        return -1;
    }

    if (0 == starttime || 0 == duration) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"Program start_time and duration found but invalid");
        return -1;
    }

    if (0 == sectionNb) {
        eit_info->section0.starttime = starttime;
        eit_info->section0.duration = duration;
    }
    else {
        eit_info->section1.starttime = starttime;
        eit_info->section1.duration = duration;
    }

    return 0;
}

int convert_xml_to_eit_struct(eitXml_t *eitXml, struct EitInfo * eit_info) {

    // Create DOM tree and init XPath env    
    xmlXPathInit();

    xmlDocPtr xmlDoc;
    xmlDoc = get_xml_doc(eitXml);

    // search program number
    if (0 != (build_program_number(eitXml, xmlDoc, eit_info)))
        return -3;

    // search event id of section0 and section1
    if (0 != (build_sections_event_id(eitXml, xmlDoc, eit_info)))
        return -3;
    
    // build section 0
    if (0 != (build_eit_section_xml(eitXml, xmlDoc, eit_info, 0)))
        return -3;

    // build section 1
    if (0 != (build_eit_section_xml(eitXml, xmlDoc, eit_info, 1)))
        return -3;

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return 0;
}

static int extract_eti_xml_from_ts(eitXml_t *eitXml, const char *tsFilePath) {
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

int extract_eit_xml_to_eit_struct(struct EitInfo * eit_info, const char *tsFilePath) {
    eitXml_t *eitXml = NULL; 

    // Init EIT XML
    if (NULL == (eitXml = init_eit_xml()))
        return -3;

    // load eit xml from ts file    
    if (0 != extract_eti_xml_from_ts(eitXml, tsFilePath))
        return -3;

    // build eit from xml
    if (0 != (convert_xml_to_eit_struct(eitXml, eit_info)))
        return -3;

    free_eit_xml(eitXml);

    return 0;
}

static void free_eit_xml(eitXml_t *eitXml) {
    if (NULL != eitXml->content) {
        free(eitXml->content);
    }
    free(eitXml);
}

static eitXml_t *init_eit_xml() {
    eitXml_t *eitXml = NULL;

    if (NULL == (eitXml = malloc(sizeof(*eitXml)))) {
        Logs(LOG_ERROR,__FILE__,__LINE__,"out of memory");
        return NULL;
    }
    memset(eitXml, 0, sizeof(*eitXml));

    return eitXml;
}

static int get_section_event_id(struct EitInfo * eit_info, int sectionNb) {
    return (0 == sectionNb) ? eit_info->section0.event_id : eit_info->section1.event_id;
}
