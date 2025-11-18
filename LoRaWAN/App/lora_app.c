/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"

/* USER CODE BEGIN Includes */
#include "usart_if.h"
#include <stdlib.h>
#include <string.h>
#include "stm32_timer.h"  // Necesario para UTIL_TIMER_Object_t

/* Fallback defines: if the project or board headers do not define these, provide
   conservative defaults to allow compilation. If your project defines them
   elsewhere these will be ignored. */
#ifndef LED_PERIOD_TIME
#define LED_PERIOD_TIME 200U
#endif

#ifndef JOIN_TIME
#define JOIN_TIME 600000U
#endif

#ifndef LORAWAN_NVM_BASE_ADDRESS
/* Default NVM base address fallback (tune to your board's flash map if needed) */
#define LORAWAN_NVM_BASE_ADDRESS ((void *)0x080E0000)
#endif
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
extern volatile uint8_t meter_data_ready;
extern volatile uint8_t button_pressed;
extern UTIL_TIMER_Object_t LedTimer;
extern char uart_rx_buffer[];
/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */

} TxEventType_t;

/**
  * Will be called each time a Radio IRQ is handled by the MAC layer
  *
  */
static void OnMacProcessNotify(void);

/**
  * @brief Change the periodicity of the uplink frames
  * @param periodicity uplink frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnTxPeriodicityChanged(uint32_t periodicity);

/**
  * @brief Change the confirmation control of the uplink frames
  * @param isTxConfirmed Indicates if the uplink requires an acknowledgement
  * @note Compliance test protocol callbacks
  */
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);

/**
  * @brief Change the periodicity of the ping slot frames
  * @param pingSlotPeriodicity ping slot frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

/**
  * @brief Will be called to reset the system
  * @note Compliance test protocol callbacks
  */
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */

/**
  * @brief  LED Tx timer callback function
  * @param  context ptr of LED context
  */
static void OnTxTimerLedEvent(void *context);

/**
  * @brief  LED Rx timer callback function
  * @param  context ptr of LED context
  */
static void OnRxTimerLedEvent(void *context);

/**
  * @brief  LED Join timer callback function
  * @param  context ptr of LED context
  */
static void OnJoinTimerLedEvent(void *context);

/**
  * @brief  Parse OBIS float value from buffer
  * @param  buffer pointer to the OBIS buffer
  * @param  marker OBIS marker string (e.g., "1.8.0(")
  * @param  result pointer to store the parsed float value
  * @retval true if parsing successful, false otherwise
  */
static bool ParseOBISFloat(char *buffer, const char *marker, float *result);

/**
  * @brief  Parse OBIS uint32 value from buffer
  * @param  buffer pointer to the OBIS buffer
  * @param  marker OBIS marker string (e.g., "C.1.0(")
  * @param  result pointer to store the parsed uint32 value
  * @retval true if parsing successful, false otherwise
  */
static bool ParseOBISUint32(char *buffer, const char *marker, uint32_t *result);

/* Forward declarations for callbacks and local functions used by the LmHandler
  These must be visible before the LmHandlerCallbacks structure initialization. */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size);
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size);
static void OnNvmDataChange(LmHandlerNvmContextStates_t state);
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(void);
static void OnClassChange(DeviceClass_t deviceClass);
static void SendTxData(void);
static void OnTxTimerEvent(void *context);
static void StoreContext(void);
static void StopJoin(void);
static void OnStopJoinTimerEvent(void *context);
static void OnTxTimerLedEvent(void *context);
static void OnRxTimerLedEvent(void *context);
static void OnJoinTimerLedEvent(void *context);
static void OnTxPeriodicityChanged(uint32_t periodicity);
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);
static void OnSystemReset(void);


