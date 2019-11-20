//
//  main.cpp
//  TsParser
//
//  Created by yajun18 on 2019/11/4.
//  Copyright © 2019 yajun. All rights reserved.
//

#include <iostream>

int pes_parse(unsigned char* p, int rpos, int npos, FILE* fd);
const char* hexdump(const unsigned char *buf, const int num);

int main(int argc, const char * argv[]) {
    // insert code here...
    
    FILE* tsf = fopen("/Users/yajun18/Movies/test.ts", "rb");
    
    fseek(tsf,0L,SEEK_END); /* 定位到文件末尾 */
    long flen=ftell(tsf); /* 得到文件大小 */
　　 unsigned char* buf=(unsigned char *)malloc(flen+1); /* 根据文件大小动态分配内存空间 */
　　 fseek(tsf,0L,SEEK_SET); /* 定位到文件开头 */

    while(1){
        size_t nread = fread(buf, 1, flen, tsf);
        printf("read buf size %ld\n", nread);
        if( nread <= 0 ){
            break;
        }
        int program_map_PID = 0, network_PID = 0;
        struct{
            int pid;
            FILE* fd;
        } pes_info[10];
        memset(pes_info, 0, sizeof(pes_info));
        int pes_index = 0;
//        int pes_PID = 0;
        int cnt = nread;
        while( cnt >= 188 ){
            unsigned char* p = buf+(nread-cnt);
            int npos = 0;
            uint8_t syncByte = *p;
            assert(syncByte==0x47);
            p++;npos++;
            uint8_t transport_error_indicator = (*p&0x80)>>7;
            uint8_t payload_unit_start_indicator = (*p&0x40)>>6;
            uint8_t transport_priority = (*p&0x20)>>5;
            uint32_t PID = ((p[0]<<8)|p[1])&0x1FFF;
            p += 2;npos += 2;
            uint8_t transport_scrambling_control = (*p&0xC0)>>6;
            uint8_t adaptation_field_control = (*p&0x30)>>4;
            uint8_t continuity_counter = (*p&0x0F);
            p++;npos++;
            if( adaptation_field_control == 2 || adaptation_field_control == 3 ){
                // adaptation_field()
                uint8_t adaptation_field_length = *p; // following field the length
                int rpos = 1;
                if( adaptation_field_length > 0 ){
                    int discontinuity_indicator = (p[rpos]&0x80)>>7;
                    int random_access_indicator = (p[rpos]&0x40)>>6;
                    int elementary_stream_priority_indicator = (p[rpos]&0x20)>>5;
                    int PCR_flag = (p[rpos]&0x10)>>4;
                    int OPCR_flag = (p[rpos]&0x08)>>3;
                    int splicing_point_flag = (p[rpos]&0x04)>>2;
                    int transport_private_data_flag = (p[rpos]&0x02)>>1;
                    int adaptation_field_extension_flag = (p[rpos]&0x01);
                    rpos++;
                    if( PCR_flag == 1 ) { // PCR info
                        //program_clock_reference_base 33 uimsbf
                        //reserved 6 bslbf
                        //program_clock_reference_extension 9 uimsbf
                        printf("parse adaptation_field() adaptation_field_length=%d  PCR_flag=%d  rpos=%d\n", adaptation_field_length, PCR_flag, rpos);
                        rpos += 6;
                    }
                    if( OPCR_flag == 1 ) {
                        //original_program_clock_reference_base 33 uimsbf
                        //reserved 6 bslbf
                        //original_program_clock_reference_extension 9 uimsbf
                        printf("parse adaptation_field() adaptation_field_length=%d  OPCR_flag=%d rpos=%d\n", adaptation_field_length, OPCR_flag, rpos);
                        rpos += 6;
                    }
                    if( splicing_point_flag == 1 ) {
                        //splice_countdown 8 tcimsbf
                        rpos++;
                    }
                    if( transport_private_data_flag == 1 ) {
                        //transport_private_data_length 8 uimsbf
                        int transport_private_data_length = p[rpos];
                        printf("parse adaptation_field() adaptation_field_length=%d  transport_private_data_length=%d rpos=%d\n", adaptation_field_length, transport_private_data_length, rpos);
                        rpos++;
                        for (int i = 0; i < transport_private_data_length; i++) {
                            //private_data_byte 8 bslbf
                            rpos++;
                        }
                    }
                    if( adaptation_field_extension_flag == 1 ) {
                        //adaptation_field_extension_length 8 uimsbf
                        rpos++;
                        //ltw_flag 1 bslbf
                        int ltw_flag = (p[rpos]&0x80)>>7;
                        //piecewise_rate_flag 1 bslbf
                        int piecewise_rate_flag = (p[rpos]&0x40)>>6;
                        //seamless_splice_flag 1 bslbf
                        int seamless_splice_flag = (p[rpos]&0x20)>>5;
                        //reserved 5 bslbf
                        printf("parse adaptation_field() adaptation_field_length=%d ltw_flag=%d piecewise_rate_flag=%d seamless_splice_flag=%d rpos=%d\n", adaptation_field_length, ltw_flag, piecewise_rate_flag, seamless_splice_flag, rpos);
                        rpos++;
                        if (ltw_flag == 1) {
                            //ltw_valid_flag 1 bslbf
                            //ltw_offset 15 uimsbf
                            rpos += 2;
                        }
                        if (piecewise_rate_flag == 1) {
                            //reserved 2 bslbf
                            //piecewise_rate 22 uimsbf
                            rpos += 3;
                        }
                        if (seamless_splice_flag == 1) {
                            //splice_type 4 bslbf
                            //DTS_next_AU[32..30] 3 bslbf
                            //marker_bit 1 bslbf
                            //DTS_next_AU[29..15] 15 bslbf
                            //marker_bit 1 bslbf
                            //DTS_next_AU[14..0] 15 bslbf
                            //marker_bit 1 bslbf
                            rpos += 5;
                        }
//                        for (int i = 0; i < N; i++) {
                            //reserved 8 bslbf
//                        }
                        printf("parse adaptation_field() adaptation_field_length=%d adaptation_field_extension_flag=1 rpos=%d\n", adaptation_field_length, rpos);
                    }
//                    for (int i = 0; i < N; i++) {
                        //stuffing_byte 8 bslbf
//                    }
                    printf("parse adaptation_field() adaptation_field_length=%d  rpos=%d\n", adaptation_field_length, rpos);
                }
                
                
                npos += adaptation_field_length + 1;
                printf("parse adaptation_field() adaptation_field_length=%d npos=%d rpos=%d adaptation_field_control=%d remain=%d id=%d\n", adaptation_field_length, npos, rpos, adaptation_field_control, adaptation_field_length-rpos, p[rpos]);
                p += adaptation_field_length + 1;
            }
            if( adaptation_field_control == 1 || adaptation_field_control == 3 ){
                // data_byte with placeholder
                printf("parse adaptation_field() adaptation_field_control=%d npos=%d remain=%d PID=%X id=%d\n", adaptation_field_control, npos, 188-npos, PID, *p);
                // payload parser
                if(PID == 0x00){
                    // PAT // program association table
                    int rpos = 0;
                    if(payload_unit_start_indicator)
                        rpos++;
                    int table_id = p[rpos];
                    rpos++;
                    int section_syntax_indicator = (p[rpos]>>7)&0x01;
                    // skip 3 bits of 1 zero and 2 reserved
                    int section_length = ((p[rpos]<<8)|p[rpos+1])&0x0FFF;
                    rpos += 2;
                    int transport_stream_id = (p[rpos]<<8)|p[rpos+1];
                    rpos += 2;
                    // reserved 2 bits
                    int version_number = (p[rpos]&0x3E)>>1;
                    int current_next_indicator = p[rpos]&0x01;
                    rpos++;
                    int section_number = p[rpos];
                    rpos++;
                    int last_section_number = p[rpos];
                    assert( table_id == 0x00 ); // PAT_TID_PAS = 0x00, // program_association_section
//                    assert(1 == section_syntax_indicator);
//                    if(previous_ver != version_number){
//                        pmt_count = 0;
//                    }
                    assert(188-npos>section_length+3); // PAT = section_length + 3
                    rpos++;
                    for (;rpos+4 <= section_length-5-4+9;) { // 4:CRC, 5:follow section_length item  rpos + 4(following unit length) section_length + 9(above field and unit_start_first_byte )
                        //program_number 16 uimsbf
                        int program_number = p[rpos]<<8|p[rpos+1];
                        rpos += 2;
//                        reserved 3 bslbf
                        
                        if (program_number == 0) {
//                            network_PID 13 uimsbf
                            network_PID = (p[rpos]<<8|p[rpos+1])&0x1FFF;
                            rpos += 2;
                        }
                        else {
//                            program_map_PID 13 uimsbf
                            program_map_PID = (p[rpos]<<8|p[rpos+1])&0x1FFF;
                            rpos += 2;
                        }
                        printf("program_number = %d network_PID = %X program_map_PID = %X rpos=%d\n", program_number, network_PID, program_map_PID, rpos);
                        // network_PID and program_map_PID save to list
                    }
//                    CRC_32 use pat to calc crc32, eq
                    rpos += 4;
                }else if(PID == 0x01){
                    // CAT // conditional access table
                }else if(PID == 0x02){
                    //TSDT  // transport stream description table
                }else if(PID == 0x03){
                    //IPMP // IPMP control information table
                    // 0x0004-0x000F Reserved
                    // 0x0010-0x1FFE May be assigned as network_PID, Program_map_PID, elementary_PID, or for other purposes
                }else if(PID == 0x11){
                    // SDT // https://en.wikipedia.org/wiki/Service_Description_Table / https://en.wikipedia.org/wiki/MPEG_transport_stream
                    printf("SDT table \n");
                }else if(PID == program_map_PID){
                    printf("PMT = %X \n", program_map_PID);
                    int rpos = 0;
                    if(payload_unit_start_indicator)
                        rpos++;
                    int table_id = p[rpos];
                    rpos++;
                    int section_syntax_indicator = (p[rpos]>>7)&0x01;
                    // skip 3 bits of 1 zero and 2 reserved
                    int section_length = ((p[rpos]<<8)|p[rpos+1])&0x0FFF;
                    rpos += 2;
                    int program_number = (p[rpos]<<8)|p[rpos+1];
                    rpos += 2;
                    // reserved 2 bits
                    int version_number = (p[rpos]&0x3E)>>1;
                    int current_next_indicator = p[rpos]&0x01;
                    rpos++;
                    int section_number = p[rpos];
                    rpos++;
                    int last_section_number = p[rpos];
                    rpos++;
                    // skip 3 bits for reserved 3 bslbf
                    int PCR_PID = ((p[rpos]<<8)|p[rpos+1])&0x1FFF; //PCR_PID 13 uimsbf
                    rpos += 2;
                    printf("PMT tid=%d section_length=%d program_number=%d version_number=%d section_number=%d PCR_PID=%X \n", table_id, section_length, program_number, version_number, section_number, PCR_PID);
                    //reserved 4 bslbf
                    int program_info_length = ((p[rpos]<<8)|p[rpos+1])&0x0FFF;//program_info_length 12 uimsbf
                    rpos += 2;
                    assert(table_id==0x02); //  0x02, // TS_program_map_section
                    
                    printf("PMT descriptor program_info_length = %d \n", program_info_length);
//                    for (i = 0; i < N; i++) {
//                        descriptor()
//                    }
                    rpos += program_info_length;
                
                    for (; rpos + 5 <= section_length + 4 - 4; ) { // rpos(above field length) i+5(following unit length) section_length +3(PMT begin three bytes)+1(payload_unit_start_indicator) -4(crc32)
                        int stream_type = p[rpos];//stream_type 8 uimsbf  0x1B AVC video stream as defined in ITU-T Rec. H.264 | ISO/IEC 14496-10 Video
                        rpos++;
                        //reserved 3 bslbf
                        int elementary_PID = ((p[rpos]<<8)|p[rpos+1])&0x1FFF; //elementary_PID 13 uimsbf
                        rpos += 2;
                        //reserved 4 bslbf
                        int ES_info_length = ((p[rpos]<<8)|p[rpos+1])&0x0FFF; //ES_info_length 12 uimsbf
                        rpos += 2;
                        printf("PMT stream_type=%X elementary_PID=%X ES_info_length=%d\n", stream_type, elementary_PID, ES_info_length);
                        if( rpos + ES_info_length > section_length + 4 - 4 )
                            break;
                        int absES_info_length = rpos + ES_info_length;
                        for (; rpos<absES_info_length; ) {
                            //descriptor()
                            int descriptor_tag = p[rpos];
                            rpos++;
                            int descriptor_length = p[rpos];
                            rpos++;
                            rpos += descriptor_length;
                            printf("descriptor tag = %d len=%d\n", descriptor_tag, descriptor_length);
                        }
                        // save program_number(stream num) elementary_PID(PES PID) stream_type(stream codec)
                        bool isFound = false;
                        for (int i=0; i<pes_index; i++) {
                            if(elementary_PID == pes_info[i].pid){
                                isFound = true;
                                break;
                            }
                        }
                        if( !isFound ){
                            pes_info[pes_index].pid = elementary_PID;
                            char fname[128] = {0};
                            sprintf(fname, "stream%XPID=%X", stream_type, elementary_PID);
                            pes_info[pes_index].fd = fopen(fname, "wb");
                            pes_index++;
                        }
                    }
                    rpos += 4;//CRC_32
                }else if(PID == 0x0042){
                    // USER
                }else if(PID == 0x1FFF){
                    // Null packet
                }else{
                    bool isFound = false;
                    for (int i=0; i<pes_index; i++) {
                        if(PID == pes_info[i].pid){
                            isFound = true;
                            printf("PES = %X \n", pes_info[i].pid);
                            int rpos = 0;
                            if(payload_unit_start_indicator){
                                rpos = pes_parse(p, rpos, npos, pes_info[i].fd);
                            }else{
                                // rpos = pes_parse(p, rpos);
                                printf("PES payload_unit_start_indicator is zero; rpos=%d npos=%d remain=%d hex=%s\n", rpos, npos, 188-(npos+rpos), hexdump(p, 8));
                                fwrite(p, 1, 188-(npos+rpos), pes_info[i].fd);
                            }
                        }
                    }
                    if(!isFound){
                        printf("unknown PID = %X \n", PID);
                    }
                }
            }
            cnt -= 188;
        }
        for (int i=0; i<pes_index; i++) {
            if( pes_info[i].fd != nullptr ){
                fclose(pes_info[i].fd);
                printf("close file %p Pid=%X\n", pes_info[i].fd, pes_info[i].pid);
            }
        }
    }
    
    return 0;
}

