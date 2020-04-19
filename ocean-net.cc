#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable.h"
#include <iostream>
#include <cmath>
#include <vector>
#include "ns3/ipv4-static-routing-helper.h"
#include "mypacket.h"
#include <fstream>
#include <sstream>
#include "ns3/ns2-mobility-helper.h"
#include "ns3/qos-tag.h"
#include "ns3/stdma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/netanim-module.h"



using namespace ns3;

//Class to output date to a file.

// class FileHandle{
//   public:
//     std::string m_filename;
//     std::ofstream m_outFile;

//     FileHandle(std::string filename);

//     void WriteHeader(std::string string);

//     void WriteData(std::string string);
    
//     void Close();

// };

// FileHandle::FileHandle(std::string filename){
//   m_filename = filename;
// }

// void FileHandle::WriteHeader(std::string string){
//   m_outFile = std::ofstream(m_filename.c_str());
//   m_outFile << string << std::endl;
//   m_outFile.close();
// }

// void FileHandle::WriteData(std::string string){
//   m_outFile = std::ofstream(m_filename.c_str(),std::ios::app);
//   m_outFile << string << std::endl;
//   Close();
// }

// void FileHandle::Close(){
//   m_outFile.close();
// }


static void
CourseChange(std::ostream * myos, std::string foo, Ptr <
  const MobilityModel > mobility) {
  Ptr < Node > node = mobility -> GetObject < Node > ();
  Vector pos = mobility -> GetPosition(); // Get position
  Vector vel = mobility -> GetVelocity(); // Get velocity

  std::cout.precision(5);
  * myos << Simulator::Now() << "; NODE: " << node -> GetId() << "; POS: x=" << pos.x << ", y=" << pos.y <<
    ", z=" << pos.z << "; VEL: x=" << vel.x << ", y=" << vel.y <<
    ", z=" << vel.z << std::endl;
}


class DtnApp: public Application {
  public:

    DtnApp();
    DtnApp(uint32_t b_s,uint32_t rp,uint32_t lifetime,uint32_t timeout,uint32_t num_retries):DtnApp(){};
    virtual~DtnApp();

    void Setup(Ptr < Node > node);
    void DstHandleConnectionCreated(Ptr < Socket > s,
      const Address & addr);
    void ReceiveHello(Ptr < Socket > socket);
    void ScheduleTx(uint32_t dstnode, Time tNext, uint32_t packetSize);
    void SendHello(Ptr < Socket > socket, double endTime, Time pktInterval, uint32_t first);
    void Retransmit(InetSocketAddress sendTo, int32_t id, int32_t retx);
    void SendMore(InetSocketAddress sendTo, int32_t id, int32_t retx);
    void ConnectionSucceeds(Ptr < Socket > localSocket);
    void ConnectionFails(Ptr < Socket > localSocket);
    void ReceiveBundle(Ptr < Socket > socket);

    private: virtual void StartApplication(void);
    virtual void StopApplication(void);

    void SendBundle(uint32_t dstnode, uint32_t packetSize);
    void SendAP(Ipv4Address srcaddr, Ipv4Address dstaddr, uint32_t seqno, Time srctimestamp);
    void PrintBuffers(void);
    void CheckBuffers(uint32_t bundletype);
    int IsDuplicate(Ptr < Packet > pkt, Ptr < Queue > queue);
    int AntipacketExists(Ptr < Packet > pkt);
    void RemoveBundle(Ptr < Packet > pkt);

    Ptr < Node > m_node;
    Ptr < Socket > m_socket;
    std::vector < Ptr < Packet > > newpkt;
    std::vector < Ptr < Packet > > retxpkt;
    Ptr < Queue > m_antipacket_queue;
    Ptr < Queue > m_queue;
    Ptr < Queue > m_helper_queue;
    Ptr < WifiMacQueue > mac_queue;
    Address m_peer;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_serverReadSize;
    uint32_t neighbors;
    InetSocketAddress * neighbor_address;
    double * neighbor_last_seen;
    uint32_t * currentServerRxBytes;
    int32_t ** neighbor_hello_bundles;
    int32_t ** neighbor_sent_bundles;
    int32_t ** neighbor_sent_aps;
    double ** neighbor_sent_ap_when;
    uint32_t bundles;
    InetSocketAddress * bundle_address;
    int32_t * bundle_seqno;
    int32_t * bundle_retx;
    uint32_t * bundle_size;
    double * bundle_ts;
    double firstSendTime[10000];
    double lastSendTime[10000];
    uint32_t lastTxBytes[10000];
    uint32_t currentTxBytes[10000];
    uint32_t totalTxBytes[10000];
    InetSocketAddress * sendTos;
    int32_t ids[10000];
    int32_t retxs[10000];
    int NumFlows;
    uint32_t drops;
    double t_c;
    uint32_t b_s;
    uint32_t * b_a;
    uint32_t rp;
    uint32_t cc;
    uint32_t timeout;
    uint32_t num_retries;
    uint32_t lifetime;
};

DtnApp::DtnApp(): m_socket(0),
  newpkt(NULL),
  retxpkt(NULL),
  m_antipacket_queue(NULL),
  m_queue(NULL),
  m_helper_queue(NULL),
  mac_queue(NULL),
  m_peer(),
  m_sendEvent(),
  m_running(false),
  m_serverReadSize(200), /* Is this OK? */
  neighbors(0),
  neighbor_address(NULL),
  neighbor_last_seen(NULL),
  currentServerRxBytes(NULL),
  neighbor_hello_bundles(NULL),
  neighbor_sent_bundles(NULL),
  neighbor_sent_aps(NULL),
  neighbor_sent_ap_when(NULL),
  bundles(0),
  bundle_address(NULL),
  bundle_seqno(NULL),
  bundle_retx(NULL),
  bundle_size(NULL),
  bundle_ts(NULL),
  sendTos(NULL),
  NumFlows(0),
  drops(0),
  t_c(0.8),
  b_s(1000000),
  b_a(NULL),
  rp(0), // 0: Epidemic, 1: Spray and Wait
  cc(0), // 0: No congestion control, 1: Static t_c, 2: Dynamic t_c
  lifetime(750),
  num_retries(3),
  timeout(1000)
{}



DtnApp::~DtnApp() {
  m_socket = 0;
}

void
DtnApp::Setup(Ptr < Node > node) {
  m_node = node;
  m_antipacket_queue = CreateObject < DropTailQueue > ();
  m_queue = CreateObject < DropTailQueue > ();
  m_helper_queue = CreateObject < DropTailQueue > ();
  m_antipacket_queue -> SetAttribute("MaxPackets", UintegerValue(1000));
  m_queue -> SetAttribute("MaxPackets", UintegerValue(1000));
  m_helper_queue -> SetAttribute("MaxPackets", UintegerValue(1000));
  for (int i = 0; i < 10000; i++) {
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  ns3::UniformVariable y;
  b_s = 1375000 + y.GetInteger(0, 1) * 9625000;
}

void
DtnApp::StartApplication(void) {
  m_running = true;
  Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (m_node->GetDevice (0));

  PointerValue ptr;
  dev->GetAttribute ("Mac",ptr);
  Ptr<AdhocWifiMac> mac = ptr.Get<AdhocWifiMac> ();
  NS_ASSERT (mac != NULL);
  Ptr<EdcaTxopN> edcaqueue = mac->GetBEQueue ();
  NS_ASSERT (edcaqueue != NULL);
  mac_queue = edcaqueue->GetQueue ();    
  NS_ASSERT (mac_queue != NULL);

  mac_queue -> SetAttribute("MaxPacketNumber", UintegerValue(1000));

  CheckBuffers(2);
  PrintBuffers();
}

void
DtnApp::StopApplication(void) {
  m_running = false;

  if (m_sendEvent.IsRunning())
    Simulator::Cancel(m_sendEvent);

  if (m_socket)
    m_socket -> Close();
}

void
DtnApp::ConnectionSucceeds(Ptr < Socket > localSocket) {
  //std::cout << "TCP connection succeeds at time " << Simulator::Now ().GetSeconds () <<
  //" at node " << m_node->GetId () << "\n";
}

void
DtnApp::ConnectionFails(Ptr < Socket > localSocket) {
  std::cout << "TCP connection fails at time " << Simulator::Now().GetSeconds() <<
    " at node " << m_node -> GetId() << "\n";
}

void
DtnApp::Retransmit(InetSocketAddress sendTo, int32_t id, int32_t retx) {
  // Check that this is last call for retransmit, otherwise return
  int index = 0, found = 0;
  while ((found == 0) && (index < NumFlows)) {
    if ((sendTos[index] == sendTo) && (ids[index] == id) && (retxs[index] == retx) && ((Simulator::Now().GetSeconds() - firstSendTime[index]) < lifetime)) {
      found = 1;
      if ((Simulator::Now().GetSeconds() - lastSendTime[index] < 1.0))
        return;
    } else
      index++;
  }
  if (found == 0)
    return;

  // Check that we are able to send, otherwise re-schedule and return
  uint32_t i = 0, neighbor_found = 0;
  while ((i < neighbors) && (neighbor_found == 0)) {
    if (neighbor_address[i].GetIpv4() == sendTo) {
      neighbor_found = 1;
      if ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) > 0.1) {
        Simulator::Schedule(Seconds(1.0), & DtnApp::Retransmit, this, sendTo, id, retx);
        return;
      }
    } else {
      i++;
    }
  }
  if (neighbor_found == 0)
    return;

  // Retransmit
  currentTxBytes[index] -= lastTxBytes[index];
  SendMore(sendTo, id, retx);
}

