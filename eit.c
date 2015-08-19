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

#include <bitstream/mpeg/ts.h>
#include <bitstream/mpeg/psi.h>
#include <bitstream/dvb/si.h>
#include <bitstream/dvb/si_print.h>

#include "eit.h"
#include "sharedMemoryLib.h"
#include "logs.h"

uint16_t tsid     = 1;
uint16_t sid      = 0; // set Hexa
uint16_t event_id = 30000;
uint16_t onid     = 167;

uint16_t pmt_pid  = 100;

time_t ts_0 = 1;
int duration_0 = 0;
uint8_t  cc = 0;

FILE * pFileOut;

static int section = 0;
static struct EitInfo * str_eit_info;

/* =========================================================================
 * DVB defined descriptors
 * ========================================================================= */

 static struct EitInfoSection * getInfoSection() {
	struct EitInfoSection * infoSection = &str_eit_info->section0;
	if ( section == 1 )
		infoSection = &str_eit_info->section1;

	return infoSection;
}


/* DVB  Descriptor 0x4d: Short event descriptor*/
static void build_desc4d(uint8_t *desc) {

    //char *event_name, char *text,char * lang
    struct EitInfoSection * infoSection = getInfoSection();
	desc4d_init(desc);
    desc4d_set_lang(desc, (uint8_t *)infoSection->short_event_desc.event_lang);
    desc4d_set_event_name(desc, (uint8_t *)infoSection->short_event_desc.event_name, strlen(infoSection->short_event_desc.event_name));
    desc4d_set_text(desc, (uint8_t *)infoSection->short_event_desc.event_text, strlen(infoSection->short_event_desc.event_text));
    desc4d_set_length(desc);
}



/* DVB  Descriptor 0x4e: Extended event descriptor */
static void build_desc4e(uint8_t *desc) {

	struct EitInfoSection * infoSection = getInfoSection();
    uint8_t k = 0;
    uint8_t *item_n;

    desc4e_init(desc);
    desc_set_length(desc, 255);

    desc4e_set_desc_number(desc, 0);
    desc4e_set_last_desc_number(desc, 0);
    desc4e_set_lang(desc, (uint8_t *)infoSection->ext_event_desc.lang);

    /*
    desc4e_set_items_length(desc, 0);

    desc4e_set_items_length(desc, 255);
    uint8_t *first_item = desc4e_get_item(desc, 0);

	do {

	    item_n = desc4e_get_item(desc, k);
        desc4en_set_item_description(item_n, (uint8_t *)infoSection->ext_event_desc.items[k].desc, strlen(infoSection->ext_event_desc.items[k].desc));
        desc4en_set_item_text(item_n, (uint8_t *)infoSection->ext_event_desc.items[k].text, strlen(infoSection->ext_event_desc.items[k].text));
		k++;
	} while( k < EXTEVENTDESC_ITEM_SIZE );

    item_n = desc4e_get_item(desc, k);
    desc4e_set_items_length(desc, item_n - first_item);*/

    desc4e_set_text(desc, (uint8_t *)infoSection->ext_event_desc.text, strlen(infoSection->ext_event_desc.text));
    desc4e_set_length(desc);
}

/* DVB  Descriptor 0x50: Component descriptor */
static void build_desc50_stereo(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();

	//char *text = "Stereo";
    desc50_init(desc);
    desc50_set_stream_content(desc, infoSection->component_desc[0].stream_content);
    desc50_set_component_type(desc, infoSection->component_desc[0].component_type);
    desc50_set_component_tag(desc, infoSection->component_desc[0].set_component_tag);
    desc50_set_language(desc, (uint8_t *)infoSection->component_desc[0].lang);
    desc50_set_text(desc, (uint8_t *)infoSection->component_desc[0].text, strlen(infoSection->component_desc[0].text)); // Not required
}

/* DVB  Descriptor 0x50: Component descriptor */
static void build_desc50_audio(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();

    //char *text = "audio description";
    desc50_init(desc);
    desc50_set_stream_content(desc, infoSection->component_desc[1].stream_content);
    desc50_set_component_type(desc, infoSection->component_desc[1].component_type);
    desc50_set_component_tag(desc, infoSection->component_desc[1].set_component_tag);
    desc50_set_language(desc, (uint8_t *)infoSection->component_desc[1].lang);
    desc50_set_text(desc, (uint8_t *)infoSection->component_desc[1].text, strlen(infoSection->component_desc[1].text)); // Not required
}

