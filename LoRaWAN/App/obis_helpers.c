/*
 * obis_helpers.c
 * Implementation of OBIS parsing helpers moved out of lora_app.c so CubeMX
 * won't erase them on .ioc regeneration.
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "sys_app.h"
#include "obis_helpers.h"

/**
  * @brief  Parse OBIS float value from buffer
  * @param  buffer pointer to the OBIS buffer
  * @param  marker OBIS marker string (e.g., "1.8.0(")
  * @param  result pointer to store the parsed float value
  * @retval true if parsing successful, false otherwise
  */
bool ParseOBISFloat(char *buffer, const char *marker, float *result)
{
  char *pos = strstr(buffer, marker);
  if (pos == NULL)
  {
    return false;
  }

  pos += strlen(marker);

  char *end_pos = pos;
  while (*end_pos != '\0' && *end_pos != '*' && *end_pos != ')')
  {
    if ((*end_pos >= '0' && *end_pos <= '9') || *end_pos == '.' || *end_pos == '-' || *end_pos == '+')
    {
      end_pos++;
    }
    else
    {
      break;
    }
  }

  if (end_pos == pos)
  {
    return false;
  }

  char temp_buffer[32];
  uint32_t len = (uint32_t)(end_pos - pos);
  if (len >= sizeof(temp_buffer))
  {
    len = sizeof(temp_buffer) - 1;
  }
  strncpy(temp_buffer, pos, len);
  temp_buffer[len] = '\0';

  *result = (float)atof(temp_buffer);

  APP_LOG(TS_ON, VLEVEL_M, "DEBUG: ParseOBISFloat('%s') -> '%s'\r\n", marker, temp_buffer);

  return true;
}

/**
  * @brief  Parse OBIS uint32 value from buffer
  * @param  buffer pointer to the OBIS buffer
  * @param  marker OBIS marker string (e.g., "C.1.0(")
  * @param  result pointer to store the parsed uint32 value
  * @retval true if parsing successful, false otherwise
  */
bool ParseOBISUint32(char *buffer, const char *marker, uint32_t *result)
{
  char *pos = strstr(buffer, marker);
  if (pos == NULL)
  {
    return false;
  }

  pos += strlen(marker);

  char *end_pos = pos;
  while (*end_pos != '\0' && *end_pos != '*' && *end_pos != ')')
  {
    if (*end_pos >= '0' && *end_pos <= '9')
    {
      end_pos++;
    }
    else
    {
      break;
    }
  }

  if (end_pos == pos)
  {
    return false;
  }

  char temp_buffer[32];
  uint32_t len = (uint32_t)(end_pos - pos);
  if (len >= sizeof(temp_buffer))
  {
    len = sizeof(temp_buffer) - 1;
  }
  strncpy(temp_buffer, pos, len);
  temp_buffer[len] = '\0';

  *result = (uint32_t)atol(temp_buffer);

  APP_LOG(TS_ON, VLEVEL_M, "DEBUG: ParseOBISUint32('%s') -> '%s' = %u\r\n", marker, temp_buffer, (unsigned int)*result);

  return true;
}