void
DtnApp::SendMore(InetSocketAddress sendTo, int32_t id, int32_t retx) {
  int index = 0, found = 0;
  while ((found == 0) && (index < NumFlows)) {
    if ((sendTos[index] == sendTo) && (ids[index] == id) && (retxs[index] == retx) && ((Simulator::Now().GetSeconds() - firstSendTime[index]) < lifetime)) {
      found = 1;
    } else
      index++;
  }
  if (found == 0)
    return;

  if (currentTxBytes[index] < totalTxBytes[index]) {
    uint32_t left = totalTxBytes[index] - currentTxBytes[index];
    uint32_t dataOffset = currentTxBytes[index] % 1472;
    uint32_t toWrite = 1472 - dataOffset;
    toWrite = std::min(toWrite, left);
    Ptr < Packet > packet = Create < Packet > (toWrite);
    if (currentTxBytes[index] == 0)
      packet = retxpkt[index] -> Copy();
    packet -> AddPacketTag(FlowIdTag(id));
    packet -> AddPacketTag(QosTag(retx));
    InetSocketAddress addr(sendTo.GetIpv4(), 50000);
    m_socket -> SendTo(packet, 0, addr);
    currentTxBytes[index] += toWrite;
    lastTxBytes[index] = toWrite;
    lastSendTime[index] = Simulator::Now().GetSeconds();
    Simulator::Schedule(Seconds(1.0), & DtnApp::Retransmit, this, sendTo, id, retx);
  } else
    lastTxBytes[index] = 0;
}

void
DtnApp::PrintBuffers(void) {
  uint32_t i = 0, currentNeighbors = 0;
  while (i < neighbors) {
    if ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1)
      currentNeighbors++;
    i++;
  }
  Ptr < MobilityModel > mobility = m_node -> GetObject < MobilityModel > ();
  std::cout << Simulator::Now().GetSeconds() <<
    " " << mac_queue -> GetSize() <<
    " " << m_antipacket_queue -> GetNPackets() <<
    " " << m_queue -> GetNPackets() <<
    " " << m_queue -> GetNBytes() <<
    " " << currentNeighbors <<
    " " << mobility -> GetPosition().x <<
    " " << mobility -> GetPosition().y << "\n";
  Simulator::Schedule(Seconds(1.0), & DtnApp::PrintBuffers, this);
}