/* DVB  Descriptor 0x50: Component descriptor */
static void build_desc50_subtitle(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();

    //char *text = "dvb subtitles (for the hard of hearing)";

    desc50_init(desc);
    desc50_set_stream_content(desc, infoSection->component_desc[2].stream_content);
    desc50_set_component_type(desc, infoSection->component_desc[2].component_type);
    desc50_set_component_tag(desc, infoSection->component_desc[2].set_component_tag);
    desc50_set_language(desc, (uint8_t *)infoSection->component_desc[2].lang);
    desc50_set_text(desc, (uint8_t *)infoSection->component_desc[2].text, strlen(infoSection->component_desc[2].text)); // Not required
}

/* DVB  Descriptor 0xAA || 0xAB: private descriptor */
static void build_descAA_AB(uint8_t *desc) {
// TODO
}
/* DVB  Descriptor 0x53: CA_identifier_descriptor */
static void build_desc53(uint8_t *desc) {

	struct EitInfoSection * infoSection = getInfoSection();
	int i;
    uint8_t k = 0;
    uint8_t *cas_n;

    desc53_init(desc);
    desc_set_length(desc, 255);

	for(i=0;i<CA_SYSTEM_ID_SIZE;i++) {
		if ( infoSection->ca_identifier_desc.CASystemId[i] == 0 )
			break;

		cas_n = desc53_get_ca(desc, k++);
		desc53n_set_ca_sysid(cas_n, infoSection->ca_identifier_desc.CASystemId[i]);
	}

/*    cas_n = desc53_get_ca(desc, k++);
    desc53n_set_ca_sysid(cas_n, 0xaabb);

    cas_n = desc53_get_ca(desc, k++);
    desc53n_set_ca_sysid(cas_n, 0xccdd);

    cas_n = desc53_get_ca(desc, k++);
    desc53n_set_ca_sysid(cas_n, 0xeeff);*/

    cas_n = desc53_get_ca(desc, k);
    desc_set_length(desc, cas_n - desc - DESC_HEADER_SIZE);
}

/* DVB  Descriptor 0x54: Content descriptor */
static void build_desc54(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();
	int i;
    uint8_t k = 0;
    uint8_t *content_n;

    desc54_init(desc);
    desc_set_length(desc, 255);

	for(i=0;i<CONTENT_DESC_SIZE;i++) {
		if ( infoSection->content_desc[i].level_1 == 0 &&
				infoSection->content_desc[i].level_2 == 0 &&
					infoSection->content_desc[i].user == 0 ) {
					break;
		}

		content_n = desc54_get_content(desc, k++);
		desc54n_set_content_l1(content_n,infoSection->content_desc[i].level_1);
		desc54n_set_content_l2(content_n, infoSection->content_desc[i].level_2);
		desc54n_set_user(content_n, infoSection->content_desc[i].user);
	}

/*    content_n = desc54_get_content(desc, k++);
    desc54n_set_content_l1(content_n, 1);
    desc54n_set_content_l2(content_n, 2);
    desc54n_set_user(content_n, 78);

    content_n = desc54_get_content(desc, k++);
    desc54n_set_content_l1(content_n, 1);
    desc54n_set_content_l2(content_n, 3);
    desc54n_set_user(content_n, 177);*/



    content_n = desc54_get_content(desc, k);
    desc_set_length(desc, content_n - desc - DESC_HEADER_SIZE);
}

/* DVB  Descriptor 0x55: Parental rating descriptor */
static void build_desc55(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();

    uint8_t k = 0;
    uint8_t *rating_n;

    desc55_init(desc);
    desc_set_length(desc, 255);

    rating_n = desc55_get_rating(desc, k++);
    desc55n_set_country_code(rating_n, (uint8_t *)infoSection->parent_rating_desc.country_code);
    desc55n_set_rating(rating_n,infoSection->parent_rating_desc.age);

    rating_n = desc55_get_rating(desc, k);
    desc_set_length(desc, rating_n - desc - DESC_HEADER_SIZE);
}

