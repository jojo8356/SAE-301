#ifndef ANIMETCG_CRYPTO_INTERNAL_H
#define ANIMETCG_CRYPTO_INTERNAL_H

#include "crypto.h"

bool crypto_difficulty_is_supported(uint32_t difficulty);
crypto_status_t crypto_validate_header_fields(
    const crypto_header_fields_t *fields);
void crypto_store_u32_be(uint8_t *destination, uint32_t value);
void crypto_store_u64_be(uint8_t *destination, uint64_t value);

#endif /* ANIMETCG_CRYPTO_INTERNAL_H */
