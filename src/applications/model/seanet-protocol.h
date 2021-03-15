#ifndef SEANET_STORAGE_PROTOCOL_H
#define SEANET_STORAGE_PROTOCOL_H

// These definations are application types
#define RESOLUTION_APPLICATION 0x05
#define SWITCH_APPLICATION 0x06
#define NEIGH_INFO_APPLICATION 0x07
// These definations are used in Resolution
#define REQUEST_EID_NA 0x05 //request eid-na info 
#define REPLY_EID_NA 0x06 // resolution reply eid-na info to client
#define REGIST_EID_NA 0x07 // regist eid-na to resolution
//These definations are used in Switch
#define REQUEST_DATA 0x05 //client request data from switch
#define REPLY_DATA 0x06 //switch reply data to client
#define RESEIVE_DATA 0x07  // client send data to switch

//These definations are used in neighbor detect
#define NEIGH_INFO_RECEIVE 0x05
#define NEIGH_INFO_REPLY 0x06

#define IS_DST 0x01
#define NOT_DST 0x00
#define MAX_PAYLOAD_LEN 128

#define TOTAL_FILE_NUM 10000
#endif