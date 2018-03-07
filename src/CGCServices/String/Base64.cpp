// Base64.cpp: implementation of the CBase64 class.
//
#include <string.h>

char Base2Chr( unsigned char n )
{
    n &= 0x3F;
    if ( n < 26 )
        return ( char )( n + 'A' );
    else if ( n < 52 )
        return ( char )( n - 26 + 'a' );
    else if ( n < 62 )
        return ( char )( n - 52 + '0' );
    else if ( n == 62 )
        return '+';
    else
        return '/';
}
//---------------------------------------------------------------------------
unsigned char Chr2Base( char c )
{
    if ( c >= 'A' && c <= 'Z' )
        return ( unsigned char )( c - 'A' );
    else if ( c >= 'a' && c <= 'z' )
        return ( unsigned char )( c - 'a' + 26 );
    else if ( c >= '0' && c <= '9' )
        return ( unsigned char )( c - '0' + 52 );
    else if ( c == '+' )
        return 62;
    else if ( c == '/' )
        return 63;
    else
        return 64;  //  ��Ч�ַ�
}


//  aLen Ϊ aSrc �ĳ��ȣ� aDest ��ָ�Ļ�������������Ϊ aLen �� 1.33 ��������
//  ���� aDest �ĳ���
unsigned int Base64Encode( char * const aDest, const unsigned char * aSrc, unsigned int aLen )
{
    char        * p = aDest;
    unsigned int          i;
    unsigned char t;

    for ( i = 0; i < aLen; i++ )
    {
        switch ( i % 3 )
        {
        case 0 :
            *p++ = Base2Chr( *aSrc >> 2 );
            t = ( *aSrc++ << 4 ) & 0x3F;
            break;
        case 1 :
            *p++ = Base2Chr( t | ( *aSrc >> 4 ) );
            t = ( *aSrc++ << 2 ) & 0x3F;
            break;
        case 2 :
            *p++ = Base2Chr( t | ( *aSrc >> 6 ) );
            *p++ = Base2Chr( *aSrc++ );
            break;
        }
    }
    if ( aLen % 3 != 0 )
    {
        *p++ = Base2Chr( t );
        if ( aLen % 3 == 1 )
            *p++ = '=';
        *p++ = '=';
    }
    *p = 0;  //  aDest is an ASCIIZ string
    return (unsigned int)( p - aDest );  //  exclude the end of zero
}

//  aDest ��ָ�Ļ�������������Ϊ aSrc ���ȵ� 0.75 ��������
//  ���� aDest �ĳ���
unsigned int Base64Decode( unsigned char * const aDest, const char * aSrc )
{
    unsigned char * p = aDest;
    unsigned int             i, n = (unsigned int)strlen( aSrc );
    unsigned char   c, t;

    for ( i = 0; i < n; i++ )
    {
        if ( *aSrc == '=' )
            break;
        do {
         if ( *aSrc )
        c = Chr2Base( *aSrc++ );
            else
             c = 65;  //  �ַ�������
        } while ( c == 64 );  //  ������Ч�ַ�����س���
        if ( c == 65 )
         break;
        switch ( i % 4 )
        {
        case 0 :
            t = c << 2;
            break;
        case 1 :
            *p++ = ( unsigned char )( t | ( c >> 4 ) );
            t = ( unsigned char )( c << 4 );
            break;
        case 2 :
            *p++ = ( unsigned char )( t | ( c >> 2 ) );
            t = ( unsigned char )( c << 6 );
            break;
        case 3 :
            *p++ = ( unsigned char )( t | c );
            break;
        }
    }
    return (unsigned int)( p - aDest );
}
