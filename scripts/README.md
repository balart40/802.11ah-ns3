This file explains the parameters of script file "s1g-mac-test.cc" and how to use it to do simulation of 802.11ah.

1) Breif introduction of file "s1g-mac-test.cc".

Users can use file "s1g-mac-test.cc" to simulate RAW feature of 802.11ah with different wifi mode. A topology in which stations are distributed randomly with distance to accsee point (AP) less than "rho" meters is bulit, after get associated with AP using fast association mechanism, all stations send udp packets to AP in their RAW slots as defined by RAW related parameters which will be explained in following section. 

2) Related Paramters

The parameters include:
  RAW related parameters:
  --NRawSta:            Number of stations supporting RAW.
  --NGroup:             Number of RAW groups.
  --NRawSlotNum:        Number of slots per RAW group.
  --SlotFormat:         Format of RAW slot count.
  --NRawSlotCount:      Used to calculate number of RAW slot duration.
    Note:  
      1. NGroup, stations are dividen evenly among each RAW groups
      2. RAW slot duration = 500 us + NRawSlotCount * 120 us, NRawSlotCount is y = 11(8) bits length when SlotFormat is set to
1(0), and NRawSlotNum is (14 - y) bits length.
  
  Wifi mode parameters:
  --DataMode:           Data mode. 
  --datarate:           Data rate of data mode.
  --bandWidth:          BandWidth of data mode.
  Other parameters:
  --SimulationTime:     Simulatiom time in seconds after all stations get associated with AP.
  --BeaconInterval:     Beacon interval time in us.
  --UdpInterval:        Traffic mode, station send one packet every UdpInterval seconds. 
  --Nsta:               Number of total stations.
  --rho:                Maximal distance between AP and stations.

  
3) Example of use

./waf --run "scratch/s1g-mac-test --NRawSta=60 --NGroup=1 --SlotFormat=1 --NRawSlotCount=412 --NRawSlotNum=2 --DataMode="OfdmRate650KbpsBW2MHz" --datarate=0.65 --bandWidth=2 --rho="100" --simulationTime=60 --UdpInterval=0.1 --Nsta=60 --rho="50" "

  

