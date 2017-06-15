/**
 * @file sok.c
 * @author Mike Scott and Kealan McCusker and Alessandro Budroni
 * @date 28th April 2016
 * @brief Sakai, Oghishi and Kasahara (SOK) definitions
 *
 *
 * LICENSE
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/* SOK Functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sok.h"

/* Hash number (optional) and octet to octet */
static void hashit(int sha, int n, octet *x, octet *w)
{
    int i,c[4];
    hash256 sha256;
    hash512 sha512;
    char hh[64];

    switch (sha)
    {
    case SHA256:
        HASH256_init(&sha256);
        break;
    case SHA384:
        HASH384_init(&sha512);
        break;
    case SHA512:
        HASH512_init(&sha512);
        break;
    }

    if (n>0)
    {
        c[0]=(n>>24)&0xff;
        c[1]=(n>>16)&0xff;
        c[2]=(n>>8)&0xff;
        c[3]=(n)&0xff;
        for (i=0; i<4; i++)
        {
            switch(sha)
            {
            case SHA256:
                HASH256_process(&sha256,c[i]);
                break;
            case SHA384:
                HASH384_process(&sha512,c[i]);
                break;
            case SHA512:
                HASH512_process(&sha512,c[i]);
                break;
            }
        }
    }
    if (x!=NULL) for (i=0; i<x->len; i++)
        {
            switch(sha)
            {
            case SHA256:
                HASH256_process(&sha256,x->val[i]);
                break;
            case SHA384:
                HASH384_process(&sha512,x->val[i]);
                break;
            case SHA512:
                HASH512_process(&sha512,x->val[i]);
                break;
            }
        }

    for (i=0; i<sha; i++) hh[i]=0;
    switch (sha)
    {
    case SHA256:
        HASH256_hash(&sha256,hh);
        break;
    case SHA384:
        HASH384_hash(&sha512,hh);
        break;
    case SHA512:
        HASH512_hash(&sha512,hh);
        break;
    }

    OCT_empty(w);

    if (sha>=MODBYTES)
        OCT_jbytes(w,hh,MODBYTES);
    else
    {
        OCT_jbytes(w,hh,sha);
        OCT_jbyte(w,0,MODBYTES-sha);
    }
}

/* Calculate AES Key; f_key( e( (s*A+s*H(date|H(AID))) , (B+H(date|H(BID))) )) */
int SOK_PAIR1(int sha, int date, octet *AKeyG1, octet *AG1TP, octet *BID, octet *KEY)
{
    ECP sAG1,TPG1;
    ECP2 BG2,dateBG2;
    char h1[MODBYTES],h2[MODBYTES];
    octet H1= {0,sizeof(h1),h1};
    octet H2= {0,sizeof(h2),h2};

    // Pairing outputs
    FP12 g;

    // Key generation
    FP4 c;
    BIG t;
    char hv[6*PFS+1];
    octet HV= {0,sizeof(hv),hv};
    char ht[PFS];
    octet HT= {0,sizeof(ht),ht};

    hashit(sha,0,BID,&H1);
    ECP2_mapit(&H1,&BG2);

    if (!ECP_fromOctet(&sAG1,AKeyG1))
        return SOK_INVALID_POINT;

    // Use time permits
    if (date)
    {
        if (!ECP_fromOctet(&TPG1,AG1TP))
            return SOK_INVALID_POINT;

        // H2(date|sha256(BID))
        hashit(sha,date,&H1,&H2);
        ECP2_mapit(&H2,&dateBG2);

        // sAG1 = sAG1 + TPG1
        ECP_add(&sAG1, &TPG1);
        // BG2 = BG2 + H(date|H(BID))
        ECP2_add(&BG2, &dateBG2);
    }

    PAIR_ate(&g,&BG2,&sAG1);
    PAIR_fexp(&g);
    //printf("SOK_PAIR1 e(sAG1,BG2) = ");FP12_output(&g); //printf("\n");

    // Generate AES Key
	char wpag1[2*PFS+1];
    octet wPaG1Oct= {0,sizeof(wpag1), wpag1};

    FP12_trace(&c,&g);

    HV.len = 4*PFS;
    BIG_copy(t,c.a.a);
    FP_redc(t);
    BIG_toBytes(&(HV.val[0]),t);

    BIG_copy(t,c.a.b);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS]),t);

    BIG_copy(t,c.b.a);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS*2]),t);

    BIG_copy(t,c.b.b);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS*3]),t);

    // Set HV.len to correct value
    OCT_joctet(&HV,&wPaG1Oct);

    hashit(sha,0,&HV,&HT);

    OCT_empty(KEY);
    OCT_jbytes(KEY,HT.val,PAS);

    return SOK_OK;
}

