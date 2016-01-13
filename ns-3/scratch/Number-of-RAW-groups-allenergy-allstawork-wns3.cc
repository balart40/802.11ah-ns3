/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 *          Sebastien Deronne <sebastien.deronne@gmail.com>
 */

//set traffic interval randomly for each station
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include <fstream>
#include <sys/stat.h>

/*
 * station randomly distributed, phy included
 */


//This is a simple example of an IEEE 802.11n Wi-Fi network.
//
//Network topology:
//
//  Wifi 192.168.1.0
//
//         AP
//    *    *
//    |    |
//    n1   n2
//
//Packets in this simulation aren't marked with a QosTag so they are considered
//belonging to BestEffort Access Class (AC_BE).

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("s1g-wifi-network-le");

uint32_t AssocNum = 0;
int64_t AssocTime=0;
uint32_t StaNum=0;
string TrafficInterval="10000";

class assoc_record
{
public:
    assoc_record ();
    bool GetAssoc ();
    void SetAssoc (std::string context, Mac48Address address);
    void UnsetAssoc (std::string context, Mac48Address address);
    void setstaid (uint16_t id);
 private:
    bool assoc;
    uint16_t staid;
};

assoc_record::assoc_record ()
{
    assoc = false;
    staid = 65535;
}

void
assoc_record::setstaid (uint16_t id)
{
    staid = id;
}

void
assoc_record::SetAssoc (std::string context, Mac48Address address)
{
   assoc = true;
   NS_LOG_UNCOND("assocation, sta ID = " <<  staid << ",time = " << Simulator::Now ().GetMicroSeconds () );
}

void
assoc_record::UnsetAssoc (std::string context, Mac48Address address)
{
    assoc = false;
    NS_LOG_UNCOND("assocation lost, sta ID = " <<  staid << ",time = " << Simulator::Now ().GetMicroSeconds () );
}

bool
assoc_record::GetAssoc ()
{
    return assoc;
}

typedef std::vector <assoc_record * > assoc_recordVector;
assoc_recordVector assoc_vector;


uint32_t
GetAssocNum ( )
{
  AssocNum = 0;
  for (assoc_recordVector::const_iterator index = assoc_vector.begin (); index != assoc_vector.end (); index++)
    {
        if ((*index)->GetAssoc ())
        {
            AssocNum++;
        }
    }
  NS_LOG_UNCOND("AssocNum=" << AssocNum << ", time = " << Simulator::Now ().GetMicroSeconds () );
    return AssocNum;
}


class Access_record
{
public:
    Access_record ();
    void RecordProcess ( double , double );
    float GetSum ();
    string name;
private:
    int interval, curr_column1, pre_column1, curr_column2, pre_column2;
    float sum;
};

Access_record::Access_record ()
{
    interval=0;
    curr_column1=0;
    pre_column1=0;
    curr_column2=0;
    pre_column2=0;
    sum=0;
}

float
Access_record::GetSum ()
{
    return sum;
}

void
Access_record::RecordProcess (double time, double data)
{
    curr_column1 = time;
    curr_column2 = data;
    interval=0;
            
    if (pre_column2 == 66)
       {
          switch (curr_column2)
                {
                    case 99:
                    case 77:
                        interval = curr_column1 - pre_column1;
                        break;
                    default:
                        interval = 0;
                        break;
                }
        }
            
    if (pre_column2 == 10 || pre_column2 == 20 || pre_column2 == 30)
        {
           switch (curr_column2)
             {
                    case 99:
                    case 77:
                        interval = curr_column1 - pre_column1;
                        break;
                    case 66:
                        //interval = (int)(curr_column1 - pre_column1) % 100000;
                        interval = curr_column1 - pre_column1;
                        break;
                    default:
                        interval = curr_column1 - pre_column1;
                        break;
            }
        }
            
    if (pre_column2 == 99)
        {
                switch (curr_column2)
                {
                    case 10:
                    case 20:
                    case 30:
                    case 77:
                        interval = curr_column1 - pre_column1;
                        break;
                    case 66:
                        //nterval = (int)(curr_column1 - pre_column1) % 100000;
                        interval = curr_column1 - pre_column1;
                        break;
                    default:
                        interval = 0;
                        break;
                }
        }
            
      sum = sum + interval;
      pre_column1 = curr_column1;
      pre_column2 = curr_column2;
}




typedef std::vector <Access_record * > Access_recordVector;
Access_recordVector m_vector;




