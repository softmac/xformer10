/*
 * encrypt.h
 *
 * Header file for modified encryption library.
 *
 * (c) 1997-1999 Solution Tech Systems Inc.
 * Used with permission.
 */


#ifndef __ENCRYPT_H__

#pragma pack(push, 1)

typedef struct tag_STS_ENCRYPT_STRUCT
{
	int keytype;
	char field1[33];
	char valid1[1];
	char field2[33];
	char valid2[1];
	char key[200];
} _STS_ENCRYPT_STRUCT;

#pragma pack(pop)

#define _STS_ERROR				-1
#define _STS_SUCCESS			0

int _STS_CreateLockKey (_STS_ENCRYPT_STRUCT *);
int _STS_UnlockKey (char *, _STS_ENCRYPT_STRUCT *);


#define __ENCRYPT_H__

#endif


