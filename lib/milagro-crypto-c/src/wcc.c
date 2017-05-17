/**
 * @file wcc.c
 * @author Mike Scott
 * @author Kealan McCusker
 * @date 28th April 2016
 * @brief AMCL Wang / Chow Choo (WCC) definitions
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
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wcc.h"

// #define DEBUG


/* Map octet string to point on curve */
static void mapit(octet *h,ECP *P)
{
    BIG q,x,c;
    BIG_fromBytes(x,h->val);
    BIG_rcopy(q,Modulus);
    BIG_mod(x,q);

    while (!ECP_setx(P,x,0))
        BIG_inc(x,1);

    BIG_rcopy(c,CURVE_Cof);
    ECP_mul(P,c);
}

/* Map to hash value to point on G2 */
static void mapit2(octet *h,ECP2 *Q)
{
    BIG q,one,Fx,Fy,x,hv;
    FP2 X;
#if CHOICE < BLS_CURVES
    ECP2 T,K;
#else
    ECP2 xQ, x2Q, x3Q, Aux1, Aux2;
#endif

    BIG_fromBytes(hv,h->val);
    BIG_rcopy(q,Modulus);
    BIG_one(one);
    BIG_mod(hv,q);

    for (;;)
    {
        FP2_from_BIGs(&X,one,hv);
        if (ECP2_setx(Q,&X)) break;
        BIG_inc(hv,1);
    }

    BIG_rcopy(Fx,CURVE_Fra);
    BIG_rcopy(Fy,CURVE_Frb);
    FP2_from_BIGs(&X,Fx,Fy);
    BIG_rcopy(x,CURVE_Bnx);

#if CHOICE < BLS_CURVES

    /* Fast Hashing to G2 - Fuentes-Castaneda, Knapp and Rodriguez-Henriquez */
    /* Q -> xQ + F(3xQ) + F(F(xQ)) + F(F(F(Q))). */
    ECP2_copy(&T,Q);
    ECP2_mul(&T,x);
    ECP2_neg(&T);   // our x is negative
    ECP2_copy(&K,&T);
    ECP2_dbl(&K);
    ECP2_add(&K,&T);
    ECP2_affine(&K);

    ECP2_frob(&K,&X);
    ECP2_frob(Q,&X);
    ECP2_frob(Q,&X);
    ECP2_frob(Q,&X);
    ECP2_add(Q,&T);
    ECP2_add(Q,&K);
    ECP2_frob(&T,&X);
    ECP2_frob(&T,&X);
    ECP2_add(Q,&T);
    ECP2_affine(Q);

#else

    /* Hashing to G2 - Scott, Benger, Charlemagne, Perez, Kachisa */
    /* Q -> 4Q+F(Q)-F(F(Q)) -xQ-F(xQ)+2F(F(xQ)) -x2Q-F(x2Q)-F(F(x2Q)) +x3Q+F(x3Q) */

    ECP2_copy(&xQ,Q);
    ECP2_copy(&Aux1,Q);
    ECP2_mul(&xQ,x);      // compute xQ
    ECP2_copy(&x2Q,&xQ);
    ECP2_mul(&x2Q,x);     // compute x2Q=x*xQ
    ECP2_copy(&x3Q,&x2Q);
    ECP2_mul(&x3Q,x);     // compute x3Q

    ECP2_sub(&x3Q,&x2Q);
    ECP2_sub(&x3Q,&xQ);
    ECP2_copy(&Aux2,&x3Q);
    ECP2_dbl(&Aux1);
    ECP2_dbl(&Aux1);
    ECP2_add(&x3Q,&Aux1);  // [x3 -x2 -x +4]Q

    ECP2_copy(&Aux1,Q);
    ECP2_add(&Aux2,&Aux1);
    ECP2_frob(&Aux2,&X);   // F([x3 -x2 -x +1]Q)

    ECP2_neg(Q);
    ECP2_dbl(&xQ);
    ECP2_add(Q,&xQ);
    ECP2_sub(Q,&x2Q);
    ECP2_frob(Q,&X);
    ECP2_frob(Q,&X);  // F(F([-x2 +2x -1]Q))

    ECP2_add(Q,&Aux2);
    ECP2_add(Q,&x3Q);

    ECP2_affine(Q);

#endif
}

