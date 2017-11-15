/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "simple-net-device.h"
#include "simple-channel.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/tag.h"
#include "ns3/simulator.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleNetDevice");

/**
 * \brief SimpleNetDevice tag to store source, destination and protocol of each packet.
 */
class SimpleTag : public Tag {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  /**
   * Set the source address
   * \param src source address
   */
  void SetSrc (Mac48Address src);
  /**
   * Get the source address
   * \return the source address
   */
  Mac48Address GetSrc (void) const;

  /**
   * Set the destination address
   * \param dst destination address
   */
  void SetDst (Mac48Address dst);
  /**
   * Get the destination address
   * \return the destination address
   */
  Mac48Address GetDst (void) const;

  /**
   * Set the protocol number
   * \param proto protocol number
   */
  void SetProto (uint16_t proto);
  /**
   * Get the protocol number
   * \return the protocol number
   */
  uint16_t GetProto (void) const;

  void Print (std::ostream &os) const;

private:
  Mac48Address m_src; //!< source address
  Mac48Address m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
};


NS_OBJECT_ENSURE_REGISTERED (SimpleTag);

TypeId
SimpleTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<SimpleTag> ()
  ;
  return tid;
}
TypeId
SimpleTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SimpleTag::GetSerializedSize (void) const
{
  return 8+8+2;
}
void
SimpleTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_src.CopyTo (mac);
  i.Write (mac, 6);
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
}
void
SimpleTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];
  i.Read (mac, 6);
  m_src.CopyFrom (mac);
  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
}

void
SimpleTag::SetSrc (Mac48Address src)
{
  m_src = src;
}

Mac48Address
SimpleTag::GetSrc (void) const
{
  return m_src;
}

void
SimpleTag::SetDst (Mac48Address dst)
{
  m_dst = dst;
}

Mac48Address
SimpleTag::GetDst (void) const
{
  return m_dst;
}

void
SimpleTag::SetProto (uint16_t proto)
{
  m_protocolNumber = proto;
}

uint16_t
SimpleTag::GetProto (void) const
{
  return m_protocolNumber;
}

void
SimpleTag::Print (std::ostream &os) const
{
  os << "src=" << m_src << " dst=" << m_dst << " proto=" << m_protocolNumber;
}



NS_OBJECT_ENSURE_REGISTERED (SimpleNetDevice);

