/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android_bpf_defs.h>  // load_word()
#include <linux/bpf.h>         // struct __sk_buff
#include <linux/netlink.h>     // struct nlmsghdr
#include <stdint.h>            // uint32_t

// M4: match 4 bytes. Returns 0 if all bytes match.
static inline uint32_t M4(struct __sk_buff* skb, unsigned int offset, uint8_t c0, uint8_t c1,
                          uint8_t c2, uint8_t c3) {
    return load_word(skb, offset) ^ ((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
}

// M2: match 2 bytes. Returns 0 if all bytes match.
static inline uint16_t M2(struct __sk_buff* skb, unsigned int offset, uint8_t c0, uint8_t c1) {
    return load_half(skb, offset) ^ ((c0 << 8) | c1);
}

// M1: match 1 byte. Returns 0 in case of a match.
static inline uint8_t M1(struct __sk_buff* skb, unsigned int offset, uint8_t c0) {
    return load_byte(skb, offset) ^ c0;
}

// Match "\0SUBSYSTEM=". Returns 0 in case of a match.
#define MATCH_SUBSYSTEM_LENGTH 11
static inline uint32_t match_subsystem(struct __sk_buff* skb, unsigned int offset) {
    return M4(skb, offset + 0, '\0', 'S', 'U', 'B') | M4(skb, offset + 4, 'S', 'Y', 'S', 'T') |
           M2(skb, offset + 8, 'E', 'M') | M1(skb, offset + 10, '=');
}

// Match "power_supply\0". Returns 0 in case of a match.
#define MATCH_POWER_SUPPLY_LENGTH 13
static inline uint32_t match_power_supply(struct __sk_buff* skb, unsigned int offset) {
    return M4(skb, offset + 0, 'p', 'o', 'w', 'e') | M4(skb, offset + 4, 'r', '_', 's', 'u') |
           M4(skb, offset + 8, 'p', 'p', 'l', 'y') | M1(skb, offset + 12, '\0');
}

// The Linux kernel 5.4 BPF verifier rejects this program, probably because of its size. Hence the
// restriction that the kernel version must be at least 5.10.
DEFINE_BPF_PROG_KVER("skfilter/power_supply", AID_ROOT, AID_SYSTEM, skfilter_power_supply,
                     KVER(5, 10, 0))
(struct __sk_buff* skb) {
    uint32_t i;

    // The first character matched by match_subsystem() is a '\0'. Starting
    // right past the netlink message header is fine since the SUBSYSTEM= text
    // never occurs at the start. See also the kobject_uevent_env() implementation:
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/lib/kobject_uevent.c?#n473
    // The upper bound of this loop has been chosen not to exceed the maximum
    // number of instructions in a BPF program (BPF loops are unrolled).
    for (i = sizeof(struct nlmsghdr); i < 256; ++i) {
        if (i + MATCH_SUBSYSTEM_LENGTH > skb->len) {
            break;
        }
        if (match_subsystem(skb, i) == 0) {
            goto found_subsystem;
        }
    }

    // The SUBSYSTEM= text has not been found in the bytes that have been
    // examined: let the user space software perform filtering.
    return skb->len;

found_subsystem:
    i += MATCH_SUBSYSTEM_LENGTH;
    if (i + MATCH_POWER_SUPPLY_LENGTH <= skb->len && match_power_supply(skb, i) == 0) {
        return skb->len;
    }
    return 0;
}

LICENSE("Apache 2.0");
CRITICAL("healthd");