/* Hash number (optional) and octet to octet */
static void hashit(int sha,int n,octet *x,octet *w)
{
    int i,c[4],hlen;
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

    hlen=sha;

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

    for (i=0; i<hlen; i++) hh[i]=0;
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

    if (hlen>=MODBYTES)
        OCT_jbytes(w,hh,MODBYTES);
    else
    {
        OCT_jbytes(w,hh,hlen);
        OCT_jbyte(w,0,MODBYTES-hlen);
    }
}

/* Perform sha256 of EC Points and Id. Map to an integer modulo the curve order.  */
void WCC_Hq(int sha, octet *A,octet *B,octet *C,octet *D,octet *h)
{
    BIG q,hs;
    // hv has to store two points in G1, One in G2 and the Id length
    char hv[2000];
    octet HV= {0,sizeof(hv),hv};
    char ht[PFS];
    octet HT= {0,sizeof(ht),ht};

    BIG_rcopy(q,CURVE_Order);

#ifdef DEBUG
    printf("WCC_Hq: A: ");
    OCT_output(A);
    printf("\n");
    printf("WCC_Hq: B: ");
    OCT_output(B);
    printf("\n");
    printf("WCC_Hq: C: ");
    OCT_output(C);
    printf("\n");
    printf("WCC_Hq: D: ");
    OCT_output(D);
    printf("\n");
#endif

    OCT_joctet(&HV,A);
    OCT_joctet(&HV,B);
    OCT_joctet(&HV,C);
    OCT_joctet(&HV,D);
    hashit(sha,0,&HV,&HT);

    BIG_fromBytes(hs,HT.val);
    BIG_mod(hs,q);
    OCT_clear(&HT);
    BIG_toBytes(h->val,hs);
    h->len=PGS;
}

/*  Calculate a value in G1. VG1 = s*H1(ID) where ID is the identity */
int WCC_GET_G1_MULTIPLE(int sha, int hashDone, octet *S,octet *ID,octet *VG1)
{
    BIG s;
    ECP P;
    char h[PFS];
    octet H= {0,sizeof(h),h};

    if (hashDone)
    {
        mapit(ID,&P);
    }
    else
    {
        hashit(sha,0,ID,&H);
        mapit(&H,&P);
    }

    BIG_fromBytes(s,S->val);
    PAIR_G1mul(&P,s);

    ECP_toOctet(VG1,&P);
    return 0;
}

/* Calculate a value in G1 used for when time permits are enabled */
int WCC_GET_G1_TPMULT(int sha, int date, octet *S,octet *ID,octet *VG1)
{
    BIG s;
    ECP P,Q;
    char h1[PFS];
    octet H1= {0,sizeof(h1),h1};
    char h2[PFS];
    octet H2= {0,sizeof(h2),h2};

    // H1(ID)
    hashit(sha,0,ID,&H1);
    mapit(&H1,&P);

    // H1(date|sha256(ID))
    hashit(sha,date,&H1,&H2);
    mapit(&H2,&Q);

    // P = P + Q
    ECP_add(&P,&Q);

    // P = s(P+Q)
    BIG_fromBytes(s,S->val);
    PAIR_G1mul(&P,s);

    ECP_toOctet(VG1,&P);
    return 0;
}

/* Calculate a value in G2 used for when time permits are enabled */
int WCC_GET_G2_TPMULT(int sha, int date, octet *S,octet *ID,octet *VG2)
{
    BIG s;
    ECP2 P,Q;
    char h1[PFS];
    octet H1= {0,sizeof(h1),h1};
    char h2[PFS];
    octet H2= {0,sizeof(h2),h2};

    // H1(ID)
    hashit(sha,0,ID,&H1);
    mapit2(&H1,&P);

    // H1(date|sha256(ID))
    hashit(sha,date,&H1,&H2);
    mapit2(&H2,&Q);

    // P = P + Q
    ECP2_add(&P,&Q);

    // P = s(P+Q)
    BIG_fromBytes(s,S->val);
    PAIR_G2mul(&P,s);

    ECP2_toOctet(VG2,&P);
    return 0;
}