void
DtnApp::CheckBuffers(uint32_t bundletype) {
  Ptr < Packet > packet;
  uint32_t i = 0, n = 0, pkts = 0, send_bundle = 0;
  mypacket::APHeader apHeader;
  mypacket::BndlHeader bndlHeader;

  // Remove expired antipackets and bundles -- do this check only once
  if (bundletype == 2) {
    pkts = m_antipacket_queue -> GetNPackets();
    n = 0;
    while (n < pkts) {
      n++;
      packet = m_antipacket_queue -> Dequeue();
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
      packet -> RemoveHeader(tHeader);
      packet -> RemoveHeader(apHeader);
      if ((Simulator::Now().GetSeconds() - apHeader.GetSrcTimestamp().GetSeconds()) < timeout) {
        packet -> AddHeader(apHeader);
        packet -> AddHeader(tHeader);
        bool success = m_antipacket_queue -> Enqueue(packet);
        if (success) {}
      } else {
        uint32_t n = 0;
        while (n < neighbors) {
          uint32_t m = 0, sent_found = 0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_aps[n][m] == -(int32_t) apHeader.GetOriginSeqno()) {
              sent_found = 1;
            } else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_aps[n][m] != 0) && (m < 999)) {
                neighbor_sent_aps[n][m] = neighbor_sent_aps[n][m + 1];
                neighbor_sent_ap_when[n][m] = neighbor_sent_ap_when[n][m + 1];
                m++;
              }
              neighbor_sent_aps[n][999] = 0;
              neighbor_sent_ap_when[n][999] = 0;
            }
          }
          n++;
        }
      }
    }
    pkts = m_queue -> GetNPackets();
    n = 0;
    while (n < pkts) {
      n++;
      packet = m_queue -> Dequeue();
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      packet -> RemoveHeader(tHeader);
      packet -> RemoveHeader(bndlHeader);
      if (((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < lifetime) || (bndlHeader.GetHopCount() == 0)) {
        packet -> AddHeader(bndlHeader);
        packet -> AddHeader(tHeader);
        bool success = m_queue -> Enqueue(packet);
        if (success) {}
      } else {
        uint32_t n = 0;
        while (n < neighbors) {
          uint32_t m = 0, sent_found = 0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_bundles[n][m] == (int32_t) bndlHeader.GetOriginSeqno()) {
              sent_found = 1;
            } else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_bundles[n][m] != 0) && (m < 999)) {
                neighbor_sent_bundles[n][m] = neighbor_sent_bundles[n][m + 1];
                m++;
              }
              neighbor_sent_bundles[n][999] = 0;
            }
          }
          n++;
        }
      }
    }
  }

  if (mac_queue -> GetSize() < 10) {
    if (bundletype == 2) {
      pkts = m_antipacket_queue -> GetNPackets();
      n = 0;
      while ((n < pkts) && (send_bundle == 0)) {
        n++;
        packet = m_antipacket_queue -> Dequeue();
        mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
        packet -> RemoveHeader(tHeader);
        packet -> RemoveHeader(apHeader);
        packet -> AddHeader(apHeader);
        packet -> AddHeader(tHeader);
        Ptr < Packet > qp = packet -> Copy();
        bool success = m_antipacket_queue -> Enqueue(qp);
        if (success) {}
        if ((Simulator::Now().GetSeconds() - apHeader.GetHopTimestamp().GetSeconds()) > 0.2) {
          i = 0;
          while ((i < neighbors) && (send_bundle == 0)) {
            if (((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1) && (neighbor_address[i].GetIpv4() != apHeader.GetOrigin())) {
              int neighbor_has_bundle = 0, ap_sent = 0, j = 0;
              while ((neighbor_has_bundle == 0) && (neighbor_hello_bundles[i][j] != 0) && (j < 1000)) {
                if (neighbor_hello_bundles[i][j] == -(int32_t) apHeader.GetOriginSeqno())
                  neighbor_has_bundle = 1;
                else
                  j++;
              }
              j = 0;
              while ((neighbor_has_bundle == 0) && (ap_sent == 0) && (neighbor_sent_aps[i][j] != 0) && (j < 1000)) {
                if ((neighbor_sent_aps[i][j] == -(int32_t) apHeader.GetOriginSeqno()) && (Simulator::Now().GetSeconds() - neighbor_sent_ap_when[i][j] < 1.5))
                  ap_sent = 1;
                else
                  j++;
              }
              if ((neighbor_has_bundle == 0) && (ap_sent == 0)) {
                send_bundle = 1;
                j = 0;
                while ((neighbor_sent_aps[i][j] != 0) && (neighbor_sent_aps[i][j] != -(int32_t) apHeader.GetOriginSeqno()) && (j < 999))
                  j++;
                neighbor_sent_aps[i][j] = -(int32_t) apHeader.GetOriginSeqno();
                neighbor_sent_ap_when[i][j] = Simulator::Now().GetSeconds();
              } else
                i++;
            } else
              i++;
          }
        }
      }
    } else {
      pkts = m_queue -> GetNPackets();
      n = 0;
      while ((n < pkts) && (send_bundle == 0)) {
        n++;
        packet = m_queue -> Dequeue();
        mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
        packet -> RemoveHeader(tHeader);
        packet -> RemoveHeader(bndlHeader);
        if ((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < lifetime) {
          if ((Simulator::Now().GetSeconds() - bndlHeader.GetHopTimestamp().GetSeconds()) > 0.2) {
            Ipv4Address dst = bndlHeader.GetDst();
            uint8_t spray = bndlHeader.GetSpray();
            i = 0;
            while ((i < neighbors) && (send_bundle == 0)) {
              if ((((bundletype == 0) && (spray > 0) && ((cc == 0) || (b_a[i] > packet -> GetSize()))) || (dst == neighbor_address[i].GetIpv4())) && \
                ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1) && (neighbor_address[i].GetIpv4() != bndlHeader.GetOrigin())) {
                int neighbor_has_bundle = 0, bundle_sent = 0, j = 0;
                while ((neighbor_has_bundle == 0) && (neighbor_hello_bundles[i][j] != 0) && (j < 1000)) {
                  if ((unsigned) neighbor_hello_bundles[i][j] == bndlHeader.GetOriginSeqno())
                    neighbor_has_bundle = 1;
                  else
                    j++;
                }
                j = 0;
                while ((neighbor_has_bundle == 0) && (bundle_sent == 0) && (neighbor_sent_bundles[i][j] != 0) && (j < 1000)) {
                  if (neighbor_sent_bundles[i][j] == (int32_t) bndlHeader.GetOriginSeqno())
                    bundle_sent = 1;
                  else
                    j++;
                }
                if ((neighbor_has_bundle == 0) && (bundle_sent == 0)) {
                  if (bundletype == 0) {
                    if (rp == 1)
                      bndlHeader.SetSpray(spray / 2);
                    if (cc > 0) {
                      if (packet -> GetSize() >= b_a[i])
                        b_a[i] = 0;
                      else
                        b_a[i] -= packet -> GetSize();
                    }
                  } else {
                    // Wait 5.0 seconds before forwarding to other (than dst) nodes
                    bndlHeader.SetHopTimestamp(Simulator::Now() + Seconds(5.0));
                  }
                  send_bundle = 1;
                  j = 0;
                  while ((neighbor_sent_bundles[i][j] != 0) && (j < 999))
                    j++;
                  neighbor_sent_bundles[i][j] = (int32_t) bndlHeader.GetOriginSeqno();
                } else
                  i++;
              } else
                i++;
            }
          }
          packet -> AddHeader(bndlHeader);
          packet -> AddHeader(tHeader);
          Ptr < Packet > qp = packet -> Copy();
          bool success = m_queue -> Enqueue(qp);
          if (success) {}
        } else {
          if (((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) > 1000.0) && (bndlHeader.GetHopCount() == 0) && (bndlHeader.GetNretx() < num_retries)) {
            bndlHeader.SetSrcTimestamp(Simulator::Now());
            bndlHeader.SetNretx(bndlHeader.GetNretx() + 1);
            uint32_t n = 0;
            while (n < neighbors) {
              uint32_t m = 0, sent_found = 0;
              while ((m < 1000) && (sent_found == 0)) {
                if (neighbor_sent_bundles[n][m] == (int32_t) bndlHeader.GetOriginSeqno()) {
                  sent_found = 1;
                } else
                  m++;
                if (sent_found == 1) {
                  while ((neighbor_sent_bundles[n][m] != 0) && (m < 999)) {
                    neighbor_sent_bundles[n][m] = neighbor_sent_bundles[n][m + 1];
                    m++;
                  }
                  neighbor_sent_bundles[n][999] = 0;
                }
              }
              n++;
            }
          }
          if ((bndlHeader.GetHopCount() == 0) && ((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) <= 1000.0)) {
            packet -> AddHeader(bndlHeader);
            packet -> AddHeader(tHeader);
            bool success = m_queue -> Enqueue(packet);
            if (success) {}
          }
        }
      }
    }
  } else {
    bundletype = 0;
  }
  if (send_bundle == 1) {
    if (m_socket == 0) {
      m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
      Ptr < Ipv4 > ipv4 = m_node -> GetObject < Ipv4 > ();
      Ipv4Address ipaddr = (ipv4 -> GetAddress(1, 0)).GetLocal();
      InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
      m_socket -> Bind(local);
    }

    InetSocketAddress dstremoteaddr(neighbor_address[i].GetIpv4(), 50000);
    if (bundletype < 2) {
      NumFlows++;
      sendTos = (InetSocketAddress * ) realloc(sendTos, NumFlows * sizeof(InetSocketAddress));
      sendTos[NumFlows - 1] = dstremoteaddr.GetIpv4();
      ids[NumFlows - 1] = bndlHeader.GetOriginSeqno();
      retxs[NumFlows - 1] = bndlHeader.GetNretx();
      currentTxBytes[NumFlows - 1] = std::min((uint32_t) 1472, packet -> GetSize());
      lastTxBytes[NumFlows - 1] = std::min((uint32_t) 1472, packet -> GetSize());
      firstSendTime[NumFlows - 1] = Simulator::Now().GetSeconds();
      lastSendTime[NumFlows - 1] = Simulator::Now().GetSeconds();
      totalTxBytes[NumFlows - 1] = packet -> GetSize();
      if (packet -> GetSize() > 1472)
        packet -> RemoveAtEnd(packet -> GetSize() - 1472);
      packet -> AddPacketTag(FlowIdTag(bndlHeader.GetOriginSeqno()));
      packet -> AddPacketTag(QosTag(bndlHeader.GetNretx()));
      retxpkt.push_back(packet -> Copy());
      Simulator::Schedule(Seconds(1.0), & DtnApp::Retransmit, this, sendTos[NumFlows - 1], ids[NumFlows - 1], retxs[NumFlows - 1]);
    } else {
      packet -> AddPacketTag(FlowIdTag(-apHeader.GetOriginSeqno()));
      packet -> AddPacketTag(QosTag(4));
    }

    m_socket -> SendTo(packet, 0, dstremoteaddr);
    Address ownaddress;
    m_socket -> GetSockName(ownaddress);
    InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress);
  }
  if (bundletype == 2) {
    if (send_bundle == 0)
      CheckBuffers(1);
    else
      Simulator::Schedule(Seconds(0.001), & DtnApp::CheckBuffers, this, 2);
  }
  if (bundletype == 1) {
    if (send_bundle == 0)
      CheckBuffers(0);
    else
      Simulator::Schedule(Seconds(0.001), & DtnApp::CheckBuffers, this, 2);
  }
  if (bundletype == 0) {
    if (send_bundle == 0)
      Simulator::Schedule(Seconds(0.01), & DtnApp::CheckBuffers, this, 2);
    else
      Simulator::Schedule(Seconds(0.001), & DtnApp::CheckBuffers, this, 2);
  }
}

void
DtnApp::SendBundle(uint32_t dstnode, uint32_t packetsize) {
  Ptr < Packet > packet = Create < Packet > (packetsize);
  mypacket::BndlHeader bndlHeader;
  char srcstring[1024] = "";
  sprintf(srcstring, "10.0.0.%d", (m_node -> GetId() + 1));
  char dststring[1024] = "";
  sprintf(dststring, "10.0.0.%d", (dstnode + 1));
  bndlHeader.SetOrigin(srcstring);
  bndlHeader.SetDst(dststring);
  bndlHeader.SetOriginSeqno(packet -> GetUid());
  bndlHeader.SetHopCount(0);
  bndlHeader.SetSpray(16);
  bndlHeader.SetNretx(0);
  bndlHeader.SetBundleSize(packetsize);
  bndlHeader.SetSrcTimestamp(Simulator::Now());
  bndlHeader.SetHopTimestamp(Simulator::Now());
  packet -> AddHeader(bndlHeader);
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  packet -> AddHeader(tHeader);
  if ((m_queue -> GetNBytes() + m_antipacket_queue -> GetNBytes() + packet -> GetSize()) <= b_s) {
    bool success = m_queue -> Enqueue(packet);
    if (success) {
      std::cout << "At time " << Simulator::Now().GetSeconds() <<
        " send bundle with sequence number " << bndlHeader.GetOriginSeqno() <<
        " from " << bndlHeader.GetOrigin() <<
        " to " << bndlHeader.GetDst() << "\n";
    }
  } else {
    std::cout << "At time " << Simulator::Now().GetSeconds() <<
      " tried to send bundle with sequence number " << bndlHeader.GetOriginSeqno() <<
      " from " << bndlHeader.GetOrigin() <<
      " to " << bndlHeader.GetDst() << "\n";
  }
}