TypeId 
SimpleNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Network") 
    .AddConstructor<SimpleNetDevice> ()
    .AddAttribute ("ReceiveErrorModel",
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&SimpleNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("PointToPointMode",
                   "The device is configured in Point to Point mode",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimpleNetDevice::m_pointToPointMode),
                   MakeBooleanChecker ())
    .AddAttribute ("TxQueue",
                   "A queue to use as the transmit queue in the device.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&SimpleNetDevice::m_queue),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("DataRate",
                   "The default data rate for point to point links. Zero means infinite",
                   DataRateValue (DataRate ("0b/s")),
                   MakeDataRateAccessor (&SimpleNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the device during reception",
                     MakeTraceSourceAccessor (&SimpleNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

SimpleNetDevice::SimpleNetDevice ()
  : m_channel (0),
    m_node (0),
    m_mtu (0xffff),
    m_ifIndex (0),
    m_linkUp (false)
{
  NS_LOG_FUNCTION (this);
  timeslot=13;
  m_rxPacket = 0;
  m_rxPacket_1 = 0;
  m_txPacket = 0;
  m_txPacket_1 = 0;
  nc_flag_1 = false;
  nc_flag_2 = false;
  theta = 0.1;
}

void
SimpleNetDevice::SetSid(uint16_t sid){
	m_sid=sid;
}

uint16_t
SimpleNetDevice::GetSid(){
	return m_sid;
}

void
SimpleNetDevice::SetSideAddress(Address laddress, Address raddress){
  l_address=Mac48Address::ConvertFrom (laddress);
  r_address=Mac48Address::ConvertFrom (raddress);
}

void
SimpleNetDevice::Receive (Ptr<Packet> packet, uint16_t protocol,
                          Mac48Address to, Mac48Address from)
{

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      m_phyRxDropTrace (packet);
      return;
    }

  if (to == m_address)   
    { 
      // gateway send !!!!!!!!!!1
      if(this->GetSid()==1||this->GetSid()==6){
      	NS_LOG_UNCOND("Sid"<<this->GetSid()<<"Receive");
      }
      else{
        LwsnHeader receiveHeader;
      	packet->PeekHeader(receiveHeader);
        NS_LOG_FUNCTION ("SID  : " <<this->GetSid() << "from->"  <<from << "   Osid->"<<receiveHeader.GetOsid());

        if(receiveHeader.GetType() == LwsnHeader::ORIGINAL_TRANSMISSION){
          SendSchedule(packet,to,from,protocol,receiveHeader);
        }
        else if(receiveHeader.GetType() == LwsnHeader::FORWARDING){
          SendSchedule(packet,to,from,protocol,receiveHeader);
        }
        else if(receiveHeader.GetType() == LwsnHeader::NETWORK_CODING){
          Ptr<Packet> p = decoding(packet);
          p->PeekHeader(receiveHeader);

          SendSchedule(p,to,from,protocol,receiveHeader);
        }
  		}
    }
 /* else if (to.IsBroadcast ())
    {
      packetType = NetDevice::PACKET_BROADCAST;
    }
  else if (to.IsGroup ())
    {
      packetType = NetDevice::PACKET_MULTICAST;
    }
  else 
    {
      packetType = NetDevice::PACKET_OTHERHOST;
    }

  if (packetType != NetDevice::PACKET_OTHERHOST)
    {
      m_rxCallback (this, packet, protocol, from);
    }

  if (!m_promiscCallback.IsNull ())
    {
      m_promiscCallback (this, packet, protocol, from, to, packetType);
    }
    */
}

void
SimpleNetDevice::SetRxPacket(Ptr<Packet> p)
{
  m_rxPacket = p;
}

Ptr<Packet>
SimpleNetDevice::GetRxPacket(void) const
{
  return m_rxPacket;
}
void
SimpleNetDevice::SetRxPacket_1(Ptr<Packet> p)
{
  m_rxPacket_1 = p;
}

Ptr<Packet>
SimpleNetDevice::GetRxPacket_1(void) const
{
  return m_rxPacket_1;
}

void
SimpleNetDevice::SetTxPacket(Ptr<Packet> p){
  m_txPacket = p;
}

Ptr<Packet>
SimpleNetDevice::GetTxPacket(void) const{
  return m_txPacket;
}
void
SimpleNetDevice::SetTxPacket_1(Ptr<Packet> p){
  m_txPacket_1 = p;
}

Ptr<Packet>
SimpleNetDevice::GetTxPacket_1(void) const{
  return m_txPacket_1;
}
void 
SimpleNetDevice::SetChannel (Ptr<SimpleChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  m_channel = channel;
  m_channel->Add (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

Ptr<Queue>
SimpleNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
SimpleNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
SimpleNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void 
SimpleNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t 
SimpleNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel> 
SimpleNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
SimpleNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}
Address 
SimpleNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool 
SimpleNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t 
SimpleNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool 
SimpleNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}
void 
SimpleNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
 m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool 
SimpleNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address
SimpleNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool 
SimpleNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address 
SimpleNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimpleNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool 
SimpleNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return true;
    }
  return false;
}

bool 
SimpleNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Ptr<Packet>
SimpleNetDevice::encoding(Ptr<Packet> p1,Ptr<Packet> p2)
{
  LwsnHeader temp1;
  p1 -> RemoveHeader(temp1);

  LwsnHeader temp2;
  p2 -> RemoveHeader(temp2);

  Ptr<Packet> ncpacket = Create<Packet> (100);
  LwsnHeader ncHeader;

  ncHeader.SetType(LwsnHeader::NETWORK_CODING);
  ncHeader.SetOsid(temp1.GetOsid());
  ncHeader.SetPsid(m_sid);
  ncHeader.SetR(0);
  ncHeader.SetE(1);
  //??Did??
  ncHeader.SetOsid2(temp2.GetOsid());

  ncpacket->AddHeader(ncHeader);

  NS_LOG_FUNCTION("Sid : "<<this->GetSid()<< "  packet encoding, get Osid_1 -> "<<ncHeader.GetOsid() << "Osid_2 ->"<<ncHeader.GetOsid2());

  return ncpacket;
}

