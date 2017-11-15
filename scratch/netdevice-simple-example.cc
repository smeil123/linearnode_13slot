#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mac48-address.h"
#include <cstdlib>
#include <iostream>

using namespace ns3;

int main (int argc, char *argv[])
{
  NS_LOG_UNCOND("Start");

  LogComponentEnableAll(LOG_PREFIX_TIME);
  LogComponentEnableAll(LOG_PREFIX_FUNC);
  LogComponentEnable("SimpleNetDevice",LOG_LEVEL_ALL);
  LogComponentEnable("SimpleChannel",LOG_LEVEL_ALL);
  NodeContainer nodes;
  nodes.Create (6);

  Ptr<SimpleNetDevice> dev1;
  dev1 = CreateObject<SimpleNetDevice> ();
  dev1->SetAddress(Mac48Address("00:00:00:00:00:01"));
  nodes.Get(0)->AddDevice (dev1);

  Ptr<SimpleNetDevice> dev2;
  dev2 = CreateObject<SimpleNetDevice> ();
  dev2->SetAddress(Mac48Address("00:00:00:00:00:02"));
  nodes.Get(1)->AddDevice (dev2);

  Ptr<SimpleNetDevice> dev3;
  dev3 = CreateObject<SimpleNetDevice> ();
  dev3->SetAddress(Mac48Address("00:00:00:00:00:03"));
  nodes.Get(2)->AddDevice (dev3);

  Ptr<SimpleNetDevice> dev4;
  dev4 = CreateObject<SimpleNetDevice> ();
  dev4->SetAddress(Mac48Address("00:00:00:00:00:04"));
  nodes.Get(3)->AddDevice (dev4);

  Ptr<SimpleNetDevice> dev5;
  dev5 = CreateObject<SimpleNetDevice> ();
  dev5->SetAddress(Mac48Address("00:00:00:00:00:05"));
  nodes.Get(4)->AddDevice (dev5);

  Ptr<SimpleNetDevice> dev6;
  dev6 = CreateObject<SimpleNetDevice> ();
  dev6->SetAddress(Mac48Address("00:00:00:00:00:06"));
  nodes.Get(5)->AddDevice (dev6);


  Ptr<SimpleChannel> channel = CreateObject<SimpleChannel> ();
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);
  dev3->SetChannel (channel);
  dev4->SetChannel (channel);
  dev5->SetChannel (channel);
  dev6->SetChannel (channel);


  dev1->SetNode (nodes.Get (0));
  dev2->SetNode (nodes.Get (1));
  dev3->SetNode (nodes.Get (2));
  dev4->SetNode (nodes.Get (3));
  dev5->SetNode (nodes.Get (4));
  dev6->SetNode (nodes.Get (5));

  dev1->SetSid(1);
  dev2->SetSid(2);
  dev3->SetSid(3);  
  dev4->SetSid(4);
  dev5->SetSid(5);
  dev6->SetSid(6);


  dev1->SetSideAddress(dev1->GetAddress(),dev2->GetAddress());
  dev2->SetSideAddress(dev1->GetAddress(),dev3->GetAddress());
  dev3->SetSideAddress(dev2->GetAddress(),dev4->GetAddress());
  dev4->SetSideAddress(dev3->GetAddress(),dev5->GetAddress());
  dev5->SetSideAddress(dev4->GetAddress(),dev6->GetAddress());
  dev6->SetSideAddress(dev5->GetAddress(),dev6->GetAddress());

  Ptr<Packet> packet = Create<Packet> (100);
  LwsnHeader sourceHeader;
  sourceHeader.SetOsid(1);
  sourceHeader.SetE(0);
  sourceHeader.SetR(Simulator::Now().GetSeconds());
  packet->AddHeader(sourceHeader);
  packet->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev1,packet,dev2->GetAddress(),0);


  Ptr<Packet> p = Create<Packet> (100);
  LwsnHeader sourceHeader_1;
  sourceHeader_1.SetOsid(2);
  sourceHeader_1.SetE(0);
  sourceHeader_1.SetR(2);
  p->AddHeader(sourceHeader_1);
  p->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev2,p,dev3->GetAddress(),0);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev2,p,dev1->GetAddress(),0);

  Ptr<Packet> p1 = Create<Packet> (100);
  LwsnHeader sourceHeader_2;
  sourceHeader_2.SetOsid(3);
  sourceHeader_2.SetE(0);
  sourceHeader_2.SetR(2);
  p1->AddHeader(sourceHeader_2);
  p1->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev3,p1,dev2->GetAddress(),0);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev3,p1,dev4->GetAddress(),0);

  Ptr<Packet> p2 = Create<Packet> (100);
  LwsnHeader sourceHeader_3;
  sourceHeader_3.SetOsid(4);
  sourceHeader_3.SetE(0);
  sourceHeader_3.SetR(2);
  p2->AddHeader(sourceHeader_3);
  p2->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev4,p2,dev3->GetAddress(),0);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev4,p2,dev5->GetAddress(),0);

  Ptr<Packet> p3 = Create<Packet> (100);
  LwsnHeader sourceHeader_4;
  sourceHeader_4.SetOsid(5);
  sourceHeader_4.SetE(0);
  sourceHeader_4.SetR(2);
  p3->AddHeader(sourceHeader_4);
  p3->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev5,p3,dev4->GetAddress(),0);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev5,p3,dev6->GetAddress(),0);

  Ptr<Packet> p4 = Create<Packet> (100);
  LwsnHeader sourceHeader_5;
  sourceHeader_5.SetOsid(6);
  sourceHeader_5.SetE(0);
  sourceHeader_5.SetR(2);
  p4->AddHeader(sourceHeader_5);
  p4->Print(std::cout);
  Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::Send,dev6,p4,dev5->GetAddress(),0);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