void
DtnApp::SendAP(Ipv4Address srcstring, Ipv4Address dststring, uint32_t seqno, Time srctimestamp) {
  Ptr < Packet > packet = Create < Packet > (10);
  mypacket::APHeader apHeader;
  apHeader.SetOrigin(srcstring);
  apHeader.SetDst(dststring);
  apHeader.SetOriginSeqno(seqno);
  apHeader.SetHopCount(0);
  apHeader.SetBundleSize(10);
  double newtimestamp = srctimestamp.GetSeconds() - (250.0 - (Simulator::Now().GetSeconds() - srctimestamp.GetSeconds()));
  if (newtimestamp < 0.0)
    newtimestamp = 0.0;
  if ((Simulator::Now().GetSeconds() - srctimestamp.GetSeconds()) < 250.0)
    apHeader.SetSrcTimestamp(Seconds(newtimestamp));
  else
    apHeader.SetSrcTimestamp(srctimestamp);
  apHeader.SetHopTimestamp(Simulator::Now());
  packet -> AddHeader(apHeader);
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
  packet -> AddHeader(tHeader);
  bool success = m_antipacket_queue -> Enqueue(packet);
  if (success)
    std::cout << "At time " << Simulator::Now().GetSeconds() <<
    " send antipacket with sequence number " << apHeader.GetOriginSeqno() <<
    " original ts " << srctimestamp.GetSeconds() <<
    " new ts " << apHeader.GetSrcTimestamp().GetSeconds() <<
    " from " << apHeader.GetOrigin() <<
    " to " << apHeader.GetDst() << "\n";
}

void
DtnApp::ScheduleTx(uint32_t dstnode, Time tNext, uint32_t packetsize) {
  m_sendEvent = Simulator::Schedule(tNext, & DtnApp::SendBundle, this, dstnode, packetsize);
}

void
DtnApp::DstHandleConnectionCreated(Ptr < Socket > s,
  const Address & addr) {
  s -> SetRecvCallback(MakeCallback( & DtnApp::ReceiveBundle, this));
}

int
DtnApp::IsDuplicate(Ptr < Packet > pkt, Ptr < Queue > queue) {
  Ptr < Packet > cpkt = pkt -> Copy();
  int duplicate = 0;
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt -> RemoveHeader(tHeader);
  cpkt -> RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno();
  uint32_t pkts = queue -> GetNPackets();
  uint32_t i = 0;
  while (i < pkts) {
    Ptr < Packet > p = queue -> Dequeue();
    if (duplicate == 0) {
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      p -> RemoveHeader(tHeader);
      if (tHeader.Get() == mypacket::MYTYPE_AP)
        mypacket::APHeader bndlHeader;
      else
        mypacket::BndlHeader bndlHeader;
      p -> RemoveHeader(bndlHeader);
      if (bndlHeader.GetOriginSeqno() == seqno)
        duplicate = 1;
      p -> AddHeader(bndlHeader);
      p -> AddHeader(tHeader);
    }
    bool success = queue -> Enqueue(p);
    if (success) {}
    i++;
  }
  return (duplicate);
}

int
DtnApp::AntipacketExists(Ptr < Packet > pkt) {
  Ptr < Packet > cpkt = pkt -> Copy();
  int apExists = 0;
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt -> RemoveHeader(tHeader);
  cpkt -> RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno();
  uint32_t pkts = m_antipacket_queue -> GetNPackets();
  uint32_t i = 0;
  while (i < pkts) {
    Ptr < Packet > p = m_antipacket_queue -> Dequeue();
    if (apExists == 0) {
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
      mypacket::APHeader apHeader;
      p -> RemoveHeader(tHeader);
      p -> RemoveHeader(apHeader);
      if (apHeader.GetOriginSeqno() == seqno)
        apExists = 1;
      p -> AddHeader(apHeader);
      p -> AddHeader(tHeader);
    }
    bool success = m_antipacket_queue -> Enqueue(p);
    if (success) {}
    i++;
  }
  return (apExists);
}

void
DtnApp::RemoveBundle(Ptr < Packet > pkt) {
  Ptr < Packet > cpkt = pkt -> Copy();
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
  mypacket::APHeader apHeader;
  cpkt -> RemoveHeader(tHeader);
  cpkt -> RemoveHeader(apHeader);
  uint32_t seqno = apHeader.GetOriginSeqno();
  uint32_t pkts = m_queue -> GetNPackets();
  uint32_t i = 0, found = 0;
  while (i < pkts) {
    Ptr < Packet > p = m_queue -> Dequeue();
    if (found == 0) {
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      mypacket::BndlHeader bndlHeader;
      p -> RemoveHeader(tHeader);
      p -> RemoveHeader(bndlHeader);
      if (bndlHeader.GetOriginSeqno() != seqno) {
        p -> AddHeader(bndlHeader);
        p -> AddHeader(tHeader);
        bool success = m_queue -> Enqueue(p);
        if (success) {}
      } else {
        found = 1;
        uint32_t n = 0;
        while (n < neighbors) {
          uint32_t m = 0, sent_found = 0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_bundles[n][m] == (int32_t) seqno) {
              sent_found = 1;
            } else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_bundles[n][m] != 0) && (m < 999)) {
                neighbor_sent_bundles[n][m] = neighbor_sent_bundles[n][m + 1];
                m++;
              }
              neighbor_sent_bundles[n][999] = 0;
            }
          }
          n++;
        }
      }
    } else {
      bool success = m_queue -> Enqueue(p);
      if (success) {}
    }
    i++;
  }
  int index = 0;
  while (index < NumFlows) {
    if (ids[index] == (int32_t) seqno)
      ids[index] = 0;
    index++;
  }
}

