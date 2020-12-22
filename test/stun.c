/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "stun.h"

#include <stdint.h>
#include <string.h>

int do_test_stun(juice_logger_t *logger);

int test_stun(void) {
	juice_log_config_t log_config;
	juice_logger_t *logger = juice_logger_create(&log_config);
	int ret = do_test_stun(logger);
	juice_logger_destroy(logger);
	return ret;
}

int do_test_stun(juice_logger_t *logger) {
    stun_message_t msg;
	uint8_t message1[] = {
	    0x00, 0x01, 0x00, 0x58, // Request type and message length
	    0x21, 0x12, 0xa4, 0x42, // Magic cookie
	    0xb7, 0xe7, 0xa7, 0x01, // Transaction ID
	    0xbc, 0x34, 0xd6, 0x86, //
	    0xfa, 0x87, 0xdf, 0xae, //
	    0x80, 0x22, 0x00, 0x10, // SOFTWARE attribute header
	    0x53, 0x54, 0x55, 0x4e, //
	    0x20, 0x74, 0x65, 0x73, //
	    0x74, 0x20, 0x63, 0x6c, //
	    0x69, 0x65, 0x6e, 0x74, //
	    0x00, 0x24, 0x00, 0x04, // PRIORITY attribute header
	    0x6e, 0x00, 0x01, 0xff, //
	    0x80, 0x29, 0x00, 0x08, // ICE-CONTROLLED attribute header
	    0x93, 0x2f, 0xf9, 0xb1, //
	    0x51, 0x26, 0x3b, 0x36, //
	    0x00, 0x06, 0x00, 0x09, // USERNAME attribute header
	    0x65, 0x76, 0x74, 0x6a, //
	    0x3a, 0x68, 0x36, 0x76, //
	    0x59, 0x20, 0x20, 0x20, //
	    0x00, 0x08, 0x00, 0x14, // MESSAGE-INTEGRITY attribute header
	    0x9a, 0xea, 0xa7, 0x0c, //
	    0xbf, 0xd8, 0xcb, 0x56, //
	    0x78, 0x1e, 0xf2, 0xb5, //
	    0xb2, 0xd3, 0xf2, 0x49, //
	    0xc1, 0xb5, 0x71, 0xa2, //
	    0x80, 0x28, 0x00, 0x04, // FINGERPRINT attribute header
	    0xe5, 0x7a, 0x3b, 0xcf, //
	};

	memset(&msg, 0, sizeof(msg));

	if (_juice_stun_read(message1, sizeof(message1), &msg, logger) <= 0)
		return -1;

	if(msg.msg_class != STUN_CLASS_REQUEST || msg.msg_method != STUN_METHOD_BINDING)
		return -1;

	if (memcmp(msg.transaction_id, message1 + 8, 12) != 0)
		return -1;

	if (msg.priority != 0x6e0001ff)
		return -1;

	if (msg.ice_controlled != 0x932ff9b151263b36LL)
		return -1;

	if (!msg.has_integrity)
		return -1;

	if (!_juice_stun_check_integrity(message1, sizeof(message1), &msg, "VOkJxbRl1RmTxUk/WvJxBt", logger))
		return -1;

	if(msg.error_code != 0)
		return -1;

	// The test vector in RFC 8489 is completely wrong
	// See https://www.rfc-editor.org/errata_search.php?rfc=8489
	uint8_t message2[] = {
	    0x00, 0x01, 0x00, 0x90, // Request type and message length
	    0x21, 0x12, 0xa4, 0x42, // Magic cookie
	    0x78, 0xad, 0x34, 0x33, // Transaction ID
	    0xc6, 0xad, 0x72, 0xc0, //
	    0x29, 0xda, 0x41, 0x2e, //
	    0x00, 0x1e, 0x00, 0x20, // USERHASH attribute header
	    0x4a, 0x3c, 0xf3, 0x8f, // Userhash value (32 bytes)
	    0xef, 0x69, 0x92, 0xbd, //
	    0xa9, 0x52, 0xc6, 0x78, //
	    0x04, 0x17, 0xda, 0x0f, //
	    0x24, 0x81, 0x94, 0x15, //
	    0x56, 0x9e, 0x60, 0xb2, //
	    0x05, 0xc4, 0x6e, 0x41, //
	    0x40, 0x7f, 0x17, 0x04, //
	    0x00, 0x15, 0x00, 0x29, // NONCE attribute header
	    0x6f, 0x62, 0x4d, 0x61, // Nonce value and padding (3 bytes)
	    0x74, 0x4a, 0x6f, 0x73, //
	    0x32, 0x41, 0x41, 0x41, //
	    0x43, 0x66, 0x2f, 0x2f, //
	    0x34, 0x39, 0x39, 0x6b, //
	    0x39, 0x35, 0x34, 0x64, //
	    0x36, 0x4f, 0x4c, 0x33, //
	    0x34, 0x6f, 0x4c, 0x39, //
	    0x46, 0x53, 0x54, 0x76, //
	    0x79, 0x36, 0x34, 0x73, //
	    0x41, 0x00, 0x00, 0x00, //
	    0x00, 0x14, 0x00, 0x0b, // REALM attribute header
	    0x65, 0x78, 0x61, 0x6d, // Realm value (11 bytes) and padding (1 byte)
	    0x70, 0x6c, 0x65, 0x2e, //
	    0x6f, 0x72, 0x67, 0x00, //
	    0x00, 0x1d, 0x00, 0x04, // PASSWORD-ALGORITHM attribute header
	    0x00, 0x02, 0x00, 0x00, // PASSWORD-ALGORITHM value (4 bytes)
	    0x00, 0x1c, 0x00, 0x20, // MESSAGE-INTEGRITY-SHA256 attribute header
	    0xb5, 0xc7, 0xbf, 0x00, // HMAC-SHA256 value
	    0x5b, 0x6c, 0x52, 0xa2, //
	    0x1c, 0x51, 0xc5, 0xe8, //
	    0x92, 0xf8, 0x19, 0x24, //
	    0x13, 0x62, 0x96, 0xcb, //
	    0x92, 0x7c, 0x43, 0x14, //
	    0x93, 0x09, 0x27, 0x8c, //
	    0xc6, 0x51, 0x8e, 0x65, //
	};

	memset(&msg, 0, sizeof(msg));

	if (_juice_stun_read(message2, sizeof(message2), &msg, logger) <= 0)
		return -1;

	if(msg.msg_class != STUN_CLASS_REQUEST || msg.msg_method != STUN_METHOD_BINDING)
		return -1;

	if (memcmp(msg.transaction_id, message2 + 8, 12) != 0)
		return -1;

	if (!msg.credentials.enable_userhash)
		return -1;

	if (memcmp(msg.credentials.userhash, message2 + 24, 32) != 0)
		return -1;

	if (strcmp(msg.credentials.realm, "example.org") != 0)
		return -1;

	if (strcmp(msg.credentials.nonce, "obMatJos2AAACf//499k954d6OL34oL9FSTvy64sA") != 0)
		return -1;

	if (!msg.has_integrity)
		return -1;

	// Username is "<U+30DE><U+30C8><U+30EA><U+30C3><U+30AF><U+30B9>" or "マトリックス"
	// aka "The Matrix" in Japanese
	strcpy(msg.credentials.username, "マトリックス");
	if (!_juice_stun_check_integrity(message2, sizeof(message2), &msg, "TheMatrIX", logger))
		return -1;

	if(msg.error_code != STUN_ERROR_INTERNAL_VALIDATION_FAILED)
		return -1;

	return 0;
}
