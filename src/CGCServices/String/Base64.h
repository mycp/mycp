// Base64.h file here
//
//////////////////////////////////////////////////////////////////////

#ifndef __Base64_h__
#define __Base64_h__

//  aLen Ϊ aSrc �ĳ��ȣ� aDest ��ָ�Ļ�������������Ϊ aLen �� 1.33 ��������
//  ���� aDest �ĳ���
unsigned int Base64Encode( char * const aDest, const unsigned char * aSrc, unsigned int aLen );

//  aDest ��ָ�Ļ�������������Ϊ aSrc ���ȵ� 0.75 ��������
//  ���� aDest �ĳ���
unsigned int Base64Decode( unsigned char * const aDest, const char * aSrc );

#endif // __Base64_h__
