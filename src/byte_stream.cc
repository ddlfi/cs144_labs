#include <stdexcept>

#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) ,stream(){}

void Writer::push( string data )
{
  // Your code here.
  uint64_t num = min(data.size(),capacity_-occupied_capacity);
  stream.insert(stream.size(),data,0,num);
  pushed += num;
  occupied_capacity += num;
  (void)data;
}

void Writer::close()
{
  // Your code here.
  closed = true;
}

void Writer::set_error()
{
  // Your code here.
  _error = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return {closed};
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {capacity_-occupied_capacity};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {pushed};
}

string_view Reader::peek() const {
  string_view s(stream);
  return s;
}

bool Reader::is_finished() const
{
  // Your code here.
  return {closed && occupied_capacity==0};
}

bool Reader::has_error() const
{
  // Your code here.
  return {_error};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t num = min(len,stream.size());
  stream.erase(0,num);
  poped += num;
  occupied_capacity -= num;
  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {occupied_capacity};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {poped};
}
