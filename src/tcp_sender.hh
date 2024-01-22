#pragma once
#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <deque>
using namespace std;
class TCPSender
{
  Wrap32 isn_;
  uint64_t ms_past = 0;
  uint64_t initial_RTO_ms_;
  uint64_t RTO;
  uint64_t num_retran = 0;
  uint64_t bytes_pushed = 0;
  uint64_t num_flight = 0;
  Wrap32 ackno;
  Wrap32 seqno;
  bool flag = false; // if has got receiver_message
  bool fin_with_data = false;
  bool end = false;
  bool special_case_flag = false;
  uint16_t window_size = 1;
  deque<TCPSenderMessage> outstanding_segments;
  deque<uint64_t> time_;
  deque<TCPSenderMessage> to_be_send;
  bool fin = false;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
