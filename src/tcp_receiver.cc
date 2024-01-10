#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) {
    ISN = Wrap32( message.seqno.unwrap( Wrap32( 0UL ), 0ULL ) );
    has_ISN = true;
    reassembler.insert( ( message.seqno + 1 ).unwrap( ISN, inbound_stream.bytes_pushed() + 1UL ) - 1UL,
                        message.payload,
                        message.FIN,
                        inbound_stream );
  } else if ( has_ISN ) {
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
  Wrap32 ackno( 1UL );
  TCPReceiverMessage tcpreceivermessage( ackno.wrap( inbound_stream.bytes_pushed() + 1UL, ISN ),
                                         inbound_stream.available_capacity() );
  (void)inbound_stream;
  return { tcpreceivermessage };
}
