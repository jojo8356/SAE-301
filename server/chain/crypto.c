#include "crypto.h"

const char *crypto_status_string(crypto_status_t status)
{
    switch (status) {
    case CRYPTO_OK:
        return "success";
    case CRYPTO_ERR_ARGUMENT:
        return "invalid argument";
    case CRYPTO_ERR_PAYLOAD_LENGTH:
        return "canonical payload length is outside 2..65536 bytes";
    case CRYPTO_ERR_HEADER_VERSION:
        return "unsupported block-header version";
    case CRYPTO_ERR_TIMESTAMP:
        return "negative block timestamp";
    case CRYPTO_ERR_DIFFICULTY:
        return "difficulty must be 3 or 4 hexadecimal zeroes";
    case CRYPTO_ERR_ATTEMPT_BUDGET:
        return "attempt budget must be positive and within the configured bound";
    case CRYPTO_ERR_NONCE_OVERFLOW:
        return "nonce range overflows uint64";
    case CRYPTO_ERR_OPENSSL:
        return "OpenSSL SHA-256 operation failed";
    case CRYPTO_ERR_NO_WORK:
        return "no proof of work found within the attempt budget";
    case CRYPTO_ERR_PAYLOAD_HASH_MISMATCH:
        return "canonical payload hash does not match the block header";
    case CRYPTO_ERR_BLOCK_HASH_MISMATCH:
        return "recomputed block hash does not match the stored hash";
    case CRYPTO_ERR_POW_INVALID:
        return "stored block hash does not meet its declared difficulty";
    case CRYPTO_ERR_CHAIN_EMPTY:
        return "block chain is empty; a genesis block is required";
    case CRYPTO_ERR_CHAIN_LINK:
        return "previous-hash link is broken";
    case CRYPTO_ERR_BLOCK_INDEX:
        return "block index is not contiguous or exceeds the header range";
    default:
        return "unknown cryptographic status";
    }
}