/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN default activation type
  */
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN force rejoin even if the NVM context is restored
  */
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =              GetBatteryLevel,
  .GetTemperature =               GetTemperatureLevel,
  .GetUniqueId =                  GetUniqueId,
  .GetDevAddr =                   GetDevAddr,
  .OnRestoreContextRequest =      OnRestoreContextRequest,
  .OnStoreContextRequest =        OnStoreContextRequest,
  .OnMacProcess =                 OnMacProcessNotify,
  .OnNvmDataChange =              OnNvmDataChange,
  .OnJoinRequest =                OnJoinRequest,
  .OnTxData =                     OnTxData,
  .OnRxData =                     OnRxData,
  .OnBeaconStatusChange =         OnBeaconStatusChange,
  .OnSysTimeUpdate =              OnSysTimeUpdate,
  .OnClassChange =                OnClassChange,
  .OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
  .OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
  .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
  .OnSystemReset =                OnSystemReset,
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .TxPower =                  LORAWAN_DEFAULT_TX_POWER,
  .PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
  .RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/**
  * @brief Tx Timer period
  */
static UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;

/**
  * @brief Join Timer period
  */
static UTIL_TIMER_Object_t StopJoinTimer;

/* USER CODE BEGIN PV */
/**
  * @brief User application buffer
  */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/**
  * @brief User application data structure
  */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/**
  * @brief Specifies the state of the application LED
  */
static uint8_t AppLedStateOn = RESET;

/**
  * @brief Timer to handle the application Tx Led to toggle
  */
static UTIL_TIMER_Object_t TxLedTimer;

/**
  * @brief Timer to handle the application Rx Led to toggle
  */
static UTIL_TIMER_Object_t RxLedTimer;

/**
  * @brief Timer to handle the application Join Led to toggle
  */
static UTIL_TIMER_Object_t JoinLedTimer;

/* Human-readable RX slot strings used in debug logs */
static const char *slotStrings[] = { "NONE", "RX1", "RX2", "C", "P", "MULTI" };

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_LV */
  uint32_t feature_version = 0UL;
  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* Get LoRaWAN APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW LoRaWAN info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_LORAWAN_VERSION:  V%X.%X.%X\r\n",
          (uint8_t)(LORAWAN_VERSION_MAIN),
          (uint8_t)(LORAWAN_VERSION_SUB1),
          (uint8_t)(LORAWAN_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Get LoRaWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "L2_SPEC_VERSION:     V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8),
          (uint8_t)(feature_version));

  UTIL_TIMER_Create(&TxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnTxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&RxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnRxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&JoinLedTimer, LED_PERIOD_TIME, UTIL_TIMER_PERIODIC, OnJoinTimerLedEvent, NULL);

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_TIMER_Create(&StopJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnStopJoinTimerEvent, NULL);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), UTIL_SEQ_RFU, StoreContext);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), UTIL_SEQ_RFU, StopJoin);

  /* Init Info table used by LmHandler*/
  LoraInfo_Init();

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */
  UTIL_TIMER_Start(&JoinLedTimer);

  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType, ForceRejoin);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */

    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */

  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
char ledpulado[] = "Led pulsado\r\n";
  switch (GPIO_Pin)
  {
    case Pulsador_Pin:
      // Encender LED
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      HAL_UART_Transmit(&huart1, (uint8_t*)ledpulado, strlen(ledpulado), HAL_MAX_DELAY);
      // Iniciar timer para apagar después de 1 segundo
      UTIL_TIMER_Start(&LedTimer);

      // Marcar que se presionó el botón
      button_pressed = 1;
      break;

    case BUT1_Pin:
      // XXX: always initialized
      if (EventType == TX_ON_EVENT || 1)
      {
        UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
      }
      break;

    default:
      break;
  }
}

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
  uint8_t RxPort = 0;

  if (params != NULL)
  {
#if 0   // XXX:
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); /* LED_BLUE */
#endif

    UTIL_TIMER_Start(&RxLedTimer);

    if (params->IsMcpsIndication)
    {
      if (appData != NULL)
      {
        RxPort = appData->Port;
        if (appData->Buffer != NULL)
        {
          switch (appData->Port)
          {
            case LORAWAN_SWITCH_CLASS_PORT:
              /*this port switches the class*/
              if (appData->BufferSize == 1)
              {
                switch (appData->Buffer[0])
                {
                  case 0:
                  {
                    LmHandlerRequestClass(CLASS_A);
                    break;
                  }
                  case 1:
                  {
                    LmHandlerRequestClass(CLASS_B);
                    break;
                  }
                  case 2:
                  {
                    LmHandlerRequestClass(CLASS_C);
                    break;
                  }
                  default:
                    break;
                }
              }
              break;
            case LORAWAN_USER_APP_PORT:
              if (appData->BufferSize == 1)
              {
                AppLedStateOn = appData->Buffer[0] & 0x01;
                if (AppLedStateOn == RESET)
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED OFF\r\n");
                  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); /* LED_RED */
                }
                else
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED ON\r\n");
                  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_RED */
                }
              }
              break;

            default:

              break;
          }
        }
      }
    }
    if (params->RxSlot < RX_SLOT_NONE)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "###### D/L FRAME:%04d | PORT:%d | DR:%d | SLOT:%s | RSSI:%d | SNR:%d\r\n",
              params->DownlinkCounter, RxPort, params->Datarate, slotStrings[params->RxSlot], params->Rssi, params->Snr);
    }
  }
  /* USER CODE END OnRxData_1 */
}