/* DVB  Descriptor 0x61: Short smoothing buffer descriptor */
static void build_desc61(uint8_t *desc) {
	struct EitInfoSection * infoSection = getInfoSection();

    desc61_init(desc);
    desc61_set_sb_size(desc, 1); // 1536 bytes buffer size
	desc61_set_sb_leak_rate(desc, infoSection->short_smoothing_Buffer_desc.sbLeakRate); // 0.5 Mbit/s
}

/* DVB  Descriptor 0x69: PDC_descriptor */
static void build_desc69(uint8_t *desc) {
	//struct EitInfoSection * infoSection = getInfoSection();

    desc69_init(desc);
//    desc69_set_pil(desc, 0x5d805);
    desc69_set_day(desc, 27);
    desc69_set_month(desc, 02);
    desc69_set_hour(desc, 18);
    desc69_set_minute(desc, 20);
}

/* ---  Descriptor 0x7d: XAIT location descriptor */
/* ---  Descriptor 0x7e: FTA_content_management_descriptor */
/* ---  Descriptor 0x7f: extension descriptor */

static void output_psi_section(uint8_t * packetEIT, uint16_t pid, uint8_t *cc, uint8_t ** output_eit,size_t* ouput_length)
{
    uint16_t section_length = psi_get_length(packetEIT) + PSI_HEADER_SIZE;
    uint16_t section_offset = 0;
	uint8_t * ptr = NULL;

	int nbPacket = section_length / TS_SIZE;
	if ( (section_length % TS_SIZE) != 0 )
		nbPacket++;

	(*ouput_length) = nbPacket * TS_SIZE;
	(*output_eit) = (uint8_t *) malloc(sizeof(uint8_t)* (*ouput_length) );
	ptr = (*output_eit);

    //Logs(LOG_DEBUG,__FILE__,__LINE__,"EIT output packet size = %d",(*ouput_length));

    do {
        uint8_t ts[TS_SIZE];
        uint8_t ts_offset = 0;
        memset(ts, 0xff, TS_SIZE);

        psi_split_section(ts, &ts_offset, packetEIT, &section_offset);

        ts_set_pid(ts, pid);
        ts_set_cc(ts, *cc);
        (*cc)++;
        *cc &= 0xf;

        if (section_offset == section_length)
            psi_split_end(ts, &ts_offset);

		memcpy(ptr,ts,TS_SIZE);
		ptr += TS_SIZE;

        //fwrite(ts, sizeof(uint8_t), TS_SIZE, pFileOut);
    } while (section_offset < section_length);
}

static void dump_eit_info(struct EitInfoSection * info_section) {
	int i;

	printf("--------------------------------------------------------------------------------\n");
	printf("------------------- SECTION %d----------------\n",section);
	printf("SHORT_EVENT_DESCRIPTOR - event_name : %s\n",info_section->short_event_desc.event_name);
	printf("SHORT_EVENT_DESCRIPTOR - event_text : %s\n",info_section->short_event_desc.event_text);
	printf("SHORT_EVENT_DESCRIPTOR - event_lang : %s\n",info_section->short_event_desc.event_lang);

	printf("EXTENDED_EVENT_DESCRIPTOR - text : %s\n",info_section->ext_event_desc.text);
	printf("EXTENDED_EVENT_DESCRIPTOR - lang : %s\n",info_section->ext_event_desc.lang);

	for(i=0;i<EXTEVENTDESC_ITEM_SIZE;i++) {
		printf("EXTENDED_EVENT_DESCRIPTOR - %d text : %s\n",i+1,info_section->ext_event_desc.items[i].text);
		printf("EXTENDED_EVENT_DESCRIPTOR - %d desc : %s\n",i+1,info_section->ext_event_desc.items[i].desc);
	}

	for (i=0;i<3;i++) {
		printf("COMPONENT_DESCRIPTOR [%d] - stream_content : %0x\n",i,info_section->component_desc[i].stream_content);
		printf("COMPONENT_DESCRIPTOR [%d] - component_type : %0x\n",i,info_section->component_desc[i].component_type);
		printf("COMPONENT_DESCRIPTOR [%d] - set_component_tag : %d\n",i,info_section->component_desc[i].set_component_tag);
		printf("COMPONENT_DESCRIPTOR [%d] - text : %s\n",i,info_section->component_desc[i].text);
		printf("COMPONENT_DESCRIPTOR [%d] - lang : %s\n",i,info_section->component_desc[i].lang);
	}

	printf("PARENTAL_RATING_DESCRIPTOR - country_code : %s\n",info_section->parent_rating_desc.country_code);
	printf("PARENTAL_RATING_DESCRIPTOR - age : %d\n",info_section->parent_rating_desc.age);
	printf("--------------------------------------------------------------------------------\n");

}


