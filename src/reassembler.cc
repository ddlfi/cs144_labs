#include "reassembler.hh"

using namespace std;

void Reassembler::handle_data_to_buffer( uint64_t first_index, string data, Writer& output )
{
  for ( uint64_t i = 0; i < data.length() && first_index + i <= bytes_pushed + output.available_capacity() - 1;
        i++ ) {
    buffer[first_index + i] = data[i];
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  if ( data.length() == 0 || output.available_capacity() == 0 ) {
    if ( is_last_substring )
      output.close();
    return;
  }
  if ( first_index > bytes_pushed + output.available_capacity() - 1 ) {
    return;
  }
  if ( first_index < bytes_pushed ) {
    if ( first_index + data.length() - 1 >= bytes_pushed ) {
      insert( bytes_pushed, data.substr( bytes_pushed - first_index ), is_last_substring, output );
    }
    return;
  }
  if ( first_index != bytes_pushed ) {
    if ( is_last_substring ) {
      last_index = first_index + data.length() - 1;
    }
    handle_data_to_buffer( first_index, data, output );
  } else {
    output.push( data );
    for ( uint64_t i = bytes_pushed; i < output.bytes_pushed(); i++ ) {
      if ( buffer.find( i ) != buffer.end() )
        buffer.erase( i );
    }
    bytes_pushed = output.bytes_pushed();
    if ( is_last_substring ) {
      output.close();
      return;
    }
    while ( buffer.find( bytes_pushed ) != buffer.end() && !output.is_closed() && output.available_capacity() ) {
      output.push( string( 1, buffer[bytes_pushed] ) );
      buffer.erase( bytes_pushed );
      if ( bytes_pushed == last_index ) {
        output.close();
      }
      bytes_pushed = output.bytes_pushed();
    }
  }
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  (void)output;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return { buffer.size() };
}
