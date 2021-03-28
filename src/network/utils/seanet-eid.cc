#include "seanet-eid.h"
#include "ns3/log.h"
#include <string.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeanetEID");

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Get a hash key.
 * \param k the key
 * \param length the length of the key
 * \param level the previous hash, or an arbitrary value
 * \return hash
 * \note Adapted from Jens Jakobsen implementation (chillispot).
 */
static uint32_t lookuphash (unsigned char* k, uint32_t length, uint32_t level)
{
NS_LOG_FUNCTION (k << length << level);
#define mix(a, b, c) \
({ \
    (a) -= (b); (a) -= (c); (a) ^= ((c) >> 13); \
    (b) -= (c); (b) -= (a); (b) ^= ((a) << 8);  \
    (c) -= (a); (c) -= (b); (c) ^= ((b) >> 13); \
    (a) -= (b); (a) -= (c); (a) ^= ((c) >> 12); \
    (b) -= (c); (b) -= (a); (b) ^= ((a) << 16); \
    (c) -= (a); (c) -= (b); (c) ^= ((b) >> 5);  \
    (a) -= (b); (a) -= (c); (a) ^= ((c) >> 3);  \
    (b) -= (c); (b) -= (a); (b) ^= ((a) << 10); \
    (c) -= (a); (c) -= (b); (c) ^= ((b) >> 15); \
})

typedef uint32_t  ub4;   /* unsigned 4-byte quantities */
uint32_t a = 0;
uint32_t b = 0;
uint32_t c = 0;
uint32_t len = 0;

/* Set up the internal state */
len = length;
a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
c = level;           /* the previous hash value */

/* handle most of the key */
while (len >= 12)
    {
    a += (k[0] + ((ub4)k[1] << 8) + ((ub4)k[2] << 16) + ((ub4)k[3] << 24));
    b += (k[4] + ((ub4)k[5] << 8) + ((ub4)k[6] << 16) + ((ub4)k[7] << 24));
    c += (k[8] + ((ub4)k[9] << 8) + ((ub4)k[10] << 16) + ((ub4)k[11] << 24));
    mix (a, b, c);
    k += 12; 
    len -= 12;
    }

/* handle the last 11 bytes */
c += length;
switch (len) /* all the case statements fall through */
    {
    case 11: c += ((ub4)k[10] << 24);
    case 10: c += ((ub4)k[9] << 16);
    case 9: c += ((ub4)k[8] << 8);  /* the first byte of c is reserved for the length */
    case 8: b += ((ub4)k[7] << 24);
    case 7: b += ((ub4)k[6] << 16);
    case 6: b += ((ub4)k[5] << 8);
    case 5: b += k[4];
    case 4: a += ((ub4)k[3] << 24);
    case 3: a += ((ub4)k[2] << 16);
    case 2: a += ((ub4)k[1] << 8);
    case 1: a += k[0];
    /* case 0: nothing left to add */
    }
mix (a, b, c);

#undef mix

/* report the result */
return c;
}

#ifdef __cplusplus
}
#endif

SeanetEID::SeanetEID(){
    memset(eidbuf,0,EIDSIZE);
}

SeanetEID::SeanetEID(uint8_t* const eid){
    memcpy(eidbuf,eid,EIDSIZE);
}

void SeanetEID::getSeanetEID(uint8_t* eid) const{
    memcpy(eid,eidbuf,EIDSIZE);
}

void SeanetEID::setSeanetEID(uint8_t* const eid){
    memcpy(eidbuf,eid,EIDSIZE);
}
void SeanetEID::Print(){
    NS_LOG_INFO("EID: "<<(uint32_t)(eidbuf[0]-'0')<<" "<<(uint32_t)(eidbuf[1]-'0')<<" "<<(uint32_t)(eidbuf[2]-'0')<<" "<<(uint32_t)(eidbuf[3]-'0')<<" |"
   <<(uint32_t)(eidbuf[4]-'0')<<" "<<(uint32_t)(eidbuf[5]-'0')<<" "<<(uint32_t)(eidbuf[6]-'0')<<" "<<(uint32_t)(eidbuf[7]-'0')<<" |"
    <<(uint32_t)(eidbuf[8]-'0')<<" "<<(uint32_t)(eidbuf[9]-'0')<<" "<<(uint32_t)(eidbuf[10]-'0')<<" "<<(uint32_t)(eidbuf[11]-'0')<<" |"
    <<(uint32_t)(eidbuf[12]-'0')<<" "<<(uint32_t)(eidbuf[13]-'0')<<" "<<(uint32_t)(eidbuf[14]-'0')<<" "<<(uint32_t)(eidbuf[15]-'0')<<" |"
    <<(uint32_t)(eidbuf[16]-'0')<<" "<<(uint32_t)(eidbuf[17]-'0')<<" "<<(uint32_t)(eidbuf[18]-'0')<<" "<<(uint32_t)(eidbuf[19]-'0'));
}
SeanetEID::~SeanetEID ()
{
  /* do nothing */
  NS_LOG_FUNCTION (this);
}
size_t SeanetEIDHash::operator () (SeanetEID const &x) const
{
  uint8_t buf[EIDSIZE];

  x.getSeanetEID(buf);
//   printf("In seanet eid, eid is : %0x2 %0x2 %0x2 %0x2 sizeof buf %lu\n",buf[0],buf[1],buf[2],buf[3],sizeof(buf));
  return lookuphash (buf, sizeof (buf), 0);
}

}