void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr)
{
    std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
}

void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble)
{
    std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;
}



void
UdpTraffic ( string Interval, uint16_t num)
{
  TrafficInterval = Interval;
  StaNum=num;
}

ApplicationContainer serverApp;
typedef std::vector <Ptr<WifiMacQueue> > QueueVector;
QueueVector m_queue;
QueueVector BEqueue;

uint32_t NPacketInQueue = 0;
//Ptr<WifiMacQueue> BEqueue;

uint32_t AppStartTime = 0;
uint32_t ApStopTime = 0;

void ReadQueue(QueueVector queuelist)
{
    BEqueue = queuelist;
    //NS_LOG_UNCOND("there are still " << BEqueue->GetSize () << " packets in the queue, " << Simulator::Now ().GetMicroSeconds ());
}

uint16_t NEmptyQueue=0;
uint16_t kk=0;

void GetQueuelen (uint32_t Nsta)
{
   NEmptyQueue=0;
   kk=0;
   for (QueueVector::const_iterator i = BEqueue.begin (); i != BEqueue.end (); i++)
     {
      kk +=1;
      if ((*i)->GetSize () != 0)
        {
         NS_LOG_UNCOND(kk <<"th station, " << (*i)->GetSize () << " packets remains, " << Simulator::Now ().GetSeconds ());
        }
   
      if ((*i)->GetSize () == 0)
         {
           NEmptyQueue +=1 ;
         }
      }
    
    if (NEmptyQueue == Nsta)
       {
         Simulator::Stop (Seconds (0));
         ApStopTime = Simulator::Now ().GetSeconds ();
          NS_LOG_UNCOND("no packets in the queue, " << Simulator::Now ().GetSeconds ());
          NS_LOG_UNCOND("Simulator start at " << AppStartTime << " s, end at " << ApStopTime << " s" );
        }
    else
       {
         Simulator::Schedule(Seconds (1.0), &GetQueuelen, Nsta);
         NS_LOG_UNCOND("there are still packets in the queue" );
       }
 }


void CheckAssoc (uint32_t Nsta, double simulationTime, NodeContainer wifiApNode, NodeContainer  wifiStaNode, Ipv4InterfaceContainer apNodeInterface)
{
    if  (GetAssocNum () == Nsta)
      {
        NS_LOG_UNCOND("Assoc Finished, AssocNum=" << AssocNum << ", time = " << Simulator::Now ().GetMicroSeconds () );
        //Application start time
        double randomInterval = std::stod (TrafficInterval,nullptr);
        Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
        //UDP flow
        UdpServerHelper myServer (9);
        serverApp = myServer.Install (wifiApNode);
        serverApp.Start (Seconds (0));
        //serverApp.Stop (Seconds (simulationTime+1)); // serverApp stops when simulator stop
        //Set traffic start time for each station
        
        UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9); //address of remote node
        myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
        myClient.SetAttribute ("Interval", TimeValue (Time (TrafficInterval))); //packets/s
        myClient.SetAttribute ("PacketSize", UintegerValue (256));
        for (uint16_t i = 0; i < Nsta; i++)
          {
            double randomStart = m_rv->GetValue (0, randomInterval);
            NS_LOG_UNCOND("Generate traffic in " << randomStart << "seconds, station " << double(i));
            
            ApplicationContainer clientApp = myClient.Install (wifiStaNode.Get(i));
            clientApp.Start (Seconds (1 + randomStart));
            clientApp.Stop (Seconds (simulationTime+1));
          }
        AppStartTime=Simulator::Now ().GetSeconds () + 1; //clientApp start time
        Simulator::Schedule(Seconds (simulationTime+1), &GetQueuelen, Nsta); //check before simulation stop
        //Simulator::Stop (Seconds (31)); //set stop time until no packet in queue
      }
    else
      {
        Simulator::Schedule(Seconds(1.0), &CheckAssoc, Nsta, simulationTime, wifiApNode, wifiStaNode, apNodeInterface);
      }
    
}