int pes_parse(unsigned char* p, int rpos, int npos, FILE* fd){
    int packet_start_code_prefix = (p[rpos]<<16)|(p[rpos+1]<<8)|p[rpos+2];  //packet_start_code_prefix 24 bslbf
    assert(0x00000001 == packet_start_code_prefix);
    rpos += 3;
    int stream_id = p[rpos]; //stream_id 8 uimsbf
    rpos++;
    int PES_packet_length = (p[rpos]<<8)|p[rpos+1]; //PES_packet_length 16 uimsbf
    rpos += 2;
    if (stream_id != 188//program_stream_map 1011 1100
        && stream_id != 190//padding_stream 1011 1110
        && stream_id != 191//private_stream_2 1011 1111
        && stream_id != 240//ECM 1111 0000
        && stream_id != 241//EMM 1111 0001
        && stream_id != 255//program_stream_directory 1111 1111
        && stream_id != 242//DSMCC_stream 1111 0010
        && stream_id != 248//ITU-T Rec. H.222.1 type E stream 1111 1000
        ) {
        printf("stream_id = %X -- header\n", stream_id);
        assert(0x80 == p[rpos]);
        //skip 2bits//'10' 2 bslbf
        int PES_scrambling_control = (p[rpos]&30)>>4; //PES_scrambling_control 2 bslbf
        int PES_priority = (p[rpos]&0x08)>>3; //PES_priority 1 bslbf
        int data_alignment_indicator = (p[rpos]&0x04)>>2;//data_alignment_indicator 1 bslbf
        int copyright = (p[rpos]&0x02)>>1; //copyright 1 bslbf
        int original_or_copy = (p[rpos]&0x01);//original_or_copy 1 bslbf
        rpos++;
        int PTS_DTS_flags = (p[rpos]&0xC0)>>6; //PTS_DTS_flags 2 bslbf
        int ESCR_flag = (p[rpos]&0x20)>>5; // ESCR_flag 1 bslbf
        int ES_rate_flag = (p[rpos]&0x10)>>4;//ES_rate_flag 1 bslbf
        int DSM_trick_mode_flag = (p[rpos]&0x08)>>3;//DSM_trick_mode_flag 1 bslbf
        int additional_copy_info_flag = (p[rpos]&0x04)>>2; //additional_copy_info_flag 1 bslbf
        int PES_CRC_flag = (p[rpos]&0x02)>>1; //PES_CRC_flag 1 bslbf
        int PES_extension_flag = (p[rpos]&0x01);//PES_extension_flag 1 bslbf
        rpos++;
        int PES_header_data_length = p[rpos]; //PES_header_data_length 8 uimsbf
        rpos++;
        printf("PES PES_header_data_length = %d\n", PES_header_data_length);
//        if( PES_header_data_length + rpos > remain_bytes )
//            error
        if (PTS_DTS_flags == 2) {
        // skip 4 bits '0010' 4 bslbf
        // PTS [32..30] 3 bslbf
        // marker_bit 1 bslbf
        // PTS [29..15] 15 bslbf
        // marker_bit 1 bslbf
        // PTS [14..0] 15 bslbf
        // marker_bit 1 bslbf
        uint64_t pts = (((p[rpos]>>1)&0x07) << 30) | (p[rpos+1]<<22) | (((p[rpos+2]>>1)&0x7F)<<15) | (p[rpos+3]<<7) | ((p[rpos+4]>>1)&0x7F);
        rpos += 5;
        }
        if (PTS_DTS_flags == 3) {
        // '0011' 4 bslbf
        // PTS [32..30] 3 bslbf
        // marker_bit 1 bslbf
        //PTS [29..15] 15 bslbf
        //marker_bit 1 bslbf
        // PTS [14..0] 15 bslbf
        // marker_bit 1 bslbf
        uint64_t pts = (((p[rpos]>>1)&0x07) << 30) | (p[rpos+1]<<22) | (((p[rpos+2]>>1)&0x7F)<<15) | (p[rpos+3]<<7) | ((p[rpos+4]>>1)&0x7F);
        rpos += 5;
        // '0001' 4 bslbf
        // DTS [32..30] 3 bslbf
        // marker_bit 1 bslbf
        // DTS [29..15] 15 bslbf
        // marker_bit 1 bslbf
        // DTS [14..0] 15 bslbf
        // marker_bit 1 bslbf
        uint64_t dts = (((p[rpos]>>1)&0x07) << 30) | (p[rpos+1]<<22) | (((p[rpos+2]>>1)&0x7F)<<15) | (p[rpos+3]<<7) | ((p[rpos+4]>>1)&0x7F);
        rpos += 5;
        }
        if (ESCR_flag == 1) {
        // reserved 2 bslbf
        // ESCR_base[32..30] 3 bslbf
        // marker_bit 1 bslbf
        // ESCR_base[29..15] 15 bslbf
        // marker_bit 1 bslbf
        // ESCR_base[14..0] 15 bslbf
        // marker_bit 1 bslbf
        // ESCR_extension 9 uimsbf
        // marker_bit 1 bslbf
        uint64_t ESCR_base = ((((uint64_t)p[rpos] >> 3) & 0x07) << 30) | (((uint64_t)p[rpos] & 0x03) << 28) | ((uint64_t)p[rpos + 1] << 20) | ((((uint64_t)p[rpos + 2] >> 3) & 0x1F) << 15) | (((uint64_t)p[rpos + 2] & 0x3) << 13) | ((uint64_t)p[rpos + 3] << 5) | ((p[rpos + 4] >> 3) & 0x1F);
        int ESCR_extension = ((p[rpos + 4] & 0x03) << 7) | ((p[rpos + 5] >> 1) & 0x7F);
        rpos += 6;
        }
        if (ES_rate_flag == 1) {
        // marker_bit 1 bslbf
        // ES_rate 22 uimsbf
        // marker_bit 1 bslbf
        int ES_rate = (p[rpos]&0x7F)<<15 | (p[rpos+1])<<7 | (p[rpos+2]&0x7F)>>1;
        rpos += 3;
        }
        if (DSM_trick_mode_flag == 1) { // ignore
            int trick_mode_control = (p[rpos]&0xE0)>>5;//trick_mode_control 3 uimsbf
            if ( trick_mode_control == 0/*fast_forward*/ ) {
                // field_id 2 bslbf
                // intra_slice_refresh 1 bslbf
                // frequency_truncation 2 bslbf
            }
            else if ( trick_mode_control == 1/*slow_motion*/ ) {
                //rep_cntrl 5 uimsbf
            }
            else if ( trick_mode_control == 2/*freeze_frame*/ ) {
                // field_id 2 uimsbf
                // reserved 3 bslbf
            }
            else if ( trick_mode_control == 3/*fast_reverse*/ ) {
                // field_id 2 bslbf
                // intra_slice_refresh 1 bslbf
                // frequency_truncation 2 bslbf
            }else if ( trick_mode_control == 4/*slow_reverse*/ ) {
                // rep_cntrl 5 uimsbf
            }
            else{
                //reserved 5 bslbf
            }
            rpos++;
        }
        if ( additional_copy_info_flag == 1) { // ignore
        // marker_bit 1 bslbf
        // additional_copy_info 7 bslbf
        rpos++;
        }
        if ( PES_CRC_flag == 1) { // ignore
        // previous_PES_packet_CRC 16 bslbf
        rpos += 2;
        }
        if ( PES_extension_flag == 1) { // ignore
            int PES_private_data_flag = (p[rpos]&0x80)>>7;// PES_private_data_flag 1 bslbf
            int pack_header_field_flag = (p[rpos]&0x40)>>6;// pack_header_field_flag 1 bslbf
            int program_packet_sequence_counter_flag = (p[rpos]&0x20)>>5;// program_packet_sequence_counter_flag 1 bslbf
            int P_STD_buffer_flag = (p[rpos]&0x10)>>4; // P-STD_buffer_flag 1 bslbf
            // reserved 3 bslbf
            int PES_extension_flag_2 = (p[rpos]&0x01);// PES_extension_flag_2 1 bslbf
            rpos++;

            if ( PES_private_data_flag == 1) {
             // PES_private_data 128 bslbf
             rpos += 16;
            }
            if (pack_header_field_flag == 1) {
             // pack_field_length 8 uimsbf
             // pack_header()
            }
            if (program_packet_sequence_counter_flag == 1) {
             // marker_bit 1 bslbf
             // program_packet_sequence_counter 7 uimsbf
             // marker_bit 1 bslbf
             // MPEG1_MPEG2_identifier 1 bslbf
             // original_stuff_length 6 uimsbf
             rpos += 2;
            }
            if ( P_STD_buffer_flag == 1) {
             // '01' 2 bslbf
             // P-STD_buffer_scale 1 bslbf
             // P-STD_buffer_size 13 uimsbf
             rpos += 2;
            }
            if ( PES_extension_flag_2 == 1) {
                // marker_bit 1 bslbf
                int PES_extension_field_length = (p[rpos]&0x7F);// PES_extension_field_length 7 uimsbf
                rpos++;
                for (int i = 0; i < PES_extension_field_length; i++) {
                 // reserved 8 bslbf
                 rpos++;
                }
            }
        }

//        for (int i = 0; i < N1; i++) {
        //stuffing_byte 8 bslbf
//            rpos++;
//        }
//        for (int i = 0; i < N2; i++) {
        //PES_packet_data_byte 8 bslbf
//        rpos++;
//        }
        printf("PES PES_header_data_length = %d rpos=%d npos=%d remain=%d hex=%s\n", PES_header_data_length, rpos, npos, 188-(npos+rpos), hexdump(p+rpos, 8));
        fwrite(p+rpos, 1, 188-(npos+rpos), fd);
    }
    else if ( stream_id == 188//program_stream_map 1011 1100 BC
             || stream_id == 191//private_stream_2 1011 1111 BF
             || stream_id == 240//ECM 1111 0000 F0
             || stream_id == 241//EMM 1111 0001 F1
             || stream_id == 255//program_stream_directory 1111 1111 FF
             || stream_id == 242//DSMCC_stream 1111 0010 F2
             || stream_id == 248//ITU-T Rec. H.222.1 type E stream 1111 1000 F8
             ) {
        printf("stream id = %X many type rpos=%d npos=%d remain=%d hex=%s\n", stream_id, rpos, npos, 188-(npos+rpos), hexdump(p, 8));
//        for (i = 0; i < PES_packet_length; i++) {
         //PES_packet_data_byte 8 bslbf
//         rpos++;
//        }
        fwrite(p, 1, 188-(npos+rpos), fd);
    }
    else if ( stream_id == 190//padding_stream 1011 1110
             ) {
//        for (i = 0; i < PES_packet_length; i++) {
        // padding_byte 8 bslbf
//            rpos++;
        printf("stream id = 0xBE padding_stream rpos=%d npos=%d remain=%d hex=%s\n", rpos, npos, 188-(npos+rpos), hexdump(p, 8));
        fwrite(p, 1, 188-(npos+rpos), fd);
//        }
    }
    
    return rpos;
}

const char* hexdump(const unsigned char *buf, const int num)
{
    char tbuf[128] = {0};
    for(int i = 0; i < num; i++)
    {
        sprintf(tbuf, "%s%02X ", tbuf, buf[i]);
    }
    return tbuf;
}

