/* \brief
 *		hashed string
 */


#ifndef __HASH_STRING_H__
#define __HASH_STRING_H__


const char* hs_hashstr(const char* str);

/* 
  len: count of bytes, including '\0' 
*/
const char* hs_hashnstr(const char* str, unsigned int len);

/*
  len: count of bytes, excluding '\0'
*/
const char* hs_hashnstr2(const char* str, unsigned int len);

#endif /* __HASH_STRING_H__ */
