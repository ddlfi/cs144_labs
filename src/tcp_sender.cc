#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <deque>
#include <iostream>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , RTO( initial_RTO_ms )
  , ackno( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , seqno( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , outstanding_segments()
  , time_()
  , to_be_send()
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return { num_flight };
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return { num_retran };
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if ( !to_be_send.empty() ) {
    TCPSenderMessage tmp_msg = to_be_send[0];
    to_be_send.pop_front();
    cout << 1111 << " " << string_view( tmp_msg.payload ) << endl;
    return { tmp_msg };
  }
  return {};
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  if ( fin || end )
    return;
  if ( !flag ) {
    TCPSenderMessage tmp_msg;
    flag = true;
    tmp_msg.SYN = true;
    tmp_msg.seqno = isn_;
    tmp_msg.FIN = outbound_stream.is_finished();
    seqno = seqno + tmp_msg.sequence_length();
    num_flight += tmp_msg.sequence_length();
    to_be_send.push_back( tmp_msg );
    outstanding_segments.push_back( tmp_msg );
    time_.push_back( ms_past );
    return;
  }
  if ( outbound_stream.bytes_buffered() == 0 && !outbound_stream.is_finished() ) {
    return;
  }
  if ( window_size <= num_flight && window_size != 0 )
    return;
  bool special_case = 0;
  if ( window_size == 0 ) {
    if ( num_flight > 0 )
      return;
    special_case_flag = 1;
    window_size = 1;
    special_case = 1;
  }
  TCPSenderMessage tmp_msg;
  string_view str_segment = outbound_stream.peek();

  uint16_t length = min( str_segment.length(), window_size - num_flight );
  uint16_t length_ = length;
  uint16_t index = 0;
  while ( length > TCPConfig::MAX_PAYLOAD_SIZE ) {
    string_view str_tmp = str_segment.substr( index, TCPConfig::MAX_PAYLOAD_SIZE );
    index += TCPConfig::MAX_PAYLOAD_SIZE;
    tmp_msg.payload = Buffer( string( str_tmp.data(), str_tmp.size() ) );
    tmp_msg.SYN = false;
    tmp_msg.seqno = seqno;
    seqno = seqno + tmp_msg.sequence_length();
    length -= TCPConfig::MAX_PAYLOAD_SIZE;
    num_flight += TCPConfig::MAX_PAYLOAD_SIZE;
    to_be_send.push_back( tmp_msg );
    outstanding_segments.push_back( tmp_msg );
    time_.push_back( ms_past );
  }
  tmp_msg.SYN = false;
  string_view str_tmp = str_segment.substr( index, length );
  tmp_msg.payload = Buffer( string( str_tmp.data(), str_tmp.size() ) );
  outbound_stream.pop( length_ );
  tmp_msg.seqno = seqno;
  if ( window_size != num_flight + tmp_msg.payload.length() )
    tmp_msg.FIN = outbound_stream.is_finished();
  num_flight += tmp_msg.sequence_length();
  to_be_send.push_back( tmp_msg );
  outstanding_segments.push_back( tmp_msg );
  time_.push_back( ms_past );
  if ( tmp_msg.FIN )
    fin = true;
  seqno = seqno + tmp_msg.sequence_length();
  if ( special_case )
    window_size = 0;
  return;
  (void)outbound_stream;
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = seqno;
  return { msg };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if ( msg.ackno.has_value() ) {
    //获取新push进去的字节数，获取绝对序列号
    uint64_t abs_ackno_new = msg.ackno.value().unwrap( isn_, bytes_pushed + 1UL );
    uint64_t abs_ackno_old = ackno.unwrap( isn_, bytes_pushed + 1UL );
    if ( abs_ackno_new < abs_ackno_old )
      abs_ackno_new += ( 1UL << 32 );
    if ( abs_ackno_new - abs_ackno_old > num_flight )
      return;
    bytes_pushed += ( abs_ackno_new - abs_ackno_old );
    if ( abs_ackno_new > abs_ackno_old ) {
      RTO = initial_RTO_ms_;
      num_retran = 0;
      for ( uint64_t i = 0; i < time_.size(); i++ ) {
        time_[i] = ms_past;
      }
      for ( auto it = outstanding_segments.begin(); it != outstanding_segments.end(); ) {
        if ( it->seqno.unwrap( isn_, bytes_pushed + 1UL ) + it->sequence_length() - 1 < abs_ackno_new ) {
          num_flight -= it->sequence_length();
          it = outstanding_segments.erase( it );
          time_.pop_back();
        } else
          it++;
      }
    }
    ackno = msg.ackno.value();
  }
  window_size = msg.window_size;
  (void)msg;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  ms_past += ms_since_last_tick;
  cout << special_case_flag << ' ' << window_size << ' ' << ms_past << ' ' << time_[0] << ' ' << RTO << endl;
  if ( !outstanding_segments.empty() && ms_past >= time_[0] + RTO
       && ( window_size != 0 || special_case_flag ) ) { //如果最久的超时了并且window不满，重传
    num_retran += 1;
    to_be_send.push_front( outstanding_segments[0] );
    if ( !special_case_flag )
      RTO = 2 * RTO;
    time_[0] = ms_past;
  }
  (void)ms_since_last_tick;
}
