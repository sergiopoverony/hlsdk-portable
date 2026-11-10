/*
minmax.h - min, max and clamp for the original CZDS code
The original was built with MSVC, where min and max accept mixed argument
types, e.g. min( float, int ). std::min doesn't, so C++ gets templates that
return the common type of both arguments.
*/
#pragma once
#if !defined( MINMAX_H )
#define MINMAX_H

#if defined( __cplusplus )
#undef min
#undef max

template <class A, class B>
inline auto min( const A &a, const B &b ) -> decltype( a < b ? a : b )
{
	return a < b ? a : b;
}

template <class A, class B>
inline auto max( const A &a, const B &b ) -> decltype( a > b ? a : b )
{
	return a > b ? a : b;
}
#else // !__cplusplus
#if !defined( min )
#define min( a, b )	((( a ) < ( b )) ? ( a ) : ( b ))
#endif

#if !defined( max )
#define max( a, b )	((( a ) > ( b )) ? ( a ) : ( b ))
#endif
#endif // !__cplusplus

#if !defined( clamp )
#define clamp( val, lo, hi )	((( val ) > ( hi )) ? ( hi ) : ((( val ) < ( lo )) ? ( lo ) : ( val )))
#endif

#endif // MINMAX_H
