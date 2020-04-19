#include "mypacket.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3
{
  namespace mypacket
  {
    
    NS_OBJECT_ENSURE_REGISTERED (TypeHeader);
    
    TypeHeader::TypeHeader (MessageType t = MYTYPE_BNDL) :
      m_type (t), m_valid (true)
    {
    }
    
    TypeId
    TypeHeader::GetTypeId ()
    {
      static TypeId tid = TypeId ("ns3::mypacket::TypeHeader")
	.SetParent<Header> ()
	.AddConstructor<TypeHeader> ()
	;
      return tid;
    }
    
    TypeId
    TypeHeader::GetInstanceTypeId () const
    {
      return GetTypeId ();
    }
    
    uint32_t
    TypeHeader::GetSerializedSize () const
    {
      return 1;
    }
    
    void
    TypeHeader::Serialize (Buffer::Iterator i) const
    {
      i.WriteU8 ((uint8_t) m_type);
    }
    
    uint32_t
    TypeHeader::Deserialize (Buffer::Iterator start)
    {
      Buffer::Iterator i = start;
      uint8_t type = i.ReadU8 ();
      m_valid = true;
      switch (type)
	{
	case MYTYPE_BNDL:
	case MYTYPE_AP:
	  {
	    m_type = (MessageType) type;
	    break;
	  }
	default:
	  m_valid = false;
	}
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize ());
      return dist;
    }
    
    void
    TypeHeader::Print (std::ostream &os) const
    {
      switch (m_type)
	{
	case MYTYPE_BNDL:
	  {
	    os << "BNDL";
	    break;
	  }
	case MYTYPE_AP:
	  {
	    os << "AP";
	    break;
	  }
	default:
	  os << "UNKNOWN_TYPE";
	}
    }
    
    bool
    TypeHeader::operator== (TypeHeader const & o) const
    {
      return (m_type == o.m_type && m_valid == o.m_valid);
    }
    
    std::ostream &
    operator<< (std::ostream & os, TypeHeader const & h)
    {
      h.Print (os);
      return os;
    }
    
    //-----------------------------------------------------------------------------
    // BNDL
    //-----------------------------------------------------------------------------
    BndlHeader::BndlHeader (uint8_t hopCount, uint8_t spray, uint8_t nretx, Ipv4Address dst, Ipv4Address origin, uint32_t originSeqNo, uint32_t bundleSize, Time srcTimestamp, Time hopTimestamp) :
      m_hopCount (hopCount), m_spray (spray), m_nretx (nretx), m_dst(dst), m_origin(origin), m_originSeqNo (originSeqNo), m_bundleSize (bundleSize) 
    {
      m_srcTimestamp = uint32_t (srcTimestamp.GetMilliSeconds ());
      m_hopTimestamp = uint32_t (hopTimestamp.GetMilliSeconds ());
    }
    
    NS_OBJECT_ENSURE_REGISTERED (BndlHeader);
    
    TypeId
    BndlHeader::GetTypeId ()
    {
      static TypeId tid = TypeId ("ns3::mypacket::BndlHeader")
	.SetParent<Header> ()
	.AddConstructor<BndlHeader> ()
	;
      return tid;
    }
    
    TypeId
    BndlHeader::GetInstanceTypeId () const
    {
      return GetTypeId ();
    }
    
    uint32_t
    BndlHeader::GetSerializedSize () const
    {
      return 27;
    }
    
    void
    BndlHeader::Serialize (Buffer::Iterator i) const
    {
      i.WriteU8 (m_hopCount);
      i.WriteU8 (m_spray);
      i.WriteU8 (m_nretx);
      WriteTo (i, m_dst);
      WriteTo (i, m_origin);
      i.WriteHtonU32 (m_originSeqNo);
      i.WriteHtonU32 (m_bundleSize);
      i.WriteHtonU32 (m_srcTimestamp);
      i.WriteHtonU32 (m_hopTimestamp);
    }
    
    uint32_t
    BndlHeader::Deserialize (Buffer::Iterator start)
    {
      Buffer::Iterator i = start;
      m_hopCount = i.ReadU8 ();
      m_spray = i.ReadU8 ();
      m_nretx = i.ReadU8 ();
      ReadFrom (i, m_dst);
      ReadFrom (i, m_origin);
      m_originSeqNo = i.ReadNtohU32 ();
      m_bundleSize = i.ReadNtohU32 ();
      m_srcTimestamp = i.ReadNtohU32 ();
      m_hopTimestamp = i.ReadNtohU32 ();
      
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize ());
      return dist;
    }
    
    void
    BndlHeader::Print (std::ostream &os) const
    {
      os << "Destination: ipv4 " << m_dst
	 << " source: ipv4 " << m_origin
	 << " sequence number " << m_originSeqNo
	 << " bundle size " << m_bundleSize
	 << " source timestamp " << m_srcTimestamp
	 << " local timestamp " << m_hopTimestamp;
    }

    void
    BndlHeader::SetSrcTimestamp (Time t)
    {
      m_srcTimestamp = t.GetMilliSeconds ();
    }
    
    Time
    BndlHeader::GetSrcTimestamp () const
    {
      Time t (MilliSeconds (m_srcTimestamp));
      return t;
    }   
    
    void
    BndlHeader::SetHopTimestamp (Time t)
    {
      m_hopTimestamp = t.GetMilliSeconds ();
    }
    
    Time
    BndlHeader::GetHopTimestamp () const
    {
      Time t (MilliSeconds (m_hopTimestamp));
      return t;
    }
    
    std::ostream &
    operator<< (std::ostream & os, BndlHeader const & h)
    {
      h.Print (os);
      return os;
    }
    
    bool
    BndlHeader::operator== (BndlHeader const & o) const
    {
      return (m_hopCount == o.m_hopCount &&
	      m_spray == o.m_spray &&
	      m_nretx == o.m_nretx &&
	      m_dst == o.m_dst &&
	      m_origin == o.m_origin &&
	      m_originSeqNo == o.m_originSeqNo &&
	      m_bundleSize == o.m_bundleSize &&
	      m_srcTimestamp == o.m_srcTimestamp &&
	      m_hopTimestamp == o.m_hopTimestamp);
    }
    
    //-----------------------------------------------------------------------------
    // AP
    //-----------------------------------------------------------------------------
    
    APHeader::APHeader (uint8_t hopCount, Ipv4Address dst, Ipv4Address origin, uint32_t originSeqNo, uint32_t bundleSize, Time srcTimestamp, Time hopTimestamp) :
      m_hopCount (hopCount), m_dst(dst), m_origin(origin), m_originSeqNo (originSeqNo), m_bundleSize (bundleSize) 
    {
      m_srcTimestamp = uint32_t (srcTimestamp.GetMilliSeconds ());
      m_hopTimestamp = uint32_t (hopTimestamp.GetMilliSeconds ());
    }
    
    NS_OBJECT_ENSURE_REGISTERED (APHeader);
    
    TypeId
    APHeader::GetTypeId ()
    {
      static TypeId tid = TypeId ("ns3::mypacket::APHeader")
	.SetParent<Header> ()
	.AddConstructor<BndlHeader> ()
	;
      return tid;
    }
    
    TypeId
    APHeader::GetInstanceTypeId () const
    {
      return GetTypeId ();
    }
    
    uint32_t
    APHeader::GetSerializedSize () const
    {
      return 25;
    }
    
    void
    APHeader::Serialize (Buffer::Iterator i) const
    {
      i.WriteU8 (m_hopCount);
      WriteTo (i, m_dst);
      WriteTo (i, m_origin);
      i.WriteHtonU32 (m_originSeqNo);
      i.WriteHtonU32 (m_bundleSize);
      i.WriteHtonU32 (m_srcTimestamp);
      i.WriteHtonU32 (m_hopTimestamp);
    }
    
    uint32_t
    APHeader::Deserialize (Buffer::Iterator start)
    {
      Buffer::Iterator i = start;
      m_hopCount = i.ReadU8 ();
      ReadFrom (i, m_dst);
      ReadFrom (i, m_origin);
      m_originSeqNo = i.ReadNtohU32 ();
      m_bundleSize = i.ReadNtohU32 ();
      m_srcTimestamp = i.ReadNtohU32 ();
      m_hopTimestamp = i.ReadNtohU32 ();
      
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize ());
      return dist;
    }
    
    void
    APHeader::Print (std::ostream &os) const
    {
      os << "Destination: ipv4 " << m_dst
	 << " source: ipv4 " << m_origin
	 << " sequence number " << m_originSeqNo
	 << " bundle size " << m_bundleSize
	 << " source timestamp " << m_srcTimestamp
         << " local timestamp " << m_hopTimestamp;
    }

    void
    APHeader::SetSrcTimestamp (Time t)
    {
      m_srcTimestamp = t.GetMilliSeconds ();
    }

    Time
    APHeader::GetSrcTimestamp () const
    {
      Time t (MilliSeconds (m_srcTimestamp));
      return t;
    }

    void
    APHeader::SetHopTimestamp (Time t)
    {
      m_hopTimestamp = t.GetMilliSeconds ();
    }

    Time
    APHeader::GetHopTimestamp () const
    {
      Time t (MilliSeconds (m_hopTimestamp));
      return t;
    }
    
    std::ostream &
    operator<< (std::ostream & os, APHeader const & h)
    {
      h.Print (os);
      return os;
    }

    bool
    APHeader::operator== (APHeader const & o) const
    {
      return (m_hopCount == o.m_hopCount &&
	      m_dst == o.m_dst &&
	      m_origin == o.m_origin &&
	      m_originSeqNo == o.m_originSeqNo &&
	      m_bundleSize == o.m_bundleSize &&
	      m_srcTimestamp == o.m_srcTimestamp &&
              m_hopTimestamp == o.m_hopTimestamp);
    }
  }
}
