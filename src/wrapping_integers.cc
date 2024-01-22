#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  (void)n;
  (void)zero_point;
  return Wrap32 { ( zero_point + n % ( 1UL << 32 ) ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  uint64_t tmp = ( raw_value_ - zero_point.raw_value_ ) % ( 1UL << 32 );
  uint64_t num = checkpoint / ( 1UL << 32 );
  uint64_t tmp1 = checkpoint % ( 1UL << 32 );
  uint64_t result;
  if ( tmp1 < ( 1UL << 31 ) ) {
    if ( tmp < ( 1UL << 31 ) )
      result = num * ( 1UL << 32 ) + tmp;
    else if ( tmp - tmp1 > ( 1UL << 31 ) ) {
      if ( num == 0 )
        result = tmp;
      else
        result = ( num - 1 ) * ( 1UL << 32 ) + tmp;
    } else
      result = num * ( 1UL << 32 ) + tmp;
  } else {
    if ( tmp >= ( 1UL << 31 ) )
      result = num * ( 1UL << 32 ) + tmp;
    else if ( tmp1 - tmp > ( 1UL << 31 ) ) {
      if ( num == ( 1UL << 32 ) - 1 )
        result = num * ( 1UL << 32 ) + tmp;
      else
        result = ( num + 1 ) * ( 1UL << 32 ) + tmp;
    } else
      result = num * ( 1UL << 32 ) + tmp;
  }
  return { result };
}