/* Calculate a value in G2. VG2 = s*H2(ID) where ID is the identity */
int WCC_GET_G2_MULTIPLE(int sha, int hashDone, octet *S,octet *ID,octet *VG2)
{
    BIG s;
    ECP2 P;
    char h[PFS];
    octet H= {0,sizeof(h),h};

    if (hashDone)
    {
        mapit2(ID,&P);
    }
    else
    {
        hashit(sha,0,ID,&H);
        mapit2(&H,&P);
    }

    BIG_fromBytes(s,S->val);
    PAIR_G2mul(&P,s);

    ECP2_toOctet(VG2,&P);
    return 0;
}

/* Calculate time permit in G2 */
int WCC_GET_G2_PERMIT(int sha, int date,octet *S,octet *HID,octet *TPG2)
{
    BIG s;
    ECP2 P;
    char h[PFS];
    octet H= {0,sizeof(h),h};

    hashit(sha,date,HID,&H);
    mapit2(&H,&P);
    BIG_fromBytes(s,S->val);
    PAIR_G2mul(&P,s);

    ECP2_toOctet(TPG2,&P);
    return 0;
}

/* Calculate the sender AES Key */
int WCC_SENDER_KEY(int sha, int date, octet *xOct, octet *piaOct, octet *pibOct, octet *PbG2Oct, octet *PgG1Oct, octet *AKeyG1Oct, octet *ATPG1Oct, octet *IdBOct, octet *AESKeyOct)
{
    ECP sAG1,ATPG1,PgG1;
    ECP2 BG2,dateBG2,PbG2;
    char hv1[PFS],hv2[PFS];
    octet HV1= {0,sizeof(hv1),hv1};
    octet HV2= {0,sizeof(hv2),hv2};

    // Pairing outputs
    FP12 g;

    FP4 c;
    BIG t,x,z,pia,pib;

    char xpgg1[2*PFS+1];
    octet xPgG1Oct= {0,sizeof(xpgg1), xpgg1};

    char hv[6*PFS+1];
    octet HV= {0,sizeof(hv),hv};
    char ht[PFS];
    octet HT= {0,sizeof(ht),ht};

    BIG_fromBytes(x,xOct->val);
    BIG_fromBytes(pia,piaOct->val);
    BIG_fromBytes(pib,pibOct->val);

    if (!ECP2_fromOctet(&PbG2,PbG2Oct))
    {
#ifdef DEBUG
        printf("PbG2Oct Invalid Point: ");
        OCT_output(PbG2Oct);
        printf("\n");
#endif
        return WCC_INVALID_POINT;
    }

    if (!ECP_fromOctet(&PgG1,PgG1Oct))
    {
#ifdef DEBUG
        printf("PgG1Oct Invalid Point: ");
        OCT_output(PgG1Oct);
        printf("\n");
#endif
        return WCC_INVALID_POINT;
    }

    hashit(sha,0,IdBOct,&HV1);
    mapit2(&HV1,&BG2);

    if (!ECP_fromOctet(&sAG1,AKeyG1Oct))
    {
#ifdef DEBUG
        printf("AKeyG1Oct Invalid Point: ");
        OCT_output(AKeyG1Oct);
        printf("\n");
#endif
        return WCC_INVALID_POINT;
    }

    // Use time permits
    if (date)
    {
        // calculate e( (s*A+s*H(date|H(AID))) , (B+H(date|H(BID))) )
        if (!ECP_fromOctet(&ATPG1,ATPG1Oct))
        {
#ifdef DEBUG
            printf("ATPG1Oct Invalid Point: ");
            OCT_output(ATPG1Oct);
            printf("\n");
            return WCC_INVALID_POINT;
#endif
        }

        // H2(date|sha256(IdB))
        hashit(sha,date,&HV1,&HV2);
        mapit2(&HV2,&dateBG2);

        // sAG1 = sAG1 + ATPG1
        ECP_add(&sAG1, &ATPG1);
        // BG2 = BG2 + H(date|H(IdB))
        ECP2_add(&BG2, &dateBG2);
    }
    // z =  x + pia
    BIG_add(z,x,pia);

    // (x+pia).AKeyG1
    PAIR_G1mul(&sAG1,z);

    // pib.BG2
    PAIR_G2mul(&BG2,pib);

    // pib.BG2+PbG2
    ECP2_add(&BG2, &PbG2);

    PAIR_ate(&g,&BG2,&sAG1);
    PAIR_fexp(&g);
    // printf("WCC_SENDER_KEY e(sAG1,BG2) = ");FP12_output(&g); printf("\n");

    // x.PgG1
    PAIR_G1mul(&PgG1,x);
    ECP_toOctet(&xPgG1Oct,&PgG1);

    // Generate AES Key : K=H(k,x.PgG1)
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
    OCT_joctet(&HV,&xPgG1Oct);

    hashit(sha,0,&HV,&HT);

    OCT_empty(AESKeyOct);
    OCT_jbytes(AESKeyOct,HT.val,PAS);

    return 0;
}

