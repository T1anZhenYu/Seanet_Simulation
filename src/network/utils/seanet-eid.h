#ifndef SEANET_EID_H
#define SEANET_EID_H

#include <stdint.h>
#include <functional>
#define EIDSIZE 20
namespace ns3 {
    class SeanetEID{
        public:
        SeanetEID();
        SeanetEID(uint8_t* const eid);
        void getSeanetEID(uint8_t* eid) const;
        void setSeanetEID(uint8_t* const eid);
        void Print();
        ~SeanetEID ();
          /**
         * \brief Equal to operator.
         *
         * \param a the first operand.
         * \param b the first operand.
         * \returns true if the operands are equal.
         */
        friend bool operator == (const SeanetEID &a, const  SeanetEID &b);
        private:
        uint8_t eidbuf[EIDSIZE];
            
    };
    class SeanetEIDHash : public std::unary_function<SeanetEID, size_t>
    {
    public:
    /**
     * \brief Unary operator to hash IPv6 address.
     * \param x IPv6 address to hash
     * \returns the hash of the address
     */
    size_t operator () (SeanetEID const &x) const;
    };
    inline bool operator == (const SeanetEID &a, const  SeanetEID &b)
    {
        uint8_t bufa[EIDSIZE];
        uint8_t bufb[EIDSIZE];
        a.getSeanetEID(bufa);
        b.getSeanetEID(bufb);
        if(memcmp(bufa,bufb,EIDSIZE) != 0 ){
            return false;
        }
        return true;
    }
}
#endif