/* DVB  Event Information Table (EIT) */
static void generate_eit(uint8_t ** output_eit,size_t* ouput_length)
{
	struct EitInfoSection * infoSection = getInfoSection();
    uint8_t *eit = psi_private_allocate();
    uint8_t *desc_loop, *desc;
    uint8_t desc_counter;
	unsigned int duration;
	time_t starttime;

	//Logs(LOG_DEBUG,__FILE__,__LINE__,"section %d",section);


	if ( section == 0 ) {
		if ( infoSection->starttime != 0 ) {
			starttime = infoSection->starttime;
		} else {
			time(&starttime);
		}

		if ( infoSection->duration != 0 ) {
			duration = infoSection->duration*60;
		}
		else {
			duration = 3600*12;
		}
		ts_0 = starttime;
		duration_0 = duration;

	} else {
		if ( infoSection->duration != 0 ) {
			duration = infoSection->duration;
		}
		else {
			duration = 3600*12;
		}
		// la date du debut du prochain evenement c'est le courant + ca duree
		starttime = ts_0 + duration_0;
	}

    // Generate EIT
    eit_init(eit, true);
    psi_set_version(eit, 0);
    psi_set_section(eit, section);
    //psi_set_current(eit);
    eit_set_sid(eit, str_eit_info->sid);
    eit_set_tsid(eit, str_eit_info->tsid);
    eit_set_onid(eit, str_eit_info->onid);
    //eit_set_last_section_number(eit, 1);
    eit_set_segment_last_sec_number(eit, 1);
    eit_set_last_table_id(eit, 55);

	//Logs(LOG_DEBUG,__FILE__,__LINE__,"sid %d",str_eit_info->sid);
	//Logs(LOG_DEBUG,__FILE__,__LINE__,"tsid %d",str_eit_info->tsid);
	//Logs(LOG_DEBUG,__FILE__,__LINE__,"onid %d",str_eit_info->onid);


    // Process transport stream loop
    eit_set_length(eit, PSI_MAX_SIZE); // This needed so eit_get_ts works
    {
        uint8_t *eit_n;
        uint8_t eit_n_counter = 0;

        eit_n = eit_get_event(eit, eit_n_counter++);
	if (section == 0) {
		eitn_set_event_id(eit_n, event_id);
		eitn_set_running(eit_n, 4);
	    eitn_set_ca(eit_n, false);
	} else {
		eitn_set_event_id(eit_n, event_id+1);
		eitn_set_running(eit_n, 1);
        eitn_set_ca(eit_n, false);
	}

	eitn_set_start_time(eit_n, dvb_time_encode_UTC(starttime));
	eitn_set_duration_bcd(eit_n, dvb_time_encode_duration(duration));

	eitn_set_desclength(eit_n, 0);
        {
            // Add descriptors to transport_stream_n
            desc_counter = 0;
            desc_loop = eitn_get_descs(eit_n);
            descs_set_length(desc_loop, DESCS_MAX_SIZE); // This is needed so descs_get_desc(x, n) works


             /// Short Event descriptor
            desc = descs_get_desc(desc_loop, desc_counter++);
            build_desc4d(desc);


		if (section == 0) {

		  /// Extended event descriptor >> a tester
            desc = descs_get_desc(desc_loop, desc_counter++);
			build_desc4e(desc);

            /// Short smoothing buffer descriptor
            desc = descs_get_desc(desc_loop, desc_counter++);
			build_desc61(desc);


		     ///Private descriptor (ajoute par sfau) >> a tester
		     desc = descs_get_desc(desc_loop, desc_counter++);
		     build_descAA_AB(desc);

		           /// Component descriptor (premier)
            desc = descs_get_desc(desc_loop, desc_counter++);
            build_desc50_stereo(desc);


            /// Component descriptor (deuxieme)
			desc = descs_get_desc(desc_loop, desc_counter++);
			build_desc50_audio(desc);

             /// Component descriptor (troisième)
			desc = descs_get_desc(desc_loop, desc_counter++);
			build_desc50_subtitle(desc);

            ///Content descriptor (categories)
            desc = descs_get_desc(desc_loop, desc_counter++);
            build_desc54(desc);

            ///CA identifier descriptor
            desc = descs_get_desc(desc_loop, desc_counter++);
			build_desc53(desc);

	    }

            ///Parental rating descriptor
            desc = descs_get_desc(desc_loop, desc_counter++);
            build_desc55(desc);

            // Finish descriptor generation
            desc = descs_get_desc(desc_loop, desc_counter); // Get next descriptor pos
            descs_set_length(desc_loop, desc - desc_loop - DESCS_HEADER_SIZE);
        }

	// Set transport_stream_loop length
        eit_n = eit_get_event(eit, eit_n_counter); // Get last service
        eit_set_length(eit, eit_n - eit_get_event(eit, 0));
    }
    psi_set_crc(eit);
    output_psi_section(eit, EIT_PID, &cc,output_eit,ouput_length);

    free(eit);
}

