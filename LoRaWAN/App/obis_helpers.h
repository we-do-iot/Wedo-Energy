/*
 * obis_helpers.h
 * Helper functions to parse OBIS strings coming from the meter.
 * This file is outside CubeMX-managed files so it won't be overwritten.
 */
#ifndef __OBIS_HELPERS_H__
#define __OBIS_HELPERS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ParseOBISFloat(char *buffer, const char *marker, float *result);
bool ParseOBISUint32(char *buffer, const char *marker, uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __OBIS_HELPERS_H__ */
