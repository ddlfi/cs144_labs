#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) {
    ISN = Wrap32( message.seqno.unwrap( Wrap32( 0UL ), 0ULL ) );
    has_ISN = true;
    if ( message.FIN )
      has_FIN = true;
    reassembler.insert( ( message.seqno + 1 ).unwrap( ISN, inbound_stream.bytes_pushed() + 1UL ) - 1UL,
                        message.payload,
                        message.FIN,
                        inbound_stream );
  } else if ( has_ISN ) {
    if ( message.FIN )
      has_FIN = true;
    reassembler.insert( message.seqno.unwrap( ISN, inbound_stream.bytes_pushed() + 1UL ) - 1UL,
                        message.payload,
                        message.FIN,
                        inbound_stream );
  }
  (void)message;
  (void)reassembler;
  (void)inbound_stream;
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg;

  Wrap32 ackno( 1UL );
  if ( inbound_stream.available_capacity() < ( 1UL << 16 ) ) {
    msg.window_size = inbound_stream.available_capacity();
  } else {
    msg.window_size = ( 1UL << 16 ) - 1;
  }

  if ( !has_ISN ) {
    return { msg };
  }
  if ( inbound_stream.is_closed() && has_FIN ) {
    msg.ackno = ackno.wrap( inbound_stream.bytes_pushed() + 2UL, ISN );
  } else {
    msg.ackno = ackno.wrap( inbound_stream.bytes_pushed() + 1UL, ISN );
  }
  (void)inbound_stream;
  return { msg };
}
