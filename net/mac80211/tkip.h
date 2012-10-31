/*
 * Copyright 2002-2004, Instant802 Networks, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TKIP_H
#define TKIP_H

#include <linux/types.h>
#include <linux/crypto.h>
#include "ieee80211_key.h"

u8 * ieee80211_tkip_add_iv(u8 *pos, struct ieee80211_key *key,
			   u8 iv0, u8 iv1, u8 iv2);
void ieee80211_tkip_gen_phase1key(struct ieee80211_key *key, u8 *ta,
				  u16 *phase1key);
void ieee80211_tkip_gen_rc4key(struct ieee80211_key *key, u8 *ta,
			       u8 *rc4key);
void ieee80211_tkip_encrypt_data(struct crypto_blkcipher *tfm,
				 struct ieee80211_key *key,
				 u8 *pos, size_t payload_len, u8 *ta);
enum {
	TKIP_DECRYPT_OK = 0,
	TKIP_DECRYPT_NO_EXT_IV = -1,
	TKIP_DECRYPT_INVALID_KEYIDX = -2,
	TKIP_DECRYPT_REPLAY = -3,
};
int ieee80211_tkip_decrypt_data(struct crypto_blkcipher *tfm,
				struct ieee80211_key *key,
				u8 *payload, size_t payload_len, u8 *ta,
				int only_iv, int queue);

#endif /* TKIP_H */