Ptr<Packet>
SimpleNetDevice::decoding(Ptr<Packet> p)
{

  Ptr<Packet> t = GetTxPacket();
  LwsnHeader temp1;
  t -> PeekHeader(temp1);

  Ptr<Packet> t1 = GetTxPacket_1();
  LwsnHeader temp3;
  t1 -> PeekHeader(temp3);

  LwsnHeader temp2;
  p -> RemoveHeader(temp2);

  if(temp1.GetOsid() == temp2.GetOsid()){
    Ptr<Packet> packet = Create<Packet> (100);
    LwsnHeader sendHeader;

    sendHeader.SetType(LwsnHeader::FORWARDING);
    sendHeader.SetOsid(temp2.GetOsid2());
    sendHeader.SetPsid(m_sid);
    sendHeader.SetE(0);

    packet->AddHeader(sendHeader);
    NS_LOG_FUNCTION("Sid : "<<this->GetSid()<< "  packet decoding, get Osid -> "<<sendHeader.GetOsid());

    return packet;
  }
  else if(temp1.GetOsid() == temp2.GetOsid2()){
    Ptr<Packet> packet = Create<Packet> (100);
    LwsnHeader sendHeader;

    sendHeader.SetType(LwsnHeader::FORWARDING);
    sendHeader.SetOsid(temp2.GetOsid());
    sendHeader.SetPsid(m_sid);
    sendHeader.SetE(0);

    packet->AddHeader(sendHeader);
    NS_LOG_FUNCTION("Sid : "<<this->GetSid()<< "  packet decoding, get Osid -> "<<sendHeader.GetOsid());

    return packet;
  }
  else if(temp3.GetOsid() == temp2.GetOsid()){
    Ptr<Packet> packet = Create<Packet> (100);
    LwsnHeader sendHeader;

    sendHeader.SetType(LwsnHeader::FORWARDING);
    sendHeader.SetOsid(temp2.GetOsid2());
    sendHeader.SetPsid(m_sid);
    sendHeader.SetE(0);

    packet->AddHeader(sendHeader);
    NS_LOG_FUNCTION("Sid : "<<this->GetSid()<< "  packet decoding, get Osid -> "<<sendHeader.GetOsid());

    return packet;
  }
  else if(temp3.GetOsid() == temp2.GetOsid2()){
    Ptr<Packet> packet = Create<Packet> (100);
    LwsnHeader sendHeader;

    sendHeader.SetType(LwsnHeader::FORWARDING);
    sendHeader.SetOsid(temp2.GetOsid());
    sendHeader.SetPsid(m_sid);
    sendHeader.SetE(0);

    packet->AddHeader(sendHeader);
    NS_LOG_FUNCTION("Sid : "<<this->GetSid()<< "  packet decoding, get Osid -> "<<sendHeader.GetOsid());

    return packet;
  }

  return p;
}

void
SimpleNetDevice::NetworkCoding(Ptr<Packet> packet)
{
  Ptr<Packet> ncpacket;
  int time = Simulator::Now().GetSeconds();
  if(time % timeslot <= 6){
    ncpacket = encoding(packet,GetRxPacket());
    SetRxPacket(0);
  }
  else{
    ncpacket = encoding(packet,GetRxPacket_1());
    SetRxPacket_1(0);
  }

  NS_LOG_UNCOND("Sid : "<<this->GetSid()<< "  Network coding send");
  Mac48Address from = Mac48Address::ConvertFrom (m_address);

  Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,ncpacket,0,l_address,from);
 // NS_LOG_UNCOND("Sid : "<<this->GetSid()<< "  Network coding send1");

  Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,ncpacket,0,r_address,from);
 // NS_LOG_UNCOND("Sid : "<<this->GetSid()<< "  Network coding send2");

}

void
SimpleNetDevice::SendCheck(Ptr<Packet> packet,bool *nc_flag){


  LwsnHeader tmpHeader;
  packet->PeekHeader(tmpHeader);

  //LwsnHeader tmpHeader_1;
  //GetRxPacket()->PeekHeader(tmpHeader_1);

  NS_LOG_UNCOND("Sid : "<<this->GetSid()<< "SendCheck");

  if(nc_flag == false){
    packet->RemoveHeader(tmpHeader);
    LwsnHeader sendHeader;
    sendHeader.SetE(0);
    sendHeader.SetOsid(tmpHeader.GetOsid());
    sendHeader.SetPsid(m_sid);
    packet->AddHeader(sendHeader);
    if(tmpHeader.GetOsid() > m_sid){
      Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,0,r_address,m_address);
    }
    else{
      Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,0,l_address,m_address);
    }
  }
  else{
    NetworkCoding(packet);
    /*if(GetRxPacket_1() != 0){
      SetRxPacket(GetRxPacket_1());
      SetRxPacket_1(0);
    }*/
  }
  *nc_flag = false;
}