int add_eit(struct EitInfo * eit_struct,int providerNumber,uint8_t *p_write_buffer,size_t i_payload_size,uint8_t ** output,size_t* ouput_length)
{
	uint8_t * ptr = NULL;
	uint8_t * output_eit = NULL;
	uint8_t * packetTab[20];
	size_t ouput_eit_length=0;
	int packetNumber = i_payload_size / TS_SIZE;
	int i,j,tabIndex = 0;
    // fix datetime
	int packetEIT = 0;
	str_eit_info = eit_struct;

	if ( sharedMemory_get_updated(providerNumber) == 1 ) {
		Logs(LOG_INFO,__FILE__,__LINE__,"Multicat Provider %d eit has updated",providerNumber);
		sharedMemory_get(providerNumber,str_eit_info);
		//event_id += 2;
	}

    for(i=0;i<packetNumber;i++)
    {
		ptr = p_write_buffer + ( i * TS_SIZE);
        if (ts_validate(ptr)) {

            uint16_t i_pid = ts_get_pid(ptr);

            if (i_pid == 0x12)
            {
				packetEIT = 1;
				break;
			}
		}
	}

	if ( packetEIT == 0 ) {
		return 0;
	}

	memset(packetTab,0,sizeof(uint8_t*) * 20);

    for(i=0;i<packetNumber;i++)
    {
		ptr = p_write_buffer + ( i * TS_SIZE);
        if (ts_validate(ptr)) {
            uint16_t i_pid = ts_get_pid(ptr);


            if (i_pid == 0x12)
            {
                generate_eit(&output_eit,&ouput_eit_length);
				for(j=0;j<(ouput_eit_length/TS_SIZE);j++) {
					packetTab[tabIndex] = output_eit + (j*TS_SIZE)  ;
					tabIndex++;
				}
				if (section == 0) {
					section = 1;
				} else {
					section = 0;
				}
			} else {
				packetTab[tabIndex] = ptr;
				tabIndex++;
			}
        }
    }

	(*ouput_length) = sizeof(uint8_t) * tabIndex * TS_SIZE ;
	(*output) = (uint8_t *) malloc(sizeof(uint8_t) * tabIndex * TS_SIZE );
	ptr = (*output);
	for(j=0;j<tabIndex;j++) {
		memcpy(ptr,packetTab[j],sizeof(uint8_t) * TS_SIZE);
		ptr += TS_SIZE;
	}

	if ( output_eit != NULL ) free(output_eit);
    return 1;
}