void
DtnApp::ReceiveBundle(Ptr < Socket > socket) {
  Address ownaddress;
  socket -> GetSockName(ownaddress);
  InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress);
  while (socket -> GetRxAvailable() > 0) {
    Address from;
    Ptr < Packet > p = socket -> RecvFrom(from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);

    int src_seqno = 0;
    QosTag tag;
    int packet_type = 0;
    if (p -> PeekPacketTag(tag))
      packet_type = tag.GetTid();
    if (packet_type == 5) { // Ack
      p -> RemoveAllByteTags();
      p -> RemoveAllPacketTags();
      uint8_t * msg = new uint8_t[p -> GetSize() + 1];
      p -> CopyData(msg, p -> GetSize());
      msg[p -> GetSize()] = '\0';
      const char * src = reinterpret_cast <
        const char * > (msg);
      char word[1024];
      strcpy(word, "");
      int j = 0, n = 0;
      int32_t id = 0;
      int32_t retx = 0;
      while (sscanf(src, "%1023s%n", word, & n) == 1) {
        if (j == 0)
          id = strtol(word, NULL, 16);
        else
          retx = strtol(word, NULL, 16);
        strcpy(word, "");
        src += n;
        j++;
      }
      delete[] msg;
      SendMore(address.GetIpv4(), id, retx);
      return;
    } else {
      FlowIdTag ftag = 0;
      if (p -> PeekPacketTag(ftag))
        src_seqno = ftag.GetFlowId();
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024] = "";
      sprintf(seqnostring, " %x %x", src_seqno, packet_type); // Add: how much data received; will be used by the sender. If total received > bundle size: discard packet.
      msg << seqnostring;
      Ptr < Packet > ack = Create < Packet > ((uint8_t * ) msg.str().c_str(), msg.str().length());
      if (m_socket == 0) {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        Ptr < Ipv4 > ipv4 = m_node -> GetObject < Ipv4 > ();
        Ipv4Address ipaddr = (ipv4 -> GetAddress(1, 0)).GetLocal();
        InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
        m_socket -> Bind(local);
      }
      ack -> AddPacketTag(QosTag(5));
      InetSocketAddress ackaddr(address.GetIpv4(), 50000);
      m_socket -> SendTo(ack, 0, ackaddr);
    }
    p -> RemoveAllByteTags();
    p -> RemoveAllPacketTags();

    uint32_t i = 0;
    uint32_t found = 0;
    while ((i < bundles) && (found == 0)) {
      if ((address.GetIpv4() == bundle_address[i].GetIpv4()) && (src_seqno == bundle_seqno[i]) && (packet_type == bundle_retx[i])) {
        found = 1;
      } else
        i++;
    }
    if (found == 0) {
      i = 0;
      while ((i < bundles) && (found == 0)) {
        if (currentServerRxBytes[i] == 0) {
          found = 1;
          bundle_address[i] = address.GetIpv4();
          bundle_seqno[i] = src_seqno;
          bundle_retx[i] = packet_type;
          bundle_ts[i] = Simulator::Now().GetSeconds();
          currentServerRxBytes[i] = 0;
        } else
          i++;
      }
    }
    if (found == 0) {
      ++bundles;
      bundle_address = (InetSocketAddress * ) realloc(bundle_address, bundles * sizeof(InetSocketAddress));
      bundle_address[i] = address.GetIpv4();
      bundle_seqno = (int32_t * ) realloc(bundle_seqno, bundles * sizeof(int32_t));
      bundle_seqno[i] = src_seqno;
      bundle_retx = (int32_t * ) realloc(bundle_retx, bundles * sizeof(int32_t));
      bundle_retx[i] = packet_type;
      currentServerRxBytes = (uint32_t * ) realloc(currentServerRxBytes, bundles * sizeof(uint32_t));
      currentServerRxBytes[i] = 0;
      bundle_size = (uint32_t * ) realloc(bundle_size, bundles * sizeof(uint32_t));
      bundle_ts = (double * ) realloc(bundle_ts, bundles * sizeof(double));
      bundle_ts[i] = Simulator::Now().GetSeconds();
      newpkt.push_back(p -> Copy());
    }
    if (p == 0 && socket -> GetErrno() != Socket::ERROR_NOTERROR)
      NS_FATAL_ERROR("Server could not read stream at byte " << currentServerRxBytes[i]);
    if (currentServerRxBytes[i] == 0) {
      currentServerRxBytes[i] += p -> GetSize();
      newpkt[i] = p;
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i] -> RemoveHeader(tHeader);
      if (tHeader.Get() == mypacket::MYTYPE_AP) {
        mypacket::APHeader apHeader;
        newpkt[i] -> RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize() + 26;
        newpkt[i] -> AddHeader(apHeader);
      } else {
        if (tHeader.Get() == mypacket::MYTYPE_BNDL) {
          mypacket::BndlHeader bndlHeader;
          newpkt[i] -> RemoveHeader(bndlHeader);
          bundle_size[i] = bndlHeader.GetBundleSize() + 28;
          newpkt[i] -> AddHeader(bndlHeader);
        } else {
          // Bundle fragments arrive in wrong order; no bundle header
          currentServerRxBytes[i] = 0;
          return;
        }
      }
      newpkt[i] -> AddHeader(tHeader);
    } else {
      if (currentServerRxBytes[i] > bundle_size[i]) {
        std::cout << "WTF at time " << Simulator::Now().GetSeconds() <<
          " received " << p -> GetSize() <<
          " bytes at " << owniaddress.GetIpv4() <<
          " total bytes " << currentServerRxBytes[i] <<
          " from " << address.GetIpv4() <<
          " seqno " << src_seqno << "\n";
      } else {
        currentServerRxBytes[i] += p -> GetSize();
        newpkt[i] -> AddAtEnd(p);
      }
    }
    if (currentServerRxBytes[i] == bundle_size[i]) {
      currentServerRxBytes[i] = 0;
      Ptr < Packet > qpkt = newpkt[i] -> Copy();
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i] -> RemoveHeader(tHeader);
      if (tHeader.Get() == mypacket::MYTYPE_AP) {
        mypacket::APHeader apHeader;
        newpkt[i] -> RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize();
        if ((IsDuplicate(qpkt, m_antipacket_queue) == 0) && ((Simulator::Now().GetSeconds() - apHeader.GetSrcTimestamp().GetSeconds()) < 1000.0)) {
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          qpkt -> RemoveHeader(tHeader);
          mypacket::APHeader apHeader;
          qpkt -> RemoveHeader(apHeader);
          apHeader.SetHopTimestamp(Simulator::Now());
          apHeader.SetHopCount(apHeader.GetHopCount() + 1);
          qpkt -> AddHeader(apHeader);
          qpkt -> AddHeader(tHeader);
          bool success = m_antipacket_queue -> Enqueue(qpkt);
          if (success) {}
          RemoveBundle(qpkt);
        }
      } else {
        mypacket::BndlHeader bndlHeader;
        newpkt[i] -> RemoveHeader(bndlHeader);
        bundle_size[i] = bndlHeader.GetBundleSize();
        if (IsDuplicate(qpkt, m_queue) == 1)
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
          " received duplicate " << newpkt[i] -> GetSize() <<
          " bytes at " << owniaddress.GetIpv4() <<
          " from " << address.GetIpv4() <<
          " bundle hop count: " << (unsigned) bndlHeader.GetHopCount() <<
          " sequence number: " << bndlHeader.GetOriginSeqno() <<
          " bundle queue occupancy: " << m_queue -> GetNBytes() << "\n";
        if ((IsDuplicate(qpkt, m_queue) == 0) && (AntipacketExists(qpkt) == 0) && ((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < lifetime)) {
          if (bndlHeader.GetDst() == owniaddress.GetIpv4()) {
            std::cout << "At time " << Simulator::Now().GetSeconds() <<
              " received " << newpkt[i] -> GetSize() <<
              " bytes at " << owniaddress.GetIpv4() <<
              " (final dst) from " << address.GetIpv4() <<
              " delay: " << Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds() + 1000.0 * (bndlHeader.GetNretx()) <<
              " bundle hop count: " << (unsigned) bndlHeader.GetHopCount() + 1 <<
              " sequence number: " << bndlHeader.GetOriginSeqno() <<
              " bundle queue occupancy: " << m_queue -> GetNBytes() << "\n";
            SendAP(bndlHeader.GetDst(), bndlHeader.GetOrigin(), bndlHeader.GetOriginSeqno(), bndlHeader.GetSrcTimestamp());
          } else {
            mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
            qpkt -> RemoveHeader(tHeader);
            mypacket::BndlHeader bndlHeader;
            qpkt -> RemoveHeader(bndlHeader);
            bndlHeader.SetHopTimestamp(Simulator::Now());
            bndlHeader.SetHopCount(bndlHeader.GetHopCount() + 1);
            qpkt -> AddHeader(bndlHeader);
            qpkt -> AddHeader(tHeader);
            if ((m_queue -> GetNBytes() + m_antipacket_queue -> GetNBytes() + qpkt -> GetSize()) <= b_s) {
              bool success = m_queue -> Enqueue(qpkt);
              if (success) {}
            } else {
              drops++;
              std::cout << "At time " << Simulator::Now().GetSeconds() <<
                " dropped " << newpkt[i] -> GetSize() <<
                " bytes at " << owniaddress.GetIpv4() <<
                " from " << address.GetIpv4() <<
                " bundle hop count: " << (unsigned) bndlHeader.GetHopCount() <<
                " sequence number: " << bndlHeader.GetOriginSeqno() <<
                " bundle queue occupancy: " << m_queue -> GetNBytes() << "\n";
            }
          }
        }
      }
    }
  }
}

void
DtnApp::SendHello(Ptr < Socket > socket, double endTime, Time pktInterval, uint32_t first) {
  if (first == 0) {
    double now(Simulator::Now().GetSeconds());
    if (now < endTime) {
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024] = "";
      if (cc == 2) {
        if ((drops == 0) && (t_c < 0.9)) {
          t_c += 0.01;
        } else {
          if ((drops > 0) && (t_c > 0.5))
            t_c = t_c * 0.8;
          drops = 0;
        }
      }
      if ((m_queue -> GetNBytes() + m_antipacket_queue -> GetNBytes()) >= (uint32_t)(t_c * b_s))
        sprintf(seqnostring, "%d", 0);
      else
        sprintf(seqnostring, "%d", ((uint32_t)(t_c * b_s) - m_queue -> GetNBytes() - m_antipacket_queue -> GetNBytes()));
      msg << seqnostring;
      uint32_t pkts = m_queue -> GetNPackets();
      // Reorder packets: put the least forwarded first
      uint32_t n = 0;
      Ptr < Packet > packet;
      while (n < pkts) {
        n++;
        packet = m_queue -> Dequeue();
        bool success = m_helper_queue -> Enqueue(packet);
        if (success) {}
      }
      uint32_t m = 0;
      while (m < pkts) {
        m++;
        uint32_t min_count = 10000, min_seqno = 0, helper_pkts = m_helper_queue -> GetNPackets();
        n = 0;
        while (n < helper_pkts) {
          n++;
          packet = m_helper_queue -> Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet -> RemoveHeader(tHeader);
          packet -> RemoveHeader(bndlHeader);
          int index = 0;
          uint32_t count = 0;
          while (index < NumFlows) {
            if (ids[index] == (int32_t) bndlHeader.GetOriginSeqno())
              count++;
            index++;
          }
          if (count < min_count) {
            min_count = count;
            min_seqno = bndlHeader.GetOriginSeqno();
          }
          packet -> AddHeader(bndlHeader);
          packet -> AddHeader(tHeader);
          bool success = m_helper_queue -> Enqueue(packet);
          if (success) {}
        }
        int min_found = 0;
        while (min_found == 0) {
          packet = m_helper_queue -> Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet -> RemoveHeader(tHeader);
          packet -> RemoveHeader(bndlHeader);
          if (bndlHeader.GetOriginSeqno() == min_seqno) {
            min_found = 1;
            packet -> AddHeader(bndlHeader);
            packet -> AddHeader(tHeader);
            bool success = m_queue -> Enqueue(packet);
            if (success) {}
          } else {
            packet -> AddHeader(bndlHeader);
            packet -> AddHeader(tHeader);
            bool success = m_helper_queue -> Enqueue(packet);
            if (success) {}
          }
        }
      }
      // End of reorder  
      char seqnostring_b[1024] = "";
      sprintf(seqnostring_b, " %d", pkts);
      msg << seqnostring_b;
      for (uint32_t i = 0; i < pkts; ++i) {
        Ptr < Packet > p = m_queue -> Dequeue();
        if (msg.str().length() < 2280) {
          // The default value of MAC-level MTU is 2296
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          p -> RemoveHeader(tHeader);
          p -> RemoveHeader(bndlHeader);
          uint32_t src_seqno = bndlHeader.GetOriginSeqno();
          char seqnostring_a[1024] = "";
          sprintf(seqnostring_a, " %x", (src_seqno));
          msg << seqnostring_a;
          p -> AddHeader(bndlHeader);
          p -> AddHeader(tHeader);
        } else {
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello (B) (" << msg.str().length() << ") bytes.\n";
        }
        bool success = m_queue -> Enqueue(p);
        if (success) {}
      }
      uint32_t apkts = m_antipacket_queue -> GetNPackets();
      for (uint32_t i = 0; i < apkts; ++i) {
        Ptr < Packet > p = m_antipacket_queue -> Dequeue();
        if (msg.str().length() < 2280) {
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          mypacket::APHeader apHeader;
          p -> RemoveHeader(tHeader);
          p -> RemoveHeader(apHeader);
          uint32_t src_seqno = apHeader.GetOriginSeqno();
          char seqnostring_a[1024] = "";
          sprintf(seqnostring_a, " %x", (src_seqno));
          msg << seqnostring_a;
          p -> AddHeader(apHeader);
          p -> AddHeader(tHeader);
        } else {
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello (AP) (" << msg.str().length() << ") bytes.\n";
        }
        bool success = m_antipacket_queue -> Enqueue(p);
        if (success) {}
      }
      Ptr < Packet > pkt = Create < Packet > ((uint8_t * ) msg.str().c_str(), msg.str().length());
      pkt -> AddPacketTag(QosTag(6)); // High priority 
      socket -> Send(pkt);
      Simulator::Schedule(Seconds(0.1), & DtnApp::SendHello, this, socket, endTime, Seconds(0.1), 0);
    } else
      socket -> Close();
  } else
    Simulator::Schedule(pktInterval, & DtnApp::SendHello, this, socket, endTime, pktInterval, 0);
}

void
DtnApp::ReceiveHello(Ptr < Socket > socket) {
  Ptr < Packet > packet;
  Address from;
  while (packet = socket -> RecvFrom(from)) {
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    uint32_t i = 0;
    uint32_t found = 0;
    while ((i < neighbors) && (found == 0)) {
      if (address.GetIpv4() == neighbor_address[i].GetIpv4()) {
        found = 1;
      } else
        i++;
    }
    if (found == 0) {
      ++neighbors;
      neighbor_address = (InetSocketAddress * ) realloc(neighbor_address, neighbors * sizeof(InetSocketAddress));
      neighbor_address[i] = address.GetIpv4();
      neighbor_last_seen = (double * ) realloc(neighbor_last_seen, neighbors * sizeof(double));
      b_a = (uint32_t * ) realloc(b_a, neighbors * sizeof(uint32_t));
      neighbor_hello_bundles = (int32_t ** ) realloc(neighbor_hello_bundles, neighbors * sizeof(int32_t * ));
      neighbor_hello_bundles[i] = (int32_t * ) calloc(1000, sizeof(int32_t));
      neighbor_sent_bundles = (int32_t ** ) realloc(neighbor_sent_bundles, neighbors * sizeof(int32_t * ));
      neighbor_sent_bundles[i] = (int32_t * ) calloc(1000, sizeof(int32_t));
      neighbor_sent_aps = (int32_t ** ) realloc(neighbor_sent_aps, neighbors * sizeof(int32_t * ));
      neighbor_sent_aps[i] = (int32_t * ) calloc(1000, sizeof(int32_t));
      neighbor_sent_ap_when = (double ** ) realloc(neighbor_sent_ap_when, neighbors * sizeof(double * ));
      neighbor_sent_ap_when[i] = (double * ) calloc(1000, sizeof(double));
      for (uint32_t j = 0; j < 1000; j++) {
        neighbor_sent_bundles[i][j] = 0;
        neighbor_sent_aps[i][j] = 0;
        neighbor_sent_ap_when[i][j] = 0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();
    for (uint32_t j = 0; j < 1000; j++)
      neighbor_hello_bundles[i][j] = 0;

    uint8_t * msg = new uint8_t[packet -> GetSize() + 1];
    packet -> CopyData(msg, packet -> GetSize());
    msg[packet -> GetSize()] = '\0';
    const char * src = reinterpret_cast <
      const char * > (msg);
    char word[1024];
    strcpy(word, "");
    int j = 0, n = 0;
    int bundle_ids = 0;
    while (sscanf(src, "%1023s%n", word, & n) == 1) {
      if (j == 0) {
        b_a[i] = atoi(word);
      } else {
        if (j == 1) {
          bundle_ids = atoi(word);
        } else {
          if (j <= (bundle_ids + 1))
            neighbor_hello_bundles[i][j - 2] = strtol(word, NULL, 16);
          else
            neighbor_hello_bundles[i][j - 2] = -strtol(word, NULL, 16);
          int m = 0, sent_found = 0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_hello_bundles[i][j - 2] == neighbor_sent_aps[i][m]) {
              sent_found = 1;
            } else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_aps[i][m] != 0) && (m < 999)) {
                neighbor_sent_aps[i][m] = neighbor_sent_aps[i][m + 1];
                neighbor_sent_ap_when[i][m] = neighbor_sent_ap_when[i][m + 1];
                m++;
              }
              neighbor_sent_aps[i][999] = 0;
              neighbor_sent_ap_when[i][999] = 0;
            }
          }
        }
      }
      strcpy(word, "");
      src += n;
      j++;
    }
    delete[] msg;
  }
}

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  else
    {
      m_socket->Bind6 ();
    }
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


class Simulation{
public:
  Simulation();

  Simulation(uint32_t scenario):Simulation()
  {
    m_route_proto = scenario;
  }

  Simulation(double dist,uint32_t mac_proto):Simulation(){
    
    m_mac_proto = mac_proto;
    m_rangeCheck = true;
    vdist = dist;
  }

  



  void SetDefaultAttributes();

  void ParseCommandLineArguments (int argc, char **argv);

  void ConfigureNodes();

  void ConfigureMobility();

  void ConfigureChannels();

  void ConfigureDevices();

  void ConfigureApplications();

  void ConfigureTracing();

  void RunSimulation();

  void ProcessOutputs();

  void PopulateArpCache();


  void Simulate(int argc, char **argv);


  //Properties
  uint32_t m_nBases;
  uint32_t m_mac_proto; // 1. CSMA 2. STDMA
  uint32_t m_route_proto; //1. MANET 2. DTN
  uint32_t m_nNodes_array;
  uint32_t m_ar_nNodes;
  uint32_t m_adaptive_nNodes;
  uint32_t m_super_nNodes;
  bool m_rangeCheck; //true -> checks TDMA vs CSMA range else does ocean net simulation
  double m_freq;
  
  double m_totalSimTime;


  //Node array Distance for range check
  double vdist;


  //Ns3 Containers
  NodeContainer m_allNodes;
  NodeContainer m_baseNodes;
  NodeContainer m_arNodes;
  NodeContainer m_adaptiveNodes;
  NodeContainer m_superNodes;


  NetDeviceContainer m_allDevices;
  NetDeviceContainer m_adaptiveDevices;
  NetDeviceContainer m_arDevices;
  NetDeviceContainer m_superDevices;
  NetDeviceContainer m_baseDevices;
  
  Ipv4InterfaceContainer m_baseInterfaces;
  Ipv4InterfaceContainer m_adaptiveInterfaces;
  Ipv4InterfaceContainer m_arInterfaces;
  Ipv4InterfaceContainer m_allInterfaces;


  //Flowmonitor
  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;

private:
uint32_t GetProtocolOverheads()
  {
    ns3::WifiMacHeader hdr;
    hdr.SetTypeData();
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    stdma::StdmaHeader stdmaHdr;
    ns3::WifiMacTrailer fcs;
    return hdr.GetSerializedSize() + stdmaHdr.GetSerializedSize() + fcs.GetSerializedSize() + ns3::LLC_SNAP_HEADER_LENGTH;
  }

};

Simulation::Simulation()
:
  m_nBases(1),
  m_nNodes_array(5),
  m_mac_proto(1),
  m_route_proto(1),
  m_ar_nNodes(10),
  m_adaptive_nNodes(10),
  m_super_nNodes(2),
  m_freq(2.4e9),
  m_rangeCheck(false),
  m_totalSimTime(50)
{

}






void Simulation::Simulate(int argc, char **argv){

  SetDefaultAttributes();
  ParseCommandLineArguments (argc, argv);
  ConfigureNodes ();
  ConfigureChannels ();
  ConfigureDevices ();
  ConfigureMobility ();
  ConfigureApplications ();
  ConfigureTracing ();
  RunSimulation ();
  ProcessOutputs ();
}

void Simulation::SetDefaultAttributes(){

}

void Simulation::ParseCommandLineArguments (int argc, char** argv){

}

void Simulation::ConfigureNodes(){
  
  if(m_rangeCheck){
    m_adaptiveNodes.Create(m_nNodes_array);
    m_baseNodes.Create(1);
    m_allNodes.Add(m_baseNodes);
    m_allNodes.Add(m_adaptiveNodes);
  }

  else{
    m_arNodes.Create(m_ar_nNodes);
    m_adaptiveNodes.Create(m_adaptive_nNodes);
    m_baseNodes.Create(m_nBases);

    m_allNodes.Add(m_baseNodes);
    m_allNodes.Add(m_arNodes);
    m_allNodes.Add(m_adaptiveNodes);
  }

}

void Simulation::ConfigureChannels(){

  if(m_rangeCheck){
    //Propogation speed will be speed of light
    YansWifiChannelHelper WifiChannel;
    WifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    //Free space propogation loss only Line of sight path for transmission ( no reflections)
    WifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    Ptr<YansWifiChannel> Channel = WifiChannel.Create ();
    YansWifiPhyHelper basePhy = YansWifiPhyHelper::Default();
    basePhy.SetChannel(Channel);
    YansWifiPhyHelper nodePhy = YansWifiPhyHelper::Default();
    nodePhy.SetChannel(Channel);

    basePhy.Set("TxGain",DoubleValue(18.6));
    basePhy.Set("TxPowerStart",DoubleValue(28));
    basePhy.Set("TxPowerEnd",DoubleValue(28));
    nodePhy.Set("TxGain",DoubleValue(14.6));
    nodePhy.Set("TxPowerStart",DoubleValue(28));
    nodePhy.Set("TxPowerEnd",DoubleValue(28));

    if(m_mac_proto == 1){
      WifiHelper wifi;
      NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
      wifiMac.SetType ("ns3::AdhocWifiMac");
      wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

      m_adaptiveDevices = wifi.Install(nodePhy,wifiMac,m_adaptiveNodes);
      m_baseDevices = wifi.Install(basePhy,wifiMac,m_baseNodes);
    }
    else{
      ns3::SeedManager::SetSeed (2);
      stdma::StdmaHelper stdma;
      stdma.SetStandard(ns3::WIFI_PHY_STANDARD_80211p_CCH);
      stdma::StdmaMacHelper stdmaMac = stdma::StdmaMacHelper::Default();

      ns3::Config::SetDefault ("stdma::StdmaMac::FrameDuration", ns3::TimeValue(ns3::Seconds(1.0)));
      ns3::Config::SetDefault ("stdma::StdmaMac::MaximumPacketSize", ns3::UintegerValue(1020));
      ns3::Config::SetDefault ("stdma::StdmaMac::ReportRate", ns3::UintegerValue(1));
      ns3::Config::SetDefault ("stdma::StdmaMac::Timeout", ns3::RandomVariableValue (ns3::UniformVariable(8, 8)));
      m_adaptiveDevices = stdma.Install(nodePhy,stdmaMac,m_adaptiveNodes);
      m_baseDevices = stdma.Install(basePhy,stdmaMac,m_baseNodes);

    }

    m_allDevices.Add(m_baseDevices);
    m_allDevices.Add(m_adaptiveDevices);
    
  }

  else{

    YansWifiChannelHelper WifiChannel;
    WifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    WifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    Ptr<YansWifiChannel> baseChannel = WifiChannel.Create ();
    YansWifiPhyHelper basePhy = YansWifiPhyHelper::Default();
    basePhy.SetChannel(baseChannel);

    YansWifiPhyHelper ABEPhy = YansWifiPhyHelper::Default();
    ABEPhy.SetChannel(baseChannel);

    basePhy.Set("TxGain",DoubleValue(18.6));
    basePhy.Set("TxPowerStart",DoubleValue(28));
    basePhy.Set("TxPowerEnd",DoubleValue(28));
    ABEPhy.Set("TxGain",DoubleValue(14.6));
    ABEPhy.Set("TxPowerStart",DoubleValue(28));
    ABEPhy.Set("TxPowerEnd",DoubleValue(28));
    
    YansWifiPhyHelper arNodePhy = YansWifiPhyHelper::Default();
    arNodePhy.SetChannel(baseChannel); //Keeping this default

    
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    QosWifiMacHelper wifiMac = QosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");

    m_baseDevices = wifi.Install(basePhy,wifiMac,m_baseNodes);
    m_adaptiveDevices = wifi.Install(ABEPhy,wifiMac,m_adaptiveNodes);
    m_arDevices = wifi.Install(arNodePhy,wifiMac,m_arNodes);

    //Store All devices
    m_allDevices.Add(m_baseDevices);
    m_allDevices.Add(m_adaptiveDevices);
    m_allDevices.Add(m_arDevices);
    
  }
}

void Simulation::ConfigureDevices(){
  
  if(m_rangeCheck){
    InternetStackHelper stack;
    stack.Install(m_allNodes);
    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase ("10.0.0.0", "255.0.0.0");
    m_allInterfaces = addressAdhoc.Assign (m_allDevices); 
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  }
  else{

    AodvHelper aodv;
    OlsrHelper olsr;
    DsdvHelper dsdv;
    DsrHelper dsr;
    DsrMainHelper dsrMain;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;
    Ipv4AddressHelper addressAdhoc;
    Ipv4StaticRoutingHelper rtprot;

    switch(m_route_proto){
      case 1:
        list.Add (olsr, 100);
        break;
      case 2:
        list.Add (aodv, 100);
        break;
      case 3:
        list.Add (dsdv, 100);
        break;
      case 4:
        break;
      case 5:
        //DTN
        internet.SetRoutingHelper(rtprot);
        internet.Install(m_allNodes);
        break;
      default:
        //DEFAULT
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
        internet.Install(m_allNodes);
        break;
    }

    if(m_route_proto < 4){
      internet.SetRoutingHelper (list);
      internet.Install (m_allNodes);
    }
    else if(m_route_proto == 4){
      internet.Install (m_allNodes);
      dsrMain.Install (dsr, m_allNodes);
    }

    addressAdhoc.SetBase ("10.0.0.0", "255.0.0.0");
    m_baseInterfaces = addressAdhoc.Assign(m_baseDevices);
    m_adaptiveInterfaces = addressAdhoc.Assign(m_adaptiveDevices);
    m_arInterfaces = addressAdhoc.Assign(m_arDevices);
    m_allInterfaces.Add(m_baseInterfaces);
    m_allInterfaces.Add(m_adaptiveInterfaces);
    m_allInterfaces.Add(m_arInterfaces);

  }


  
}

void Simulation::ConfigureMobility(){

  //Stationary base stations
  MobilityHelper mobility;

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (50.0),
                                    "MinY", DoubleValue (0.0),
                                    "DeltaX", DoubleValue (1000.0),
                                    "DeltaY", DoubleValue (0.0),
                                    "GridWidth", UintegerValue (m_nBases),
                                    "LayoutType", StringValue ("RowFirst"));
  mobility.Install (m_baseNodes);


  //Other nodes
  if(m_rangeCheck){
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (50.0),
                                    "MinY", DoubleValue (vdist),
                                    "DeltaX", DoubleValue (10),
                                    "DeltaY", DoubleValue (0.0),
                                    "GridWidth", UintegerValue (m_nNodes_array),
                                    "LayoutType", StringValue ("RowFirst"));
    mobility.Install (m_adaptiveNodes);
  }
  else{
    //RWP for now will change to GaussMarkov later
    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=9]";
    std::stringstream ssPause;
    ssPause << "ns3::UniformRandomVariable[Min=0.0|Max=120]";

    ObjectFactory pos;
    pos.SetTypeId ("ns3::GridPositionAllocator");
    pos.Set("MinX",DoubleValue(50.0));
    pos.Set("MinY",DoubleValue(100.0));
    pos.Set("DeltaX",DoubleValue(500.0));
    pos.Set("GridWidth",UintegerValue(10));
    pos.Set("LayoutType",StringValue("RowFirst"));
    Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();

    mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                      "Speed", StringValue (ssSpeed.str ()),
                                      "Pause", StringValue (ssPause.str ()),
                                      "PositionAllocator", PointerValue (taPositionAlloc)
                              );

    // mobility.SetPositionAllocator("ns3::GridPositionAllocator",
    //                               "MinX",DoubleValue(50.0),
    //                               "MinY",DoubleValue(100.0),
    //                               "DeltaX",DoubleValue(500.0),
    //                               "GridWidth",UintegerValue(10),
    //                               "LayoutType",StringValue("RowFirst"));

    mobility.Install(m_adaptiveNodes);
    mobility.Install(m_arNodes);

  }



}

