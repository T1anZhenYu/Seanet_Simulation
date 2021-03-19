#ifndef SEANET_PROTOCOL_H
#define SEANET_PROTOCOL_H

// These definations are application types
#define RESOLUTION_APPLICATION 0x01
#define SWITCH_APPLICATION 0x02
#define NEIGH_INFO_APPLICATION 0x03
#define MULTICAST_APPLICATION 0x04
// These definations are used in Resolution
#define REQUEST_EID_NA 0x01 //request eid-na info 
#define REPLY_EID_NA 0x02 // resolution reply eid-na info to client
#define REGIST_EID_NA 0x03 // regist eid-na to resolution
#define PACKET_FINISH 0x01
#define PACKET_NOT_FINISH 0x00


//These definations are used in Switch
#define REQUEST_DATA 0x01 //client request data from switch
#define REPLY_DATA 0x02 //switch reply data to client
#define RESEIVE_DATA 0x03  // client send data to switch

//These definations are used in neighbor detect
#define NEIGH_INFO_RECEIVE 0x01
#define NEIGH_INFO_REPLY 0x02

//These definations are used in Multicast 
#define REGIST_TO_SOURCE_DR 0x01
#define REGIST_TO_RN 0x02
#define GRAFITING_REQUEST 0x03
#define MULTICAST_DATA_TRANS 0x04
#define REGIST_TO_DEST_DR 0x05


#define IS_DST 0x01
#define NOT_DST 0x00
#define MAX_PAYLOAD_LEN 255

#define TOTAL_FILE_NUM 10000


#endif