/* USER CODE BEGIN PrFD_OBIS_Parser */

/**
  * @brief  Parse OBIS float value from buffer
  * @param  buffer pointer to the OBIS buffer
  * @param  marker OBIS marker string (e.g., "1.8.0(")
  * @param  result pointer to store the parsed float value
  * @retval true if parsing successful, false otherwise
  */
static bool ParseOBISFloat(char *buffer, const char *marker, float *result)
{
  char *pos = strstr(buffer, marker);
  if (pos == NULL)
  {
    return false;
  }

  // Avanzar después del marcador
  pos += strlen(marker);

  // Buscar el final del valor numérico (* o ))
  char *end_pos = pos;
  while (*end_pos != '\0' && *end_pos != '*' && *end_pos != ')')
  {
    // Validar que sea un carácter numérico, punto decimal, o signo
    if ((*end_pos >= '0' && *end_pos <= '9') || 
        *end_pos == '.' || 
        *end_pos == '-' || 
        *end_pos == '+')
    {
      end_pos++;
    }
    else
    {
      break;
    }
  }

  // Si no encontramos ningún carácter válido, fallar
  if (end_pos == pos)
  {
    return false;
  }

  // Extraer el substring numérico
  char temp_buffer[32];
  uint32_t len = (uint32_t)(end_pos - pos);
  if (len >= sizeof(temp_buffer))
  {
    len = sizeof(temp_buffer) - 1;
  }
  
  strncpy(temp_buffer, pos, len);
  temp_buffer[len] = '\0';

  // Convertir a float
  *result = atof(temp_buffer);
  
    // Debug: mostrar lo que se parseó (printf %f no soportado en tiny printf, imprimimos la cadena)
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
static bool ParseOBISUint32(char *buffer, const char *marker, uint32_t *result)
{
  char *pos = strstr(buffer, marker);
  if (pos == NULL)
  {
  }

  // Avanzar después del marcador
  pos += strlen(marker);

  // Buscar el final del valor numérico (* o ))
  char *end_pos = pos;
  while (*end_pos != '\0' && *end_pos != '*' && *end_pos != ')')
  {
    // Validar que sea un carácter numérico
    if (*end_pos >= '0' && *end_pos <= '9')
    {
      end_pos++;
    }
    else
    {
      break;
    }
  }

  // Si no encontramos ningún carácter válido, fallar
  if (end_pos == pos)
  {
    return false;
  }

  // Extraer el substring numérico
  char temp_buffer[32];
  uint32_t len = (uint32_t)(end_pos - pos);
  if (len >= sizeof(temp_buffer))
  {
    len = sizeof(temp_buffer) - 1;
  }
  
  strncpy(temp_buffer, pos, len);
  temp_buffer[len] = '\0';

  // Convertir a uint32_t
  *result = atol(temp_buffer);
  
  // Debug: mostrar lo que se parseó
    APP_LOG(TS_ON, VLEVEL_M, "DEBUG: ParseOBISUint32('%s') -> '%s' = %u\r\n", marker, temp_buffer, (unsigned int)*result);
  
  return true;
}

/* USER CODE END PrFD_OBIS_Parser */

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
  UTIL_TIMER_Time_t nextTxIn = 0;
  uint32_t payload_index = 0;

  AppData.Port = LORAWAN_USER_APP_PORT;

  // Verificar si hay datos del medidor listos
  if (meter_data_ready) {
      APP_LOG(TS_ON, VLEVEL_M, "Construyendo payload TLV desde datos OBIS...\r\n");

      // Variables temporales para parseo
      float valor_float = 0.0f;
      uint32_t valor_uint32 = 0;
      bool parse_ok = false;

    // ===== 0x02: Batería (%) - 1 byte =====
    uint8_t bateria_level_lora = GetBatteryLevel();
    uint8_t bateria_pct = 0xFF;
    /* Convert LoRa battery level (1..254) to percentage (0..100).
     Keep 0xFF as 'not measured' sentinel. */
    if (bateria_level_lora == 0xFF)
    {
      bateria_pct = 0xFF;
    }
    else if (bateria_level_lora == 0)
    {
      bateria_pct = 0;
    }
    else
    {
      const uint16_t LORAWAN_MAX_BAT = 254U;
      bateria_pct = (uint8_t)((((uint32_t)bateria_level_lora) * 100U + (LORAWAN_MAX_BAT/2U)) / LORAWAN_MAX_BAT);
    }
    AppData.Buffer[payload_index++] = 0x02;  // ID
    AppData.Buffer[payload_index++] = bateria_pct;
    if (bateria_pct == 0xFF)
    {
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x02 Bateria=NA\r\n");
    }
    else
    {
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x02 Bateria=%u%%\r\n", (unsigned int)bateria_pct);
    }

    /* ===== 0x04: network_state (1 byte) - detect external 3.3V on PB5 ===== */
    {
      uint8_t net_state = 0;
      if (HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin) == GPIO_PIN_SET)
      {
        net_state = 1; /* external 3.3V present */
      }
      AppData.Buffer[payload_index++] = 0x04; /* ID */
      AppData.Buffer[payload_index++] = net_state;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x04 network_state=%u\r\n", (unsigned int)net_state);
    }

      // ===== 0x0A: Energía activa total (15.8.0) - 4 bytes en Wh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "15.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t energia_kwh = (uint32_t)(valor_float);  // kWh a Wh
      AppData.Buffer[payload_index++] = 0x0A;  // ID
      AppData.Buffer[payload_index++] = (energia_kwh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (energia_kwh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (energia_kwh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = energia_kwh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x0A Activa_Total=%u Wh\r\n", (unsigned int)energia_kwh);
    }

      // ===== 0x0B: Energía reactiva total (130.8.0) - 4 bytes en VArh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "130.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t reactiva_kvarh = (uint32_t)(valor_float);  // kVArh a VArh
      AppData.Buffer[payload_index++] = 0x0B;  // ID
      AppData.Buffer[payload_index++] = (reactiva_kvarh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_kvarh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_kvarh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = reactiva_kvarh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x0B Reactiva_Total=%u VArh\r\n", (unsigned int)reactiva_kvarh);
    }

      // ===== 0x28: Demanda máxima potencia (1.6.0) - 2 bytes en W =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "1.6.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 65.535f) {
      uint16_t peak_demand_kw = (uint16_t)(valor_float);  // kW a W
      AppData.Buffer[payload_index++] = 0x28;  // ID
      AppData.Buffer[payload_index++] = (peak_demand_kw >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = peak_demand_kw & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x28 Demanda_Max=%u W\r\n", (unsigned int)peak_demand_kw);
    }

      // ===== 0x3C: Energía activa consumida (1.8.0) - 4 bytes en Wh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "1.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t consumida_kwh = (uint32_t)(valor_float);
      AppData.Buffer[payload_index++] = 0x3C;  // ID
      AppData.Buffer[payload_index++] = (consumida_kwh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (consumida_kwh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (consumida_kwh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = consumida_kwh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x3C Activa_Consumida=%u Wh\r\n", (unsigned int)consumida_kwh);
    }

      // ===== 0x3D: Energía activa generada (2.8.0) - 4 bytes en Wh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "2.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t generada_kwh = (uint32_t)(valor_float);
      AppData.Buffer[payload_index++] = 0x3D;  // ID
      AppData.Buffer[payload_index++] = (generada_kwh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (generada_kwh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (generada_kwh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = generada_kwh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x3D Activa_Generada=%u Wh\r\n", (unsigned int)generada_kwh);
    }

      // ===== 0x3E: Energía reactiva consumida (3.8.0) - 4 bytes en VArh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "3.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t reactiva_cons_kvarh = (uint32_t)(valor_float);
      AppData.Buffer[payload_index++] = 0x3E;  // ID
      AppData.Buffer[payload_index++] = (reactiva_cons_kvarh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_cons_kvarh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_cons_kvarh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = reactiva_cons_kvarh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x3E Reactiva_Consumida=%u VArh\r\n", (unsigned int)reactiva_cons_kvarh);
    }

      // ===== 0x3F: Energía reactiva generada (4.8.0) - 4 bytes en VArh =====
      parse_ok = ParseOBISFloat(uart_rx_buffer, "4.8.0(", &valor_float);
    if (parse_ok && valor_float >= 0.0f && valor_float < 10000000.0f) {
      uint32_t reactiva_gen_kvarh = (uint32_t)(valor_float);
      AppData.Buffer[payload_index++] = 0x3F;  // ID
      AppData.Buffer[payload_index++] = (reactiva_gen_kvarh >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_gen_kvarh >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (reactiva_gen_kvarh >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = reactiva_gen_kvarh & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x3F Reactiva_Generada=%u VArh\r\n", (unsigned int)reactiva_gen_kvarh);
    }

      // ===== 0x5A: Número de serie (C.1.0) - 4 bytes =====
    parse_ok = ParseOBISUint32(uart_rx_buffer, "C.1.0(", &valor_uint32);
    if (parse_ok && valor_uint32 != 0) {
      AppData.Buffer[payload_index++] = 0x5A;  // ID
      AppData.Buffer[payload_index++] = (valor_uint32 >> 24) & 0xFF;
      AppData.Buffer[payload_index++] = (valor_uint32 >> 16) & 0xFF;
      AppData.Buffer[payload_index++] = (valor_uint32 >> 8) & 0xFF;
      AppData.Buffer[payload_index++] = valor_uint32 & 0xFF;
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x5A Numero_Serie=%u\r\n", (unsigned int)valor_uint32);
    }

      AppData.BufferSize = payload_index;
      meter_data_ready = 0;

      APP_LOG(TS_ON, VLEVEL_M, "Payload TLV construido: %d bytes\r\n", payload_index);

  } else {
      // Sin datos del medidor, enviar solo batería
      APP_LOG(TS_ON, VLEVEL_M, "Sin datos del medidor, enviando solo bateria\r\n");
    {
      /* Fallback: send battery + network_state */
      uint8_t bateria_level_lora = GetBatteryLevel();
      uint8_t bateria_pct = 0xFF;
      const uint16_t LORAWAN_MAX_BAT = 254U;
      if (bateria_level_lora == 0xFF)
      {
        bateria_pct = 0xFF;
      }
      else if (bateria_level_lora == 0)
      {
        bateria_pct = 0;
      }
      else
      {
        bateria_pct = (uint8_t)((((uint32_t)bateria_level_lora) * 100U + (LORAWAN_MAX_BAT/2U)) / LORAWAN_MAX_BAT);
      }
      AppData.Buffer[0] = 0x02;  // ID Batería
      AppData.Buffer[1] = bateria_pct;

      /* Add network_state TLV (0x04) */
      uint8_t net_state = 0;
      if (HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin) == GPIO_PIN_SET)
      {
        net_state = 1;
      }
      AppData.Buffer[2] = 0x04;
      AppData.Buffer[3] = net_state;
      AppData.BufferSize = 4;

      if (bateria_pct == 0xFF)
      {
        APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x02 Bateria=NA\r\n");
      }
      else
      {
        APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x02 Bateria=%u%%\r\n", (unsigned int)bateria_pct);
      }
      APP_LOG(TS_ON, VLEVEL_M, "TLV: 0x04 network_state=%u\r\n", (unsigned int)net_state);
    }
  }

  if ((JoinLedTimer.IsRunning) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    UTIL_TIMER_Stop(&JoinLedTimer);
  }

  status = LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed, false);
  if (LORAMAC_HANDLER_SUCCESS == status)
  {
    APP_LOG(TS_ON, VLEVEL_L, "SEND REQUEST\r\n");
  }
  else if (LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED == status)
  {
    nextTxIn = LmHandlerGetDutyCycleWaitTime();
    if (nextTxIn > 0)
    {
      APP_LOG(TS_ON, VLEVEL_L, "Next Tx in  : ~%d second(s)\r\n", (nextTxIn / 1000));
    }
  }

  if (EventType == TX_ON_TIMER)
  {
    UTIL_TIMER_Stop(&TxTimer);
    UTIL_TIMER_SetPeriod(&TxTimer, MAX(nextTxIn, TxPeriodicity));
    UTIL_TIMER_Start(&TxTimer);
  }
  /* USER CODE END SendTxData_1 */
}
static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */

  /* USER CODE END OnTxTimerEvent_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */
static void OnTxTimerLedEvent(void *context)
{
#if 0	// XXX: No LED available
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
#endif
}

static void OnRxTimerLedEvent(void *context)
{
#if 0   // XXX: No LED available
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); /* LED_BLUE */
#endif
}

static void OnJoinTimerLedEvent(void *context)
{
#if 0   // XXX: No LED available
  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
#endif
}

/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
  if ((params != NULL))
  {
    /* Process Tx event only if its a mcps response to prevent some internal events (mlme) */
    if (params->IsMcpsConfirm != 0)
    {
#if 0	// XXX: No LED available
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); /* LED_GREEN */
#endif
      UTIL_TIMER_Start(&TxLedTimer);

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### ========== MCPS-Confirm =============\r\n");
      APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", params->UplinkCounter,
              params->AppData.Port, params->Datarate, params->TxPower);

      APP_LOG(TS_OFF, VLEVEL_H, " | MSG TYPE:");
      if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
      {
        APP_LOG(TS_OFF, VLEVEL_H, "CONFIRMED [%s]\r\n", (params->AckReceived != 0) ? "ACK" : "NACK");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "UNCONFIRMED\r\n");
      }
    }
  }
  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
  if (joinParams != NULL)
  {
    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
    {
      UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), CFG_SEQ_Prio_0);

      UTIL_TIMER_Stop(&JoinLedTimer);
#if 0   // XXX:
      HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
#endif

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOINED = ");
      if (joinParams->Mode == ACTIVATION_TYPE_ABP)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "ABP ======================\r\n");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "OTAA =====================\r\n");
      }
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOIN FAILED\r\n");

      if (joinParams->Mode == ACTIVATION_TYPE_OTAA) {
          APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = RE-TRYING OTAA JOIN\r\n");
    	/* re-try the OTAA join */
    	LmHandlerJoin(ActivationType, LORAWAN_FORCE_REJOIN_AT_BOOT);
      }
    }
  }
  /* USER CODE END OnJoinRequest_1 */
}

static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN OnBeaconStatusChange_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        APP_LOG(TS_OFF, VLEVEL_M,
                "\r\n###### BEACON RECEIVED | DR:%d | RSSI:%d | SNR:%d | FQ:%d | TIME:%d | DESC:%d | "
                "INFO:02X%02X%02X %02X%02X%02X\r\n",
                params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON NOT RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE END OnBeaconStatusChange_1 */
}

static void OnSysTimeUpdate(void)
{
  /* USER CODE BEGIN OnSysTimeUpdate_1 */

  /* USER CODE END OnSysTimeUpdate_1 */
}

static void OnClassChange(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN OnClassChange_1 */
  APP_LOG(TS_OFF, VLEVEL_M, "Switch to Class %c done\r\n", "ABC"[deviceClass]);
  /* USER CODE END OnClassChange_1 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

  /* USER CODE BEGIN OnMacProcessNotify_2 */

  /* USER CODE END OnMacProcessNotify_2 */
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
  /* USER CODE BEGIN OnTxPeriodicityChanged_1 */

  /* USER CODE END OnTxPeriodicityChanged_1 */
  TxPeriodicity = periodicity;

  if (TxPeriodicity == 0)
  {
    /* Revert to application default periodicity */
    TxPeriodicity = APP_TX_DUTYCYCLE;
  }

  /* Update timer periodicity */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_SetPeriod(&TxTimer, TxPeriodicity);
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxPeriodicityChanged_2 */

  /* USER CODE END OnTxPeriodicityChanged_2 */
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
  /* USER CODE BEGIN OnTxFrameCtrlChanged_1 */

  /* USER CODE END OnTxFrameCtrlChanged_1 */
  LmHandlerParams.IsTxConfirmed = isTxConfirmed;
  /* USER CODE BEGIN OnTxFrameCtrlChanged_2 */

  /* USER CODE END OnTxFrameCtrlChanged_2 */
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_1 */

  /* USER CODE END OnPingSlotPeriodicityChanged_1 */
  LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_2 */

  /* USER CODE END OnPingSlotPeriodicityChanged_2 */
}

static void OnSystemReset(void)
{
  /* USER CODE BEGIN OnSystemReset_1 */

  /* USER CODE END OnSystemReset_1 */
  if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    NVIC_SystemReset();
  }
  /* USER CODE BEGIN OnSystemReset_Last */

  /* USER CODE END OnSystemReset_Last */
}

static void StopJoin(void)
{
  /* USER CODE BEGIN StopJoin_1 */
#if 0   // XXX: No LED available
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); /* LED_BLUE */
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); /* LED_RED */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); /* LED_GREEN */
#endif

  /* USER CODE END StopJoin_1 */

  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");
    if (LORAWAN_DEFAULT_ACTIVATION_TYPE == ACTIVATION_TYPE_ABP)
    {
      ActivationType = ACTIVATION_TYPE_OTAA;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to OTAA mode\r\n");
    }
    else
    {
      ActivationType = ACTIVATION_TYPE_ABP;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to ABP mode\r\n");
    }
    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&StopJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnStopJoinTimerEvent(void *context)
{
  /* USER CODE BEGIN OnStopJoinTimerEvent_1 */

  /* USER CODE END OnStopJoinTimerEvent_1 */
  if (ActivationType == LORAWAN_DEFAULT_ACTIVATION_TYPE)
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), CFG_SEQ_Prio_0);
  }
  /* USER CODE BEGIN OnStopJoinTimerEvent_Last */
#if 0   // XXX: No LED available
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); /* LED_BLUE */
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
#endif
  /* USER CODE END OnStopJoinTimerEvent_Last */
}

static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  /* USER CODE BEGIN StoreContext_1 */

  /* USER CODE END StoreContext_1 */
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
  /* USER CODE BEGIN StoreContext_Last */

  /* USER CODE END StoreContext_Last */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  /* USER CODE BEGIN OnNvmDataChange_1 */

  /* USER CODE END OnNvmDataChange_1 */
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\n");
  }
  /* USER CODE BEGIN OnNvmDataChange_Last */

  /* USER CODE END OnNvmDataChange_Last */
}

static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnStoreContextRequest_1 */

  /* USER CODE END OnStoreContextRequest_1 */
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  }
  /* USER CODE BEGIN OnStoreContextRequest_Last */

  /* USER CODE END OnStoreContextRequest_Last */
}

static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnRestoreContextRequest_1 */

  /* USER CODE END OnRestoreContextRequest_1 */
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  /* USER CODE BEGIN OnRestoreContextRequest_Last */

  /* USER CODE END OnRestoreContextRequest_Last */
}

