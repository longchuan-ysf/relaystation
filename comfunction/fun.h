/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   fun.h
作者:   龙川
说明:   常见函数的实现
***************************************************************************************************/
#ifndef __FUN_H_
#define __FUN_H_
#include "com.h"

#define BCD_TO_HEX(n)  ((n>>4)*10 +(n&0xf))
#define HEX_TO_BCD(n)  (((n/10)<<4) + (n%10))


extern void printf_Hexbuffer(uint8_t *buffer,uint32_t len);
extern unsigned  char CheckSum(const unsigned  char *buffer,unsigned  int bufferLen);
extern uint8_t DigitalToString(uint32_t Digital,uint8_t *String);

extern int str_len(char *_str);
extern void str_cpy(char *_tar, char *_src);
extern int str_cmp(char * s1, char * s2);
extern void mem_set(char *_tar, char _data, int _len);

extern void int_to_str(int _iNumber, char *_pBuf, unsigned char _len);
extern int str_to_int(char *_pStr,unsigned char len);

extern uint16_t BEBufToUint16(uint8_t *_pBuf);
extern uint16_t LEBufToUint16(uint8_t *_pBuf);

extern uint32_t BEBufToUint32(uint8_t *_pBuf);
extern uint32_t LEBufToUint32(uint8_t *_pBuf);

extern uint16_t CRC16_Modbus(uint8_t *_pBuf, uint16_t _usLen) ;
extern int32_t  CaculTwoPoint(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x);

extern char BcdToChar(uint8_t _bcd);
extern void HexToAscll(uint8_t * _pHex, char *_pAscii, uint16_t _BinBytes);
extern uint32_t AsciiToUint32(char *pAscii);
#endif







