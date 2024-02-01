#include "network_interface.hh"
#include <queue>
#include <unordered_map>
#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address ), map(), ip_Address_list(), time_(), ethernetframe_queue(), ipdatagram(),
  ip_address_queue(), arp_()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame ethernetframe;
  //没有目标的Ethernet地址
  if(map.find(next_hop.ipv4_numeric()) == map.end()){
    if(arp_.find(next_hop.ipv4_numeric()) == arp_.end()){
      ethernetframe.header.type = ethernetframe.header.TYPE_ARP;
      ethernetframe.header.src = ethernet_address_;
      ethernetframe.header.dst = ETHERNET_BROADCAST;
      ARPMessage msg;
      msg.opcode = msg.OPCODE_REQUEST;
      msg.sender_ethernet_address = ethernet_address_;
      msg.sender_ip_address = ip_address_.ipv4_numeric();
      msg.target_ip_address = next_hop.ipv4_numeric();
      ethernetframe.payload = serialize(msg);
      arp_[next_hop.ipv4_numeric()] = ms_past;
      ethernetframe_queue.push_back(ethernetframe);
    }
    ipdatagram.push_back(dgram);
    ip_address_queue.push_back(next_hop);
  }
  else{
    ethernetframe.header.type = ethernetframe.header.TYPE_IPv4;
    ethernetframe.header.src = ethernet_address_;
    ethernetframe.header.dst = map[next_hop.ipv4_numeric()];
    ethernetframe.payload =  serialize(dgram);
    ethernetframe_queue.push_back(ethernetframe);
  }
  (void)dgram;
  (void)next_hop;
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.type == frame.header.TYPE_IPv4 && frame.header.dst != ethernet_address_){
    return {};
  }
  else if(frame.header.type == frame.header.TYPE_ARP){
    ARPMessage msg;
    if(parse(msg,frame.payload)){
      if(map.find(msg.sender_ip_address)== map.end()){
        map[msg.sender_ip_address] = msg.sender_ethernet_address;
        ip_Address_list.push_back(Address::from_ipv4_numeric(msg.sender_ip_address));
        time_.push_back(ms_past);
      }
      if(arp_.find(msg.sender_ip_address)!=arp_.end()){
        arp_.erase(msg.sender_ip_address);  
      }
      if(msg.opcode == msg.OPCODE_REPLY){
        if(map.find(ip_address_queue[0].ipv4_numeric()) != map.end()){
          send_datagram(ipdatagram[0],ip_address_queue[0]);
          ipdatagram.pop_front();
          ip_address_queue.pop_front();
          return {};
        }
      }
      if(msg.opcode == msg.OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric()){
        EthernetFrame ethernetframe;
        ethernetframe.header.type = ethernetframe.header.TYPE_ARP;
        ethernetframe.header.src = ethernet_address_;
        ethernetframe.header.dst = frame.header.src;
        ARPMessage msg_;
        msg_.opcode = msg.OPCODE_REPLY;
        msg_.sender_ip_address = ip_address_.ipv4_numeric();
        msg_.sender_ethernet_address = ethernet_address_;
        msg_.target_ethernet_address = frame.header.src;
        msg_.target_ip_address = msg.sender_ip_address;
        ethernetframe.payload = serialize(msg_);
        ethernetframe_queue.push_back(ethernetframe);
      }
    }

  }
  else{
    InternetDatagram result;
    if(parse(result,frame.payload)){
      return result;
    }
  }
  (void)frame;
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  ms_past += ms_since_last_tick;
  for(auto iter = arp_.begin();iter!=arp_.end();){
    if(ms_past - iter->second > 5000) iter = arp_.erase(iter);
    else iter++;
  }
  while(!time_.empty() && ms_past - time_[0] > 30000){
    time_.pop_front();
    map.erase(ip_Address_list[0].ipv4_numeric());
    ip_Address_list.pop_front();
  }
  (void)ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(!ethernetframe_queue.empty()) {
    EthernetFrame result = ethernetframe_queue[0];
    ethernetframe_queue.pop_front();
    return {result};
  }
  return {};
}
