#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  
  Route route_;
  route_.route_prefix = route_prefix;
  route_.prefix_length = prefix_length;
  route_.next_hop = next_hop;
  route_.interface_num = interface_num;
  route_table.push_back(route_);

  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;
}

void Router::route() {
  optional<InternetDatagram> datagram_;
  for(uint32_t i=0;i<interfaces_.size();i++){
    datagram_ = interfaces_[i].maybe_receive();
    if(datagram_.has_value()){
      InternetDatagram datagram = *datagram_;
      send_(datagram);
    }
  }
}

void Router::send_(InternetDatagram datagram){
  size_t index = route_table.size();
  uint8_t num = 0;
  uint32_t dst = datagram.header.dst;
  for(size_t i=0;i<route_table.size();i++){
    uint32_t xor_num;
    xor_num = route_table[i].route_prefix ^ dst;
    if(xor_num < (1ul<<(32-route_table[i].prefix_length)) && route_table[i].prefix_length >= num){
      index = i;
      num = route_table[i].prefix_length;
    }
  }
  if(datagram.header.ttl <= 1 || index == route_table.size()) return;
  datagram.header.ttl--;
  datagram.header.compute_checksum();

  if(route_table[index].next_hop.has_value()) interfaces_[route_table[index].interface_num].send_datagram(datagram,*route_table[index].next_hop);
  else{
    interfaces_[route_table[index].interface_num].send_datagram(datagram,Address::from_ipv4_numeric(dst));
  }
}