/* Calculate the receiver AES key */
int WCC_RECEIVER_KEY(int sha, int date, octet *yOct, octet *wOct,  octet *piaOct, octet *pibOct,  octet *PaG1Oct, octet *PgG1Oct, octet *BKeyG2Oct,octet *BTPG2Oct,  octet *IdAOct, octet *AESKeyOct)
{
    ECP AG1,dateAG1,PgG1,PaG1;
    ECP2 sBG2,BTPG2;
    char hv1[PFS],hv2[PFS];
    octet HV1= {0,sizeof(hv1),hv1};
    octet HV2= {0,sizeof(hv2),hv2};

    // Pairing outputs
    FP12 g;

    FP4 c;
    BIG t,w,y,pia,pib;;

    char wpag1[2*PFS+1];
    octet wPaG1Oct= {0,sizeof(wpag1), wpag1};

    char hv[6*PFS+1];
    octet HV= {0,sizeof(hv),hv};
    char ht[PFS];
    octet HT= {0,sizeof(ht),ht};

    BIG_fromBytes(y,yOct->val);
    BIG_fromBytes(w,wOct->val);
    BIG_fromBytes(pia,piaOct->val);
    BIG_fromBytes(pib,pibOct->val);

    if (!ECP_fromOctet(&PaG1,PaG1Oct))
        return WCC_INVALID_POINT;

    if (!ECP_fromOctet(&PgG1,PgG1Oct))
        return WCC_INVALID_POINT;

    hashit(sha,0,IdAOct,&HV1);
    mapit(&HV1,&AG1);

    if (!ECP2_fromOctet(&sBG2,BKeyG2Oct))
        return WCC_INVALID_POINT;

    if (date)
    {
        // Calculate e( (A+H(date|H(AID))) , (s*B+s*H(date|H(IdB))) )
        if (!ECP2_fromOctet(&BTPG2,BTPG2Oct))
            return WCC_INVALID_POINT;

        // H1(date|sha256(AID))
        hashit(sha,date,&HV1,&HV2);
        mapit(&HV2,&dateAG1);

        // sBG2 = sBG2 + TPG2
        ECP2_add(&sBG2, &BTPG2);
        // AG1 = AG1 + H(date|H(AID))
        ECP_add(&AG1, &dateAG1);
    }
    // y =  y + pib
    BIG_add(y,y,pib);

    // (y+pib).BKeyG2
    PAIR_G2mul(&sBG2,y);

    // pia.AG1
    PAIR_G1mul(&AG1,pia);

    // pia.AG1+PaG1
    ECP_add(&AG1, &PaG1);

    PAIR_ate(&g,&sBG2,&AG1);
    PAIR_fexp(&g);
    // printf("WCC_RECEIVER_KEY e(AG1,sBG2) = ");FP12_output(&g); printf("\n");

    // w.PaG1
    PAIR_G1mul(&PaG1,w);
    ECP_toOctet(&wPaG1Oct,&PaG1);

    // Generate AES Key: K=H(k,w.PaG1)
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

    OCT_empty(AESKeyOct);
    OCT_jbytes(AESKeyOct,HT.val,PAS);

    return 0;

}