/* Calculate AES Key; f_key ( e( (A+H(date|H(AID))) , (s*B+s*H(date|H(BID))) )) */
int SOK_PAIR2(int sha, int date, octet *BKeyG2, octet *BG2TP, octet *AID, octet *KEY)
{
    ECP2 sBG2,TPG2;
    ECP AG1,dateAG1;
    char h1[MODBYTES],h2[MODBYTES];
    octet H1= {0,sizeof(h1),h1};
    octet H2= {0,sizeof(h2),h2};

    // Pairing outputs
    FP12 g;

    // Key generation
    FP4 c;
    BIG t;
    char hv[6*PFS+1];
    octet HV= {0,sizeof(hv),hv};
    char ht[PFS];
    octet HT= {0,sizeof(ht),ht};

    hashit(sha,0,AID,&H1);
    ECP_mapit(&H1,&AG1);

    if (!ECP2_fromOctet(&sBG2,BKeyG2))
        return SOK_INVALID_POINT;

    // Use time permits
    if (date)
    {
        if (!ECP2_fromOctet(&TPG2,BG2TP))
            return SOK_INVALID_POINT;

        // H1(date|sha256(AID))
        hashit(sha,date,&H1,&H2);
        ECP_mapit(&H2,&dateAG1);

        // sBG2 = sBG2 + TPG2
        ECP2_add(&sBG2, &TPG2);
        // AG1 = AG1 + H(date|H(AID))
        ECP_add(&AG1, &dateAG1);
    }

    PAIR_ate(&g,&sBG2,&AG1);
    PAIR_fexp(&g);
    //printf("SOK_PAIR2 e(AG1,sBG2) = ");FP12_output(&g); //printf("\n");

    // Generate AES Key
	char wpag1[2*PFS+1];
    octet wPaG1Oct= {0,sizeof(wpag1), wpag1};

    FP12_trace(&c,&g);

    HV.len = 4*PFS;
    BIG_copy(t,c.a.a);
    FP_redc(t);
    BIG_toBytes(&(HV.val[0]),t);

    BIG_copy(t,c.a.b);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS]),t);

    BIG_copy(t,c.b.a);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS*2]),t);

    BIG_copy(t,c.b.b);
    FP_redc(t);
    BIG_toBytes(&(HV.val[PFS*3]),t);

    // Set HV.len to correct value
    OCT_joctet(&HV,&wPaG1Oct);

    hashit(sha,0,&HV,&HT);

    OCT_empty(KEY);
    OCT_jbytes(KEY,HT.val,PAS);

    return SOK_OK;

}

/* AES-GCM Encryption of octets, K is key, H is header,
   P is plaintext, C is ciphertext, T is authentication tag */
void SOK_AES_GCM_ENCRYPT(octet *K, octet *IV, octet *H, octet *P, octet *C, octet *T)
{
    gcm g;
    GCM_init(&g,K->len,K->val,IV->len,IV->val);
    GCM_add_header(&g,H->val,H->len);
    GCM_add_plain(&g,C->val,P->val,P->len);
    C->len=P->len;
    GCM_finish(&g,T->val);
    T->len=16;
}

/* AES-GCM Decryption of octets, K is key, H is header,
   P is plaintext, C is ciphertext, T is authentication tag */
