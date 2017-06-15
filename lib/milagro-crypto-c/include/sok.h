/**
 * @file sok.h
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

/* AMCL Sakai, Oghishi and Kasahara (SOK) definitions */

#ifndef SOK_H
#define SOK_H

#include "amcl.h"

/* Field size is assumed to be greater than or equal to group size */

#define PGS MODBYTES  /**< SOK Group Size */
#define PFS MODBYTES  /**< SOK Field Size */
#define PAS 16  /**< AES Symmetric Key Size */

#define SOK_OK                     0     /**< return code */
#define SOK_INVALID_POINT         -14    /**< return error code */

#define TIME_SLOT_MINUTES 1440 /**< Time Slot = 1 day */

/**	@brief Create random secret S
 *
  	@param RNG is a pointer to a cryptographically secure random number generator
    @param S is the output random octet
 */
void SOK_RANDOM_GENERATE(csprng *RNG, octet* S);

/**	@brief G1 left hand secret
 *
    @param S is an input master secret
  	@param HCID is client ID hash
  	@param CST = S*H1(HCID)
 */
void SOK_GET_G1_SECRET(octet *S, octet *HCID, octet *CST);

/**	@brief G1 left hand Time Permit
 *
    @param sha: integer representing the bit-length of the sha: 32, 48, 64
    @param date as input in days since the epoch
  	@param S is an input master secret
  	@param HCID is client ID hash
  	@param TP is the Time Permit given as output TP=s*H1(date|H(CID))
 */
void SOK_GET_G1_PERMIT(int sha, int date, octet *S, octet *HCID, octet *TP);


/**	@brief G2 right hand secret
 *
    @param S is an input master secret
  	@param HCID is client ID hash
  	@param PK = S*H2(HCID)
 */
void SOK_GET_G2_SECRET(octet *S, octet *HCID, octet *PK);

/**	@brief G2 right hand Time Permit
 *
    @param sha: integer representing the bit-length of the sha: 32, 48, 64
    @param date as input in days since the epoch
  	@param S is an input master secret
  	@param HCID is client ID hash
  	@param TP is the Time Permit given as output TP=S*H2(date|H(CID))
 */
void SOK_GET_G2_PERMIT(int sha, int date, octet *S, octet *HCID, octet *TP);

/**	@brief Calculate sender AES Key
 *
    @param sha: integer representing the bit-length of the sha: 32, 48, 64
    @param date as input in days since the epoch
  	@param AKeyG1: client secret in G1
  	@param AG1TP: Time Permit in G1
  	@param BID: other party identity
  	@param KEY: AES key as output, f_key(e((s*A+s*H(date|H(AID))),(B+H(date|H(BID))) ))
  	@return 0 or an error code
 */
int SOK_PAIR1(int sha, int date, octet *AKeyG1, octet *AG1TP, octet *BID, octet *KEY);

/**	@brief Calculate receiver AES Key
 *
    @param sha: integer representing the bit-length of the sha: 32, 48, 64
    @param date as input in days since the epoch
  	@param BKeyG2: client secret in G2
  	@param BG2TP: Time Permit in G2
  	@param AID: other party identity
  	@param KEY: AES key as output, f_key(e((A+H(date|H(AID))),(s*B+s*H(date|H(BID)))))
  	@return 0 or an error code
 */
int SOK_PAIR2(int sha, int date, octet *BKeyG2, octet *BG2TP, octet *AID, octet *KEY);

/**	@brief SOK_AES_GCM_ENCRYPT Encryption
 *
    @param K: Encryption key
    @param IV: Initial Vector
  	@param H: Client Identity
  	@param P: Plaintext as input
  	@param C: Ciphertext as output
  	@param T: Encryption Tag, as output
 */
void SOK_AES_GCM_ENCRYPT(octet *K, octet *IV, octet *H, octet *P, octet *C, octet *T);

/**	@brief SOK_AES_GCM_DECRYPT Decryption
 *
    @param K: Decryption key
    @param IV: Initial Vector
  	@param H: Client Identity
  	@param C: Ciphertext as input
  	@param P: Plaintext as output
  	@param T: Encryption Tag
 */
void SOK_AES_GCM_DECRYPT(octet *K, octet *IV, octet *H, octet *C, octet *P, octet *T);

/**	@brief Initialise a Cryptographically Strong Random Number Generator from an octet of raw random data
 *
    @param sha: integer representing the bit-length of the sha: 32, 48, 64
    @param ID: Client Identity
  	@param HID: HASH of the Client Identity
 */
void SOK_HASH_ID(int sha, octet *ID, octet *HID);

/** @brief Add two element of G1
  *
    @param R1: element of G1 given as input
    @param R2: element of G1 given as input
    @param R=R1+R2 in group G1
    @return 0 or an error code
 */
int SOK_RECOMBINE_G1(octet *R1, octet *R2, octet *R);

/**	@brief Add two element of G2
 *
    @param W1: element of G2 given as input
    @param W2: element of G2 given as input
    @param W=W1+W2 in group G2
    @return 0 or an error code
 */
int SOK_RECOMBINE_G2(octet *W1, octet *W2, octet *W);

/**	@brief Epoch day time in slots
 *
  	@return return time in slots since epoch
 */
unsign32 SOK_today(void);

#endif
