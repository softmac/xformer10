/*
 * encrypt.c
 *
 * Modified encryption library that uses a credit card
 * number and expiry instead of a string version of the BIOS
 * serial number and hard disk serial number to represent the
 * locking key.
 *
 * (c) 1997-1999 Solution Tech Systems Inc.
 * Used with permission.
 */

#ifndef __ENCRYPT_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encrypt.h"
#endif

const unsigned long raw_keys[4][8] = {
	/* Emulator */
	{ 0x76543245, 0x17426d11, 0x37752116, 0x6c227110,
	  0x00400461, 0x9aab74c0, 0xff6ffeff, 0x72616263 },

	/* Soltech! */
	{ 0x1574fd53, 0x87aa6f5f, 0x626c1f10, 0x74353637,
	  0xeeffaa65, 0x51116351, 0x9068002f, 0x21feefef },

	/* UsaCanad */
	{ 0x41ab6c55, 0xd1e2731a, 0x71615424, 0x43463215,
	  0x45114861, 0xabb56eac, 0x74615314, 0x64520421 },

	/* FredFlin */
	{ 0x741aee46, 0xabb77274, 0xab65abba, 0x64464aaa,
	  0x61626346, 0x41626c44, 0x61693334, 0x6e216263 }
};


void __inline Conv3To4(unsigned char *p4, unsigned char *p3)
{
    unsigned char t1 =  (p3[1] >> 2);
    unsigned char t2 = ((p3[1] << 4) | (p3[2] >> 4)) & 0x3F;
    unsigned char t3 = ((p3[2] << 2) | (p3[0] >> 6)) & 0x3F;
    unsigned char t4 =   p3[0]                       & 0x3F;

    *p4++ = (t1 < 10) ? ('0' + t1) : ((t1 < 37) ? ('@' - 10 + t1) : ('a' - 37 + t1));
    *p4++ = (t2 < 10) ? ('0' + t2) : ((t2 < 37) ? ('@' - 10 + t2) : ('a' - 37 + t2));
    *p4++ = (t3 < 10) ? ('0' + t3) : ((t3 < 37) ? ('@' - 10 + t3) : ('a' - 37 + t3));
    *p4++ = (t4 < 10) ? ('0' + t4) : ((t4 < 37) ? ('@' - 10 + t4) : ('a' - 37 + t4));
}

void __inline Conv4To3(unsigned char *p4, unsigned char *p3)
{
    unsigned char t1 = (p4[0] >= 'a') ? (p4[0] - 'a' + 37) : ((p4[0] >= '@') ? (p4[0] - '@' + 10) : (p4[0] - '0'));
    unsigned char t2 = (p4[1] >= 'a') ? (p4[1] - 'a' + 37) : ((p4[1] >= '@') ? (p4[1] - '@' + 10) : (p4[1] - '0'));
    unsigned char t3 = (p4[2] >= 'a') ? (p4[2] - 'a' + 37) : ((p4[2] >= '@') ? (p4[2] - '@' + 10) : (p4[2] - '0'));
    unsigned char t4 = (p4[3] >= 'a') ? (p4[3] - 'a' + 37) : ((p4[3] >= '@') ? (p4[3] - '@' + 10) : (p4[3] - '0'));

    p3[1] = (t1 << 2) | (t2 >> 4);
    p3[2] = (t2 << 4) | ((t3 >> 2) & 15);
    p3[0] = (t3 << 6) | t4;
}

/*
 * int _STS_CreateLockKey (_STS_ENCRYPT_STRUCT *p);
 *
 * This function takes a key type, and two additional char
 * strings as character sequences to build up an encoded locking
 * string, to be given to STS to convert into an unlock key. Each
 * user of the software will be able to generate a unique key based on
 * the two char fields passed in (for example, a string representation
 * of a BIOS serial number and string representation of a hard disk
 * serial number), to lock the software to a given system. The only way
 * the software can be unlocked is to supply a matching "unlock" code,
 * which matches the code created here.
 *
 * returns: _STS_ERROR to indicate an error, _STS_SUCCESS indicates
 * that the key string is present in the key field of the
 * incoming data structure
 */

