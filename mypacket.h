#ifndef MYPACKET_H
#define MYPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
  namespace mypacket {
    
    enum MessageType
    {
      MYTYPE_BNDL  = 1,
      MYTYPE_AP  = 2
    };
    
    class TypeHeader : public Header
    {
    public:
      TypeHeader (MessageType t);
      
      static TypeId GetTypeId ();
      TypeId GetInstanceTypeId () const;
      uint32_t GetSerializedSize () const;
      void Serialize (Buffer::Iterator start) const;
      uint32_t Deserialize (Buffer::Iterator start);
      void Print (std::ostream &os) const;
      
      MessageType Get () const { return m_type; }
      bool IsValid () const { return m_valid; }
      bool operator== (TypeHeader const & o) const;
    private:
      MessageType m_type;
      bool m_valid;
    };
    
    std::ostream & operator<< (std::ostream & os, TypeHeader const & h);
    
    /**
       
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       Primary Bundle Block
       +----------------+----------------+----------------+----------------+
       |    Version     |                  Proc. Flags (*)                 |
       +----------------+----------------+----------------+----------------+
       |                          Block length (*)                         |
       +----------------+----------------+---------------------------------+
       |   Destination scheme offset (*) |     Destination SSP offset (*)  |
       +----------------+----------------+----------------+----------------+
       |      Source scheme offset (*)   |        Source SSP offset (*)    |
       +----------------+----------------+----------------+----------------+
       |    Report-to scheme offset (*)  |      Report-to SSP offset (*)   |
       +----------------+----------------+----------------+----------------+
       |    Custodian scheme offset (*)  |      Custodian SSP offset (*)   |
       +----------------+----------------+----------------+----------------+
       |                    Creation Timestamp time (*)                    |
       +---------------------------------+---------------------------------+
       |             Creation Timestamp sequence number (*)                |
       +---------------------------------+---------------------------------+
       |                           Lifetime (*)                            |
       +----------------+----------------+----------------+----------------+
       |                        Dictionary length (*)                      |
       +----------------+----------------+----------------+----------------+
       |                  Dictionary byte array (variable)                 |
       +----------------+----------------+---------------------------------+
       |                      [Fragment offset (*)]                        |
       +----------------+----------------+---------------------------------+
       |              [Total application data unit length (*)]             |
       +----------------+----------------+---------------------------------+
       
       Bundle Payload Block
       +----------------+----------------+----------------+----------------+
       |  Block type    | Proc. Flags (*)|        Block length(*)          |
       +----------------+----------------+----------------+----------------+
       /                     Bundle Payload (variable)                     /
       +-------------------------------------------------------------------+
       
    */
    class BndlHeader : public Header 
    {
    public:
      BndlHeader (uint8_t hopCount = 0, uint8_t spray = 0, uint8_t nretx = 0, Ipv4Address dst = Ipv4Address (),
		  Ipv4Address origin = Ipv4Address (), uint32_t originSeqNo = 0, uint32_t bundleSize = 0, Time srcTimestamp = MilliSeconds (0), Time hopTimestamp = MilliSeconds (0));
      
      static TypeId GetTypeId ();
      TypeId GetInstanceTypeId () const;
      uint32_t GetSerializedSize () const;
      void Serialize (Buffer::Iterator start) const;
      uint32_t Deserialize (Buffer::Iterator start);
      void Print (std::ostream &os) const;
      
      void SetHopCount (uint8_t count) { m_hopCount = count; }
      uint8_t GetHopCount () const { return m_hopCount; }
      void SetSpray (uint8_t spray) { m_spray = spray; }
      uint8_t GetSpray () const { return m_spray; }
      void SetNretx (uint8_t nretx) { m_nretx = nretx; }
      uint8_t GetNretx () const { return m_nretx; }
      void SetDst (Ipv4Address a) { m_dst = a; }
      Ipv4Address GetDst () const { return m_dst; }
      void SetOrigin (Ipv4Address a) { m_origin = a; }
      Ipv4Address GetOrigin () const { return m_origin; }
      void SetOriginSeqno (uint32_t s) { m_originSeqNo = s; }
      uint32_t GetOriginSeqno () const { return m_originSeqNo; }
      void SetBundleSize (uint32_t s) { m_bundleSize = s; }
      uint32_t GetBundleSize () const { return m_bundleSize; }
      void SetSrcTimestamp (Time s);
      Time GetSrcTimestamp () const;
      void SetHopTimestamp (Time s);
      Time GetHopTimestamp () const;

      bool operator== (BndlHeader const & o) const;
    private:
      uint8_t        m_hopCount;
      uint8_t        m_spray;
      uint8_t        m_nretx;
      Ipv4Address    m_dst;
      Ipv4Address    m_origin;
      uint32_t        m_originSeqNo;
      uint32_t       m_bundleSize;
      uint32_t       m_srcTimestamp;
      uint32_t       m_hopTimestamp;
    };
    
    std::ostream & operator<< (std::ostream & os, BndlHeader const &);
    
    class APHeader : public Header
    {
    public:
      APHeader (uint8_t hopCount = 0, Ipv4Address dst = Ipv4Address (),
		Ipv4Address origin = Ipv4Address (), uint32_t originSeqNo = 0, uint32_t bundleSize = 0, Time srcTimestamp = MilliSeconds (0), Time hopTimestamp = MilliSeconds (0));
      
      static TypeId GetTypeId ();
      TypeId GetInstanceTypeId () const;
      uint32_t GetSerializedSize () const;
      void Serialize (Buffer::Iterator start) const;
      uint32_t Deserialize (Buffer::Iterator start);
      void Print (std::ostream &os) const;
      
      void SetHopCount (uint8_t count) { m_hopCount = count; }
      uint8_t GetHopCount () const { return m_hopCount; }
      void SetDst (Ipv4Address a) { m_dst = a; }
      Ipv4Address GetDst () const { return m_dst; }
      void SetOrigin (Ipv4Address a) { m_origin = a; }
      Ipv4Address GetOrigin () const { return m_origin; }
      void SetOriginSeqno (uint32_t s) { m_originSeqNo = s; }
      uint32_t GetOriginSeqno () const { return m_originSeqNo; }
      void SetBundleSize (uint32_t s) { m_bundleSize = s; }
      uint32_t GetBundleSize () const { return m_bundleSize; }
      void SetSrcTimestamp (Time s);
      Time GetSrcTimestamp () const;
      void SetHopTimestamp (Time s);
      Time GetHopTimestamp () const;
      
      bool operator== (APHeader const & o) const;
    private:
      uint8_t        m_hopCount;
      Ipv4Address    m_dst;
      Ipv4Address    m_origin;
      uint32_t        m_originSeqNo;
      uint32_t       m_bundleSize;
      uint32_t       m_srcTimestamp;
      uint32_t       m_hopTimestamp;
    };
    
    std::ostream & operator<< (std::ostream & os, APHeader const &);
  }
}
#endif /* MYPACKET_H */
