/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "ntohll.h"

uint64_t ntohll(uint64_t x) {
    return (((uint64_t)ntohl((uint32_t)x)) << 32) + ntohl(x >> 32);
}