/* AES is run as a block cypher in the GCM  mode of operation. The key
   size is 128 bits. This function will encrypt any data length */
void WCC_AES_GCM_ENCRYPT(octet *K,octet *IV,octet *H,octet *P,octet *C,octet *T)
{
    gcm g;
    GCM_init(&g,K->len,K->val,IV->len,IV->val);
    GCM_add_header(&g,H->val,H->len);
    GCM_add_plain(&g,C->val,P->val,P->len);
    C->len=P->len;
    GCM_finish(&g,T->val);
    T->len=16;
}

/* AES is run as a block cypher in the GCM  mode of operation. The key
   size is 128 bits. This function will decrypt any data length */
void WCC_AES_GCM_DECRYPT(octet *K,octet *IV,octet *H,octet *C,octet *P,octet *T)
{
    gcm g;
    GCM_init(&g,K->len,K->val,IV->len,IV->val);
    GCM_add_header(&g,H->val,H->len);
    GCM_add_cipher(&g,P->val,C->val,C->len);
    P->len=C->len;
    GCM_finish(&g,T->val);
    T->len=16;
}

/* Get today's date as days from the epoch */
unsign32 WCC_today(void)
{
    unsign32 ti=(unsign32)time(NULL);
    return (uint32_t)(ti/(60*TIME_SLOT_MINUTES));
}

/* Performe the hash ID using sha256 */
void WCC_HASH_ID(int sha,octet *ID,octet *HID)
{
    hashit(sha,0,ID,HID);
}

/* Generate a random number modulus the group order */
int WCC_RANDOM_GENERATE(csprng *RNG,octet* S)
{
    BIG r,s;
    BIG_rcopy(r,CURVE_Order);
    BIG_randomnum(s,r,RNG);
    BIG_toBytes(S->val,s);
    S->len=PGS;
    return 0;
}

/* Calculate time permit in G2 */
int WCC_GET_G1_PERMIT(int sha, int date,octet *S,octet *HID,octet *TPG1)
{
    BIG s;
    ECP P;
    char h[PFS];
    octet H= {0,sizeof(h),h};

    hashit(sha,date,HID,&H);
    mapit(&H,&P);
    BIG_fromBytes(s,S->val);
    PAIR_G1mul(&P,s);

    ECP_toOctet(TPG1,&P);
    return 0;
}

/* Add two members from the group G1 */
int WCC_RECOMBINE_G1(octet *R1,octet *R2,octet *R)
{
    ECP P,T;
    int res=0;
    if (!ECP_fromOctet(&P,R1)) res=WCC_INVALID_POINT;
    if (!ECP_fromOctet(&T,R2)) res=WCC_INVALID_POINT;
    if (res==0)
    {
        ECP_add(&P,&T);
        ECP_toOctet(R,&P);
    }
    return res;
}

/* Add two members from the group G2 */
int WCC_RECOMBINE_G2(octet *W1,octet *W2,octet *W)
{
    ECP2 Q,T;
    int res=0;
    if (!ECP2_fromOctet(&Q,W1)) res=WCC_INVALID_POINT;
    if (!ECP2_fromOctet(&T,W2)) res=WCC_INVALID_POINT;
    if (res==0)
    {
        ECP2_add(&Q,&T);
        ECP2_toOctet(W,&Q);
    }
    return res;
}