void
PopulateArpCache ()
{
    Ptr<ArpCache> arp = CreateObject<ArpCache> ();
    arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=
            interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            NS_ASSERT(ipIface != 0);
            Ptr<NetDevice> device = ipIface->GetDevice();
            NS_ASSERT(device != 0);
            Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
            for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++)
            {
                Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
                if(ipAddr == Ipv4Address::GetLoopback())
                    continue;
                ArpCache::Entry * entry = arp->Add(ipAddr);
                entry->MarkWaitReply(0);
                entry->MarkAlive(addr);
                NS_LOG_UNCOND ("Arp Cache: Adding the pair (" << addr << "," << ipAddr << ")");
            }
        }
    }
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            ipIface->SetAttribute("ArpCache", PointerValue(arp));
        }
    }
}


int main (int argc, char *argv[])
{
  LogComponentEnable ("UdpServer", LOG_INFO);
   // LogComponentEnable ("ApWifiMac", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("StaWifiMac", LOG_LEVEL_FUNCTION);
  
  bool udp = true;
  uint32_t seed = 1;
  double simulationTime = 10; //seconds
  uint32_t Nsta =1;
  uint32_t NRawSta = 1;
  uint32_t SlotFormat=1;
  uint32_t  NRawSlotCount = 829;
  uint32_t  NRawSlotNum = 1;
  uint32_t NGroup = 1;
  uint32_t NGroupStas;
  uint32_t BeaconInterval = 102400;
  uint32_t RawDuration = 102400;
  bool OutputPosition = true;
  string DataMode = "OfdmRate2_4MbpsBW1MHz";
  double datarate = 2.4;
  double bandWidth = 1;
  string UdpInterval="0.2";
  string APpositon="1000.0";
  string rho="250.0";
  double APpos=1000.0;
  string folder = "./TestMacresult";
  string file = "./TestMacresult/mactest.txt";
  string pcapfile = "./TestMacresult/mactest";
  string energyfile = "./TestMacresult/power";

  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.AddValue ("Nsta", "number of total stations", Nsta);
  cmd.AddValue ("NRawSta", "number of stations supporting RAW", NRawSta);
  cmd.AddValue ("SlotFormat", "format of NRawSlotCount", SlotFormat);
  cmd.AddValue ("NRawSlotCount", "number of stations supporting RAW", NRawSlotCount);
  cmd.AddValue ("NRawSlotNum", "number of stations supporting RAW", NRawSlotNum);
  cmd.AddValue ("NGroup", "number of RAW group", NGroup);
  cmd.AddValue ("BeaconInterval", "Beacon interval time in us", BeaconInterval);
  cmd.AddValue ("RawDuration", "Duration of one RAW in us", RawDuration);
  cmd.AddValue ("DataMode", "Date mode", DataMode);
  cmd.AddValue ("datarate", "data rate in Mbps", datarate);
  cmd.AddValue ("bandWidth", "bandwidth in MHz", bandWidth);
  cmd.AddValue ("UdpInterval", "traffic mode", UdpInterval);
  cmd.AddValue ("APpositon", "Ap node location", APpositon);
  cmd.AddValue ("rho", "maximal distance between AP and stations", rho);
  cmd.AddValue ("APpos", "Ap node location", APpos);
  cmd.AddValue ("folder", "folder where result files are placed", folder);
  cmd.AddValue ("file", "files containing reslut information", file);
  cmd.AddValue ("pcapfile", "files containing reslut information", pcapfile);
  cmd.AddValue ("energyfile", "files containing reslut energy information", energyfile);
  cmd.Parse (argc,argv);
    
  RngSeedManager::SetSeed (seed);

    // start time
    time_t rawtime;
    struct tm * timeinfo;
    
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    printf ("Current local time and date: %s", asctime(timeinfo));
    //calculate execuate time
    
      ofstream myfile;
      myfile.open (energyfile, ios::out | ios::trunc);
      myfile.close();

      NGroupStas = NRawSta/NGroup;
      uint32_t payloadSize; //1500 byte IP packet
      if (udp)
        {
          payloadSize = 256; //bytes
          //payloadSize = 500; //bytes
        }
      else
        {
          payloadSize = 1448; //bytes
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
        }

      NodeContainer wifiStaNode;
      wifiStaNode.Create (Nsta);
      NodeContainer wifiApNode;
      wifiApNode.Create (1);

      YansWifiChannelHelper channel = YansWifiChannelHelper ();
      channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent", DoubleValue(3.76) ,"ReferenceLoss", DoubleValue(8.0), "ReferenceDistance", DoubleValue(1.0));
      //channel.AddPropagationLoss ("ns3::FixedRssLossModel", "Rss", DoubleValue (0)); //fixed received signal
      channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        
      YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
      phy.SetErrorRateModel ("ns3::YansErrorRateModel");
      phy.SetChannel (channel.Create ());
      
      phy.Set ("ShortGuardEnabled", BooleanValue (false));
      phy.Set ("ChannelWidth", UintegerValue (bandWidth));  //&&&&&&&&
    
    phy.Set ("EnergyDetectionThreshold", DoubleValue (-116.0));
    phy.Set ("CcaMode1Threshold", DoubleValue (-119.0));
    phy.Set ("TxGain", DoubleValue (0.0));
    phy.Set ("RxGain", DoubleValue (0.0));
    phy.Set ("TxPowerLevels", UintegerValue (1));
    phy.Set ("TxPowerEnd", DoubleValue (0.0));
    phy.Set ("TxPowerStart", DoubleValue (0.0));
    phy.Set ("RxNoiseFigure", DoubleValue (3.0));
    phy.Set ("LdpcEnabled", BooleanValue (true));

      WifiHelper wifi = WifiHelper::Default ();
      wifi.SetStandard (WIFI_PHY_STANDARD_80211ah);
      S1gWifiMacHelper mac = S1gWifiMacHelper::Default ();

      Ssid ssid = Ssid ("ns380211ah");
      StringValue DataRate;
      DataRate = StringValue (DataMode);
 

      wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate,
                                    "ControlMode", DataRate);
      mac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false),
                   "RawDuration", TimeValue (MicroSeconds (RawDuration)));
    
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(60000));
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));
    
    
      NetDeviceContainer staDevice;
      staDevice = wifi.Install (phy, mac, wifiStaNode);

    mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid),
                 "BeaconInterval", TimeValue (MicroSeconds(BeaconInterval)),
                 "NRawGroupStas", UintegerValue (NGroupStas),
                 "NRawStations", UintegerValue (NRawSta),
                 "SlotFormat", UintegerValue (SlotFormat),
                 "SlotCrossBoundary", UintegerValue (1),
                 "SlotDurationCount", UintegerValue (NRawSlotCount),
                 "SlotNum", UintegerValue (NRawSlotNum));
    
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(60000));
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));
    
    Simulator::ScheduleNow(&UdpTraffic, UdpInterval, Nsta);


      NetDeviceContainer apDevice;
    phy.Set ("TxGain", DoubleValue (3.0));
    phy.Set ("RxGain", DoubleValue (3.0));
    phy.Set ("TxPowerLevels", UintegerValue (1));
    phy.Set ("TxPowerEnd", DoubleValue (30.0));
    phy.Set ("TxPowerStart", DoubleValue (30.0));
    phy.Set ("RxNoiseFigure", DoubleValue (5));
      apDevice = wifi.Install (phy, mac, wifiApNode);

      // mobility.
      /*MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                       "X", StringValue (APpositon),
                                       "Y", StringValue (APpositon),
                                       "rho", StringValue (rho));
        
     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobility.Install(wifiStaNode);

      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        
      positionAlloc->Add (Vector (APpos, APpos, 0.0));
      mobilityAp.SetPositionAllocator (positionAlloc);

      mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install(wifiApNode);*/
    
      //read attributes

    
   for (uint16_t i = 0; i < Nsta; i++)
    {
      Ptr<NetDevice> netDe;
      Ptr<WifiNetDevice> wifiDe;
      netDe = staDevice.Get(i);
      wifiDe = netDe->GetObject<WifiNetDevice>();   // This was the secret sauce to getting access to that mac object
      PointerValue ptr1;
      wifiDe->GetMac()->GetAttribute("BE_EdcaTxopN", ptr1);
      Ptr<EdcaTxopN> BE_edca = ptr1.Get<EdcaTxopN>();
      PointerValue ptr2;
      BE_edca->GetAttribute("Queue", ptr2);
      Ptr<WifiMacQueue> BE_edca_queue = ptr2.Get<WifiMacQueue> ();
      m_queue.push_back (BE_edca_queue);
    }
    
    
     Simulator::Schedule(Seconds(1), &ReadQueue, m_queue);
    
    
    
      MobilityHelper mobilityAp;
      Ptr<ListPositionAllocator> positionAllocAp = CreateObject<ListPositionAllocator> ();
      positionAllocAp->Add (Vector (0.0, 0.0, 0.0));
      mobilityAp.SetPositionAllocator (positionAllocAp);
      mobilityAp.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobilityAp.Install (wifiApNode);
    
      float deltaAngle = 2* M_PI / Nsta;
      float angle = 0.0;
      double x = 0.0;
      double y = 0.0;
      double Distance = 450.0;
    
      for (int i = 0; i < Nsta; i++)
       {
          x = cos(angle) * Distance;
          y = sin(angle) * Distance;
          NS_LOG_UNCOND ("x=" << x << ", " << "y = " << y);
        
          MobilityHelper mobilitySta;
          Ptr<ListPositionAllocator> positionAllocSta = CreateObject<ListPositionAllocator> ();
          positionAllocSta->Add(Vector(x, y, 0.0));
          mobilitySta.SetPositionAllocator(positionAllocSta);
          mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
          mobilitySta.Install(wifiStaNode.Get(i));
          angle += deltaAngle;
        }


      /* Internet stack*/
      InternetStackHelper stack;
      stack.Install (wifiApNode);
      stack.Install (wifiStaNode);

      Ipv4AddressHelper address;

      address.SetBase ("192.168.0.0", "255.255.0.0");
      Ipv4InterfaceContainer staNodeInterface;
      Ipv4InterfaceContainer apNodeInterface;

      staNodeInterface = address.Assign (staDevice);
      apNodeInterface = address.Assign (apDevice);

      /* Setting applications */
      //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc", MakeCallback (&StaAssoc));
    
    for (uint16_t kk=0; kk< Nsta; kk++)
    {
        std::ostringstream STA;
        STA << kk;
        std::string strSTA = STA.str();
        
        assoc_record *m_assocrecord=new assoc_record;
        m_assocrecord->setstaid (kk);
        Config::Connect ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc", MakeCallback (&assoc_record::SetAssoc, m_assocrecord));
        Config::Connect ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc", MakeCallback (&assoc_record::UnsetAssoc, m_assocrecord));
        assoc_vector.push_back (m_assocrecord);
    }
    
    
    for (uint16_t kk=0; kk< Nsta; kk++)
    {
        std::ostringstream STA;
        STA << kk;
        std::string strSTA = STA.str();
        
        Access_record *m_record=new Access_record;
        Config::ConnectWithoutContext ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/AccessQuest_record", MakeCallback (&Access_record::RecordProcess, m_record));
        m_vector.push_back (m_record);
        
    }

    
    //////////
    
    Simulator::Schedule(Seconds(1), &CheckAssoc, Nsta, simulationTime, wifiApNode, wifiStaNode,apNodeInterface);
    
    

       Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
       PopulateArpCache ();
    
      if  (OutputPosition)
        {
          int i =0;
          while (i < Nsta)
            {
               Ptr<MobilityModel> mobility = wifiStaNode.Get (i)->GetObject<MobilityModel>();
               Vector position = mobility->GetPosition();
               NS_LOG_UNCOND ("Sta node#" << i << ", " << "position = " << position);
               i++;
            }
          Ptr<MobilityModel> mobility1 = wifiApNode.Get (0)->GetObject<MobilityModel>();
          Vector position = mobility1->GetPosition();
          NS_LOG_UNCOND ("AP node, position = " << position);
        }
    
      phy.EnablePcap ("pcapfile", Nsta, 0);
      Simulator::Run ();
      Simulator::Destroy ();
    
    for (Access_recordVector::const_iterator index = m_vector.begin (); index != m_vector.end (); index++)
     {
        Access_record * recordEnergyPointer=new Access_record;
        recordEnergyPointer=(*index);
         
         myfile.open (energyfile, ios::out | ios::app);
         myfile << recordEnergyPointer->GetSum() <<"\n";
         myfile.close();
     }

      double throughput = 0;
      if (udp)
        {
          //UDP
          uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
          //throughput = totalPacketsThrough * payloadSize * 8 / (simulationTime * 1000000.0); //Mbit/s
          throughput = totalPacketsThrough * payloadSize * 8 / ((ApStopTime - AppStartTime) * 1000000.0); //Mbit/s
        }

      std::cout << "DataRate" << "\t" << "Throughput" << '\n';
      std::cout << datarate << "\t\t" << throughput << " Mbit/s" << std::endl;
       
      myfile.open (file, ios::out | ios::app);
      myfile << NRawSlotNum <<  "\t\t" << throughput <<"\n";
      myfile.close();
    
    // end time
    
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    printf ("Current local time and date: %s", asctime(timeinfo));
    // calculate execuate time
return 0;
}