void
SimpleNetDevice::Forwarding(Ptr<Packet> p,Mac48Address to){
  LwsnHeader tempHeader;
  p->RemoveHeader(tempHeader);

  LwsnHeader sendHeader;
  sendHeader.SetType(LwsnHeader::FORWARDING);
  sendHeader.SetPsid(m_sid);
  sendHeader.SetOsid(tempHeader.GetOsid());
  sendHeader.SetE(0);

  p->AddHeader(sendHeader);
//  SetTxPacket(p);
  Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,0,to,m_address);

}

void
SimpleNetDevice::OriginalTransmission(Ptr<Packet> p, Mac48Address to, Mac48Address from, uint16_t protocolNumber){
  LwsnHeader sendHeader;
  sendHeader.SetType(LwsnHeader::ORIGINAL_TRANSMISSION);
  sendHeader.SetOsid(m_sid);
  sendHeader.SetPsid(m_sid);
  sendHeader.SetE(0);
  p->AddHeader(sendHeader);

  Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,0,l_address,m_address);
  Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,0,r_address,m_address);
}

void 
SimpleNetDevice::SendSchedule(Ptr<Packet> p,Mac48Address to,Mac48Address from,uint16_t protocolNumber,LwsnHeader header)
{
	
  
	switch(this->GetSid()){
		case 1:
			NS_LOG_UNCOND("Sid : "<<this->GetSid()<<"Receive"<<from);
		break;
		case 2:
			if(header.GetOsid()==1){
				//networkcoding
        nc_flag_1 = true;
        SetRxPacket_1(p);
				Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
			}
			else if(header.GetOsid()==3){
				//networkcoding
        if(nc_flag_1 == false){
				  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
        }else{
          SetRxPacket(p);
        }
			}
			else if(header.GetOsid()==4){
				Simulator::Schedule(Seconds(6.0),&SimpleNetDevice::Forwarding,this,p,l_address);
			}
			else if(header.GetOsid()==5){
				Simulator::Schedule(Seconds(5.0),&SimpleNetDevice::Forwarding,this,p,l_address);
			}
			else{
				Simulator::Schedule(Seconds(4.0),&SimpleNetDevice::Forwarding,this,p,l_address);
			}
			
		break;
		case 3:
			if(header.GetOsid()==1){
				//networkcoding
        nc_flag_2 = true;
        Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_2);
			}
			else if(header.GetOsid()==2){
				//networkcoding
        SetRxPacket_1(p);
        if(nc_flag_1 == false){
          Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
        }else{
          SetRxPacket(p);
        }
			}
			else if(header.GetOsid()==4){
				//networkcoding
        nc_flag_1 = true;
        Simulator::Schedule(Seconds(4.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
			}
			else if(header.GetOsid()==5){
				//networkcoding
        if(nc_flag_2 == false){
          Simulator::Schedule(Seconds(2.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_2);
        }else{
          SetRxPacket_1(p);
        }
			}
			else{
				Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Forwarding,this,p,l_address);
			}
		break;
		case 4:
			if(header.GetOsid()==1){
				Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::Forwarding,this,p,r_address);
			}
			else if(header.GetOsid()==2){
				//networkcoding
        if(nc_flag_2 == false){
          Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_2);
        }else{
          SetRxPacket_1(p);
        }
			}
			else if(header.GetOsid()==3){
				//networkcoding
        if(nc_flag_1 == false){
          Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
        }else{
          SetRxPacket(p);
        }
			}
			else if(header.GetOsid()==5){
				//networkcoding
        SetRxPacket_1(p);
        nc_flag_1 = true;
        Simulator::Schedule(Seconds(4.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
			}
			else{
				//networkcoding
        nc_flag_2 = true;
        Simulator::Schedule(Seconds(4.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_2);
			}
		break;
		case 5:
			if(header.GetOsid()==1){
				Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::Forwarding,this,p,r_address);
			}
			else if(header.GetOsid()==2){
				Simulator::Schedule(Seconds(4.0),&SimpleNetDevice::Forwarding,this,p,r_address);
			}
			else if(header.GetOsid()==3){
				Simulator::Schedule(Seconds(5.0),&SimpleNetDevice::Forwarding,this,p,r_address);
			}
			else if(header.GetOsid()==4){
				//networkcoding
        nc_flag_1 = true;
        Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
			}
			else{
				//networkcoding
        SetRxPacket_1(p);
        if(nc_flag_1 == false){
          Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::SendCheck,this,p,&nc_flag_1);
        }else{
          SetRxPacket(p);
        }
			}
		break;
		case 6:
      std::cout<<"sendschedule"<<std::endl;
			NS_LOG_UNCOND("Sid : "<<this->GetSid()<<"Receive"<<from);
		break;
		default:

		break;
	}
}
void
SimpleNetDevice::SetSleep(){
  if(m_queue->GetNPackets()>0)
    Ptr<Packet> packet = m_queue->Dequeue()->GetPacket ();
}
void 
SimpleNetDevice::ChannelSend(Ptr<Packet> p, uint16_t protocol, Mac48Address to, Mac48Address from){
  NS_LOG_FUNCTION ("Sid"<<this->GetSid() );
  m_channel->Send(p, protocol, to, from, this);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::SetSleep,this);
}
bool 
SimpleNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  //NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  //std::cout<<"send"<<std::endl;
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
SimpleNetDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{ 
  //std::cout<<this->GetSid()<<"sendFrom"<<std::endl;	
  //NS_LOG_FUNCTION (this << p << source << dest << protocolNumber);  
  if (p->GetSize () > GetMtu ())
    {
      return false;
    }
  Ptr<Packet> packet = p->Copy ();

  Mac48Address to = Mac48Address::ConvertFrom (dest);
  Mac48Address from = Mac48Address::ConvertFrom (source);

  SimpleTag tag;
  tag.SetSrc (from);
  tag.SetDst (to);
  tag.SetProto (protocolNumber);

  p->AddPacketTag (tag);

  if(m_queue->GetNPackets()>0){
        p->RemovePacketTag (tag);
        Simulator::Schedule(Seconds(13.0), &SimpleNetDevice::Send, this, p,dest, protocolNumber);
        return 0;
  }

  if (m_queue->Enqueue (Create<QueueItem> (p)))
    {
      if (m_queue->GetNPackets () == 1 && !TransmitCompleteEvent.IsRunning ())
        {
          p = m_queue->Dequeue ()->GetPacket ();
          p->RemovePacketTag (tag);
          Time txTime = Time (0);
          if (m_bps > DataRate (0))
            {
              txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
            }
            /*schedule*/
          int delay=Simulator::Now().GetSeconds();
    	  int slot=0;
    	  if(delay==0){
    	  	slot=0;
    	  }
    	  else{
    	  	slot=delay%13;
    	  }
    	  if(this->GetSid()==1 || this->GetSid()==4){
    	  		if(slot==1){
    	  			std::cout<<"sendfrom"<<std::endl;
    	  			Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  		else if(slot==0){
    	  			Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  		else {
    	  			delay=14-slot;
    	  			Simulator::Schedule(Seconds(delay),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  }
    	  else if(this->GetSid()==2 || this->GetSid()==5){
    	  		if(slot==2){
    	  			Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
            else if(slot==1){
              Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
            }
    	  		else if(slot==0){
    	  			Simulator::Schedule(Seconds(2.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  		else{
    	  			delay=15-slot;
    	  			Simulator::Schedule(Seconds(delay),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  }
    	  else {
    	  		if(slot==3){
    	  			Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
            else if(slot==1){
              Simulator::Schedule(Seconds(2.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
            }
            else if(slot==2){
              Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
            }
    	  		else if(slot==0){
    	  			Simulator::Schedule(Seconds(3.0),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  		else{
    	  			delay=16-slot;
    	  			Simulator::Schedule(Seconds(delay),&SimpleNetDevice::ChannelSend,this,p,protocolNumber,to,from);
    	  		}
    	  }
        SetTxPacket(p);
      }
      return true;
    }


  m_channel->Send (packet, protocolNumber, to, from, this);
  return true;
}


void
SimpleNetDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);

  if (m_queue->GetNPackets () == 0)
    {
      return;
    }

  Ptr<Packet> packet = m_queue->Dequeue ()->GetPacket ();

  SimpleTag tag;
  packet->RemovePacketTag (tag);

  Mac48Address src = tag.GetSrc ();
  Mac48Address dst = tag.GetDst ();
  uint16_t proto = tag.GetProto ();

  m_channel->Send (packet, proto, dst, src, this);

  if (m_queue->GetNPackets ())
    {
      Time txTime = Time (0);
      if (m_bps > DataRate (0))
        {
          txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
        }
      TransmitCompleteEvent = Simulator::Schedule (txTime, &SimpleNetDevice::TransmitComplete, this);
    }

  return;
}

Ptr<Node> 
SimpleNetDevice::GetNode (void) const
{
 // NS_LOG_FUNCTION (this);
  return m_node;
}
void 
SimpleNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool 
SimpleNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
void 
SimpleNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
SimpleNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_receiveErrorModel = 0;
  m_queue->DequeueAll ();
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  NetDevice::DoDispose ();
}


void
SimpleNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
SimpleNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

} // namespace ns3