int
_STS_CreateLockKey (_STS_ENCRYPT_STRUCT *p)
{
	int x, y, z, a, b;
	unsigned char c;
	unsigned char t = 0;
	char tmp[32];
	unsigned char k[128];

	if (p == NULL)
		return _STS_ERROR;

	/*
	 * ensure all fields are 32 bytes or less
	 */

	p->field1[32] = '\0';
	p->field2[32] = '\0';

	for (x = 0; x < strlen (p->field1); x++)
		t += (unsigned char)p->field1[x];
	for (x = 0; x < strlen (p->field2); x++)
		t += (unsigned char)p->field2[x];

	/*
	 * checksum, and lengths of 1st/2nd field
	 */

	sprintf (tmp, "%02X%02X%02X", (unsigned int)t,
		(unsigned int)strlen (p->field1), (unsigned int)strlen (p->field2));

	/*
	 * encrypt checksum/lengths first
	 */

	p->keytype %= 4;
	z = p->keytype;

	for (y = 0, x = 0; x < strlen (tmp); x++) {
		a = x % 8;
		b = (x % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		k[y++] = (unsigned char)tmp[x] ^ c;
	}	/* end for */

	/*
	 * encrypt 1st field
	 */

	z++;
	z %= 4;
	for (x = 0; x < strlen (p->field1); x++) {
		a = x % 8;
		b = (x % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		k[y++] = (unsigned char)p->field1[x] ^ c;
	}	/* end for */

	/*
	 * encrypt 2nd field
	 */

	z++;
	z %= 4;
	for (x = 0; x < strlen (p->field2); x++) {
		a = x % 8;
		b = (x % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		k[y++] = (unsigned char)p->field2[x] ^ c;
	}	/* end for */

	/*
	 * now format an ASCII representation of this sequence
	 * by converting groups of 3 characters into 4
	 */
  
// printf("encoding '%s'\n", k);

	for (x = 0; x < y; x+=3)
        {
        Conv3To4(p->key + 1 + x * 4 / 3, &k[x]);
// printf("3To4\n");
        }

// printf("encoded to '%s'\n", p->key+1);

	p->key[0] = 'W' + p->keytype;
	p->key[(y+2)/3 * 4 + 1] = '\0';

	return _STS_SUCCESS;
}	/* end _STS_CreateLockKey */


/*
 * int _STS_UnlockKey (char *key, _STS_ENCRYPT_STRUCT *p);
 *
 * This function takes the key and decrypts the data to
 * generate the original data. The checksum and lengths must work out.
 * If the key is OK, then a result of _STS_SUCCESS
 * value is returned. Else, an appropriate error code is generated.
 */

int
_STS_UnlockKey (char *key, _STS_ENCRYPT_STRUCT *p)
{
	int x, y, z;
	int a, b;
	int l1, l2;
	unsigned char t, t1;
	unsigned char c;
	unsigned char k[128];
	char tmp[3];

	if (key == NULL)
		return _STS_ERROR;

	if (p == NULL)
		return _STS_ERROR - 1;

	strcpy (p->key, key);

	/*
	 * 1st char is the raw encoding string type - subtract
	 * 'W' from this byte to get which string was used. get
	 * the unencoded string
	 */

	if ((key[0] < 'W') || (key[0] > 'Z'))
		return _STS_ERROR - 2;

	z = key[0] - 'W';

	p->keytype = z;

// printf("decoding '%s'\n", key);

	for (y = 0, x = 1; x < strlen (key); ) {
        Conv4To3(&key[x], &k[y]);
// printf("4To3\n");
		x+=4;
		y+=3;
	}	/* end for */

// printf("decoded to '%s'\n", k);

	/*
	 * 1st 6 bytes are the checksum, 2 lengths
	 * decode with raw key z
	 */

	for (x = 0; x < 6; x++) {
		a = x % 8;
		b = (x % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		k[x] = (unsigned char)k[x] ^ c;
	}	/* end for */

	/*
	 * decode the checksum, lengths
	 */

	tmp[0] = k[0];
	tmp[1] = k[1];
	tmp[2] = '\0';
	t = strtol (tmp, NULL, 16);

	tmp[0] = k[2];
	tmp[1] = k[3];
	tmp[2] = '\0';
	l1 = strtol (tmp, NULL, 16);
	if (l1 > 32)
		return _STS_ERROR - 3;

	tmp[0] = k[4];
	tmp[1] = k[5];
	tmp[2] = '\0';
	l2 = strtol (tmp, NULL, 16);
	if (l2 > 32)
		return _STS_ERROR - 4;

// printf("checkpoint 261\n");

// printf("l1 = %d, l2 = %d, y = %d\n", l1, l2, y);

	/*
	 * check if the lengths match the original
	 * string length (minus the 1st byte
	 */

#if 0
	if ((6 + l1 + l2) != y)
		return _STS_ERROR - 5;
#endif

// printf("checkpoint 271\n");

	/*
	 * decode 1st string
	 */

	z++;
	z %= 4;
	for (y = 0, x = 6; y < l1; y++, x++) {
		a = y % 8;
		b = (y % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		p->field1[y] = (unsigned char)k[x] ^ c;
	}	/* end for */
	p->field1[y] = '\0';

	/*
	 * decode 2nd string
	 */

	z++;
	z %= 4;
	for (y = 0, x = 6 + l1; y < l2; y++, x++) {
		a = y % 8;
		b = (y % 4) * 8;
		c = (unsigned char)(raw_keys[z][a] >> b);
		p->field2[y] = (unsigned char)k[x] ^ c;
	}	/* end for */
	p->field2[y] = '\0';

// printf("checkpoint 301\n");

	/*
	 * do a checksum on the fields
	 */

	t1 = 0;
	for (x = 0; x < strlen (p->field1); x++)
		t1 += (unsigned char)p->field1[x];
	for (x = 0; x < strlen (p->field2); x++)
		t1 += (unsigned char)p->field2[x];

	if (t != t1)
		return _STS_ERROR - 6;

// printf("checkpoint 316\n");

	/*
	 * key is OK!
	 */

	return _STS_SUCCESS;
}	/* end _STS_UnlockKey */



#ifdef __TEST_API__

void
main (void)
{
	_STS_ENCRYPT_STRUCT e;
	int rc;
	char buf[128];

	printf ("Card (0 = Visa, 1 = MC, 2 = Amex, 3 = Diners): ");
	gets (buf);
	e.keytype = atoi (buf);
	if (e.keytype < 0)
		e.keytype = 0;
	else if (e.keytype > 3)
		e.keytype = 3;

	printf ("Enter your credit card number: ");
	gets (buf);
	//strcpy (buf, "4412123456781234");
	//printf ("%s\n", buf);
	buf[sizeof (e.field1) - 1] = '\0';
	strcpy (e.field1, buf);

	while (1) {
		printf ("Enter your credit card expiry date (MM/YY): ");
		gets (buf);
		if ((strlen (buf) == 5) && (buf[2] == '/'))
			break;
	}	/* endif */
	strcpy (e.field2, buf);

	if (_STS_CreateLockKey (&e) != _STS_SUCCESS) {
		printf ("Error: cannot create a key!\n");
		exit (0);
	}	/* endif */

	printf ("\n\nKey is: %s\n\n", e.key);

	strcpy (buf, e.key);

	memset (&e, 0, sizeof (e));

	printf ("\n\nunlocking key: ");
	rc = _STS_UnlockKey (buf, &e);
	printf ("%d\n\n", rc);
}	/* end main */

#endif