void SOK_AES_GCM_DECRYPT(octet *K, octet *IV, octet *H, octet *C, octet *P, octet *T)
{
    gcm g;
    GCM_init(&g,K->len,K->val,IV->len,IV->val);
    GCM_add_header(&g,H->val,H->len);
    GCM_add_cipher(&g,P->val,C->val,C->len);
    P->len=C->len;
    GCM_finish(&g,T->val);
    T->len=16;
}

/* return time in slots since epoch */
unsign32 SOK_today(void)
{
    unsign32 ti=(unsign32)time(NULL);
    return (long)(ti/(60*TIME_SLOT_MINUTES));
}

/* Initialise a Cryptographically Strong Random Number Generator from
   an octet of raw random data */

void SOK_HASH_ID(int sha, octet *ID, octet *HID)
{
    hashit(sha,0,ID,HID);
}

/* create random secret S */
void SOK_RANDOM_GENERATE(csprng *RNG, octet* S)
{
    BIG r,s;

    BIG_rcopy(r,CURVE_Order);
    BIG_randomnum(s,r,RNG);
#ifdef AES_S
    BIG_mod2m(s,2*AES_S);
#endif
    BIG_toBytes(S->val,s);
    S->len=MODBYTES;
}

/* G1 left hand secret LHS=s*H1(CID) where HCID is client ID hash and s is
   the master secret */
void SOK_GET_G1_SECRET(octet *S, octet *HCID, octet *CST)
{
    BIG s;
    ECP P;

    ECP_mapit(HCID,&P);

    BIG_fromBytes(s,S->val);
    PAIR_G1mul(&P,s);

    ECP_toOctet(CST,&P);
}

/* G2 right hand secret RHS = s*H2(CID) where HCID is client ID hash and s is the
   master secret */
void SOK_GET_G2_SECRET(octet *S, octet *HCID, octet *PK)
{
    BIG s;
    ECP2 P;

    ECP2_mapit(HCID,&P);
    BIG_fromBytes(s,S->val);
    PAIR_G2mul(&P,s);

    ECP2_toOctet(PK,&P);
}

/* G1 left hand time permit LHTP=s*H1(date|H(CID)) where HCID is client ID hash
   and s is the master secret */
void SOK_GET_G1_PERMIT(int sha, int date, octet *S, octet *HCID, octet *CTT)
{
    BIG s;
    ECP P;
    char h[MODBYTES];
    octet H= {0,sizeof(h),h};

    hashit(sha,date,HCID,&H);
    ECP_mapit(&H,&P);
    BIG_fromBytes(s,S->val);
    PAIR_G1mul(&P,s);

    ECP_toOctet(CTT,&P);
}

/* G2 right hand time termit RHTP=s*H2(date|H(CID)) where CID is client ID hash
   and s is the master secret */
void SOK_GET_G2_PERMIT(int sha, int date, octet *S, octet *HCID, octet *TP)
{
    BIG s;
    ECP2 P;
    char h[MODBYTES];
    octet H= {0,sizeof(h),h};

    hashit(sha,date,HCID,&H);
    ECP2_mapit(&H,&P);
    BIG_fromBytes(s,S->val);
    PAIR_G2mul(&P,s);

    ECP2_toOctet(TP,&P);
}

/* R=R1+R2 in group G1 */
int SOK_RECOMBINE_G1(octet *R1, octet *R2, octet *R)
{
    ECP P,T;
    int res=0;
    if (!ECP_fromOctet(&P,R1)) res=SOK_INVALID_POINT;
    if (!ECP_fromOctet(&T,R2)) res=SOK_INVALID_POINT;
    if (res==0)
    {
        ECP_add(&P,&T);
        ECP_toOctet(R,&P);
    }
    return res;
}

/* W=W1+W2 in group G2 */
int SOK_RECOMBINE_G2(octet *W1,octet *W2,octet *W)
{
    ECP2 Q,T;
    int res=0;
    if (!ECP2_fromOctet(&Q,W1)) res=SOK_INVALID_POINT;
    if (!ECP2_fromOctet(&T,W2)) res=SOK_INVALID_POINT;
    if (res==0)
    {
        ECP2_add(&Q,&T);
        ECP2_toOctet(W,&Q);
    }
    return res;
}