void Simulation::ConfigureApplications(){
  
  if(m_rangeCheck){
    
   
    //App for boats
    OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address (InetSocketAddress(m_allInterfaces.GetAddress(0),8080)));
    onoff1.SetConstantRate(ns3::DataRate("8000bps"),1000);

    //App for base station
    OnOffHelper onoff2("ns3::UdpSocketFactory",Address (InetSocketAddress(m_allInterfaces.GetAddress(1),8080)));
    onoff2.SetConstantRate(ns3::DataRate("8000bps"),1000);
   
    //Base station Sink
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
    // Ptr<Socket> sink = Socket::CreateSocket (m_baseNodes.Get(0), tid);
    // InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), 8080);

    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8080));
    ApplicationContainer sinkApps = packetSinkHelper.Install (m_baseNodes.Get(0));
    sinkApps.Start (Seconds (0));
    sinkApps.Stop (Seconds (m_totalSimTime));

    ApplicationContainer temp2 = onoff2.Install (m_baseNodes.Get (0));
    temp2.Start (Seconds (0));
    temp2.Stop (Seconds (m_totalSimTime));
    

    //Now lets Tell all the other nodes to send packet to the sink.
    for(uint32_t i=0;i<m_adaptiveNodes.GetN();i++){
     
      ApplicationContainer temp1 = onoff1.Install (m_adaptiveNodes.Get (i));
      temp1.Start (Seconds (0));
      temp1.Stop (Seconds (m_totalSimTime));

      //Sink
      ApplicationContainer sinkAppsTp = packetSinkHelper.Install (m_adaptiveNodes.Get(i)); 
      sinkAppsTp.Start (Seconds (0));
      sinkAppsTp.Stop (Seconds (m_totalSimTime));
    }
  }

  else{

    
    if(m_route_proto == 5){ //DTN
      TypeId udp_tid = TypeId::LookupByName("ns3::UdpSocketFactory");
      Ptr <DtnApp> app[m_allNodes.GetN()];
      for (uint32_t i = 0; i < m_allNodes.GetN(); ++i) {
        app[i] = CreateObject < DtnApp > ();
        app[i] -> Setup(m_allNodes.Get(i));
        m_allNodes.Get(i) -> AddApplication(app[i]);
        app[i] -> SetStartTime(Seconds(0.5 + 0.00001 * i));
        app[i] -> SetStopTime(Seconds(m_totalSimTime));

        //Setting up Recieve bundle sinks
        Ptr < Socket > dst = Socket::CreateSocket(m_allNodes.Get(i), udp_tid);
        char dststring[1024] = "";
        sprintf(dststring, "10.0.0.%d", (i + 1));
        InetSocketAddress dstlocaladdr(Ipv4Address(dststring), 50000);
        dst -> Bind(dstlocaladdr);
        dst -> SetRecvCallback(MakeCallback( & DtnApp::ReceiveBundle, app[i]));

        //Setting up Hello Packets Broadcasting
        Ptr < Socket > source = Socket::CreateSocket(m_allNodes.Get(i), udp_tid);
        InetSocketAddress remote(Ipv4Address("255.255.255.255"), 80);
        source -> SetAllowBroadcast(true);
        source -> Connect(remote);
        app[i] -> SendHello(source, m_totalSimTime, Seconds(0.1 + 0.00085 * i), 1);

        //Setting up RecieveHello Sink
        Ptr < Socket > recvSink = Socket::CreateSocket(m_allNodes.Get(i), udp_tid);
        InetSocketAddress local(Ipv4Address::GetAny(), 80);
        recvSink -> Bind(local);
        recvSink -> SetRecvCallback(MakeCallback( & DtnApp::ReceiveHello, app[i]));
      }

      //Code for sending
      ns3::UniformVariable x;
      for (uint32_t i = 0; i < m_allNodes.GetN(); ++i) {
      for (uint32_t j = 0; j < 10; ++j) {
        uint32_t dstnode = i;
        while (dstnode == i)
          dstnode = x.GetInteger(0, m_allNodes.GetN() - 1);
        app[i] -> ScheduleTx(dstnode, Seconds(2 * j + x.GetValue(5, 5)), 100 * (x.GetInteger(1, 10)));
      }

      PopulateArpCache();
    }


    }
    else{ //Rest of the stuff.
      for(uint32_t i=0;i<m_allNodes.GetN();++i){
        
        TypeId udp_tid = TypeId::LookupByName("ns3::UdpSocketFactory");


        //Recieve Sink
        Ptr < Socket > recvSink = Socket::CreateSocket(m_allNodes.Get(i), udp_tid);
        InetSocketAddress local(Ipv4Address::GetAny(), 80);
        recvSink -> Bind(local);

        Ptr<Socket> ns3Socket = Socket::CreateSocket (m_allNodes.Get (i), udp_tid);
        ns3::UniformVariable x;
        Ptr<MyApp> app = CreateObject<MyApp> ();
        uint32_t dstnode = i;
        while (dstnode == i)
          dstnode = x.GetInteger(0, m_allNodes.GetN() - 1);
        
        app->Setup (ns3Socket,InetSocketAddress (m_allInterfaces.GetAddress(dstnode), 80), 1040, 1000, DataRate ("1Mbps"));
        m_allNodes.Get(i)->AddApplication(app);
        app->SetStartTime (Seconds (1.));
        app->SetStopTime (Seconds (m_totalSimTime));
      }
      

    }

  }



}

void Simulation::ConfigureTracing(){
  // Flow Monitor
  flowmon = flowmonHelper.InstallAll ();
}

void Simulation::RunSimulation(){

  Simulator::Stop (Seconds(m_totalSimTime));
  Simulator::Run();
  
}

void Simulation::ProcessOutputs(){
    
  flowmon->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();
  float avgThroughput = 0;
  float totalflows = 0;
  int lostPackets = 0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "---- Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") ---- \n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      std::cout << " Lost packets: " << i->second.lostPackets << "\n";
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
      std::cout << " Delay: " << i->second.delaySum << "\n";
      avgThroughput += i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024 ;
      totalflows++;

      lostPackets += i->second.lostPackets;
  }

  std::cout << "------- Summary -----" << "\n";
  std::cout << "Distinct packet flows: "<<totalflows<<"\n";
  std::cout <<"Average Throughput: "<<avgThroughput/totalflows<<"\n";
  std::cout << "Total Packets Lost: " << lostPackets<<"\n";

  Simulator::Destroy();

}


void
Simulation::PopulateArpCache() {
  Ptr < ArpCache > arp = CreateObject < ArpCache > ();
  arp -> SetAliveTimeout(Seconds(3600 * 24 * 365));
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
    Ptr < Ipv4L3Protocol > ip = ( * i) -> GetObject < Ipv4L3Protocol > ();
    NS_ASSERT(ip != 0);
    ObjectVectorValue interfaces;
    ip -> GetAttribute("InterfaceList", interfaces);
    for (uint32_t j = 0; j != ip -> GetNInterfaces(); j++) {
      Ptr < Ipv4Interface > ipIface = ip -> GetInterface(j);
      NS_ASSERT(ipIface != 0);
      Ptr < NetDevice > device = ipIface -> GetDevice();
      NS_ASSERT(device != 0);
      Mac48Address addr = Mac48Address::ConvertFrom(device -> GetAddress());
      for (uint32_t k = 0; k < ipIface -> GetNAddresses(); k++) {
        Ipv4Address ipAddr = ipIface -> GetAddress(k).GetLocal();
        if (ipAddr == Ipv4Address::GetLoopback())
          continue;
        ArpCache::Entry * entry = arp -> Add(ipAddr);
        entry -> MarkWaitReply(0);
        entry -> MarkAlive(addr);
      }
    }
  }
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
    Ptr < Ipv4L3Protocol > ip = ( * i) -> GetObject < Ipv4L3Protocol > ();
    NS_ASSERT(ip != 0);
    ObjectVectorValue interfaces;
    ip -> GetAttribute("InterfaceList", interfaces);
    for (uint32_t j = 0; j != ip -> GetNInterfaces(); j++) {
      Ptr < Ipv4Interface > ipIface = ip -> GetInterface(j);
      ipIface -> SetAttribute("ArpCache", PointerValue(arp));
    }
  }
}


int main(int argc, char **argv){

  Simulation(3).Simulate(argc,argv);
  return 0;
}




