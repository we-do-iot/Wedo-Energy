# EDC Energy Meter LoRaWAN Payload Parser

Este directorio contiene los decodificadores/codificadores de payload para el medidor de energía EDC LoRaWAN.

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `ttn_decoder.js` | Decodificador para The Things Network (TTN) V3 |
| `chirpstack_v4_decoder.js` | Decodificador para Chirpstack V4 |

---

## Uplinks (Dispositivo → Servidor)

### Puerto 2 - Datos de medidor

El payload utiliza codificación **TLV (Type-Length-Value)** con valores Big-Endian.

#### Formato TLV

Cada campo se envía como:
```
[Channel ID (1 byte)] [Value (N bytes)]
```

#### Campos Soportados

| Channel ID | Nombre | Bytes | Descripción | Unidad |
|------------|--------|-------|-------------|--------|
| `0x02` | battery | 1 | Nivel de batería | % (0-100, 0xFF=N/A) |
| `0x03` | read_error | 1 | Error de lectura del medidor | 0=OK, 1=Error |
| `0x04` | network_state | 1 | Estado de red (detección de 3.3V externo) | 0=Ausente, 1=Presente |
| `0x05` | firmware | 1 | Versión de firmware | - |
| `0x0A` | active_energy | 4 | Energía activa total | Wh |
| `0x0B` | reactive_energy | 4 | Energía reactiva total | VArh |
| `0x0C` | apparent_energy | 4 | Energía aparente total | VAh |
| `0x0D` | reactive_qa | 3 | Reactiva Q-A | kVArh (/1000) |
| `0x0E` | reactive_qb | 3 | Reactiva Q-B | kVArh (/1000) |
| `0x14` | i1 | 3 | Corriente fase 1 | A (/10) |
| `0x15` | i2 | 3 | Corriente fase 2 | A (/10) |
| `0x16` | i3 | 3 | Corriente fase 3 | A (/10) |
| `0x1E` | v1 | 2 | Voltaje fase 1 | V (/10) |
| `0x1F` | v2 | 2 | Voltaje fase 2 | V (/10) |
| `0x20` | v3 | 2 | Voltaje fase 3 | V (/10) |
| `0x28` | peak_demand | 2 | Demanda máxima de potencia | W |
| `0x29` | last_demand | 3 | Última demanda | W |
| `0x32` | fpl1 | 1 | Factor de potencia fase 1 | (/100) |
| `0x33` | fpl2 | 1 | Factor de potencia fase 2 | (/100) |
| `0x34` | fpl3 | 1 | Factor de potencia fase 3 | (/100) |
| `0x35` | fplt | 1 | Factor de potencia total | (/100) |
| `0x3C` | active_consumed | 4 | Energía activa consumida | Wh |
| `0x3D` | active_generated | 4 | Energía activa generada | Wh |
| `0x3E` | reactive_consumed | 4 | Energía reactiva consumida | VArh |
| `0x3F` | reactive_generated | 4 | Energía reactiva generada | VArh |
| `0x46` | active_energy_last_period | 4 | Energía activa período anterior | Wh |
| `0x47` | reactive_energy_last_period | 4 | Energía reactiva período anterior | VArh |
| `0x48` | reactive_consumption_last_period | 4 | Reactiva consumida período anterior | VArh |
| `0x49` | reactive_generated_last_period | 4 | Reactiva generada período anterior | VArh |
| `0x4A` | peak_demand_last_period | 2 | Demanda máxima período anterior | W |
| `0x4B` | active_energy_last_period_alt | 2 | Energía activa período anterior (alt) | Wh |
| `0x50` | statf1 | 1 | Estado flag 1 | - |
| `0x51` | statf2 | 1 | Estado flag 2 | - |
| `0x5A` | serial_number | 4 | Número de serie (numérico) | - |
| `0x5B` | serial_number_str | 8 | Número de serie (string ASCII) | - |

### Puerto 2 - Range Test

Cuando el payload tiene 5 bytes y comienza con `0xFF`, es un mensaje de test de alcance:

| Offset | Bytes | Descripción |
|--------|-------|-------------|
| 0 | 1 | Identificador: `0xFF` |
| 1-4 | 4 | Timestamp Unix (Big-Endian) |

**Ejemplo:** `FF 67 65 0A BC` → timestamp = 1734676156

---

## Downlinks (Servidor → Dispositivo)

### Puerto 85 - Comandos de Configuración

#### Comando: Set Reporting Interval (0xFF03)

Configura el intervalo de reporte en segundos.

| Offset | Bytes | Descripción |
|--------|-------|-------------|
| 0-1 | 2 | Command ID: `0xFF 0x03` |
| 2-3 | 2 | Intervalo en segundos (Little-Endian) |

**Ejemplos:**
- `FF 03 3C 00` → Intervalo de 60 segundos (1 minuto)
- `FF 03 E8 03` → Intervalo de 1000 segundos (~16.6 minutos)
- `FF 03 00 00` → Reset al intervalo por defecto

**Uso en TTN/Chirpstack (JSON):**
```json
{
  "command": "set_reporting_interval",
  "interval_seconds": 60
}
```

---

#### Comando: Reset (0xFF10)

Reinicia el dispositivo después de completar el siguiente uplink.

| Offset | Bytes | Descripción |
|--------|-------|-------------|
| 0-1 | 2 | Command ID: `0xFF 0x10` |
| 2 | 1 | Confirmación: `0xFF` |

**Payload:** `FF 10 FF`

**Uso en TTN/Chirpstack (JSON):**
```json
{
  "command": "reset"
}
```

---

#### Comando: Factory Reset LoRaWAN (0xFF99)

Borra la memoria NVM de LoRaWAN y reinicia el dispositivo. El dispositivo deberá volver a hacer JOIN.

| Offset | Bytes | Descripción |
|--------|-------|-------------|
| 0-1 | 2 | Command ID: `0xFF 0x99` |
| 2 | 1 | Confirmación: `0xFF` |

**Payload:** `FF 99 FF`

**Uso en TTN/Chirpstack (JSON):**
```json
{
  "command": "factory_reset"
}
```

---

## Instalación

### The Things Network (TTN) V3

1. Ve a tu aplicación en la consola de TTN
2. Navega a **End devices** → selecciona tu dispositivo → **Payload formatters**
3. Selecciona **Uplink** → **Custom JavaScript formatter**
4. Copia el contenido de `ttn_decoder.js`
5. Repite para **Downlink** si deseas enviar comandos codificados

### Chirpstack V4

1. Accede a tu instancia de Chirpstack
2. Ve a **Device profiles** → selecciona o crea un perfil
3. En la pestaña **Codec**, selecciona **JavaScript functions**
4. Copia el contenido de `chirpstack_v4_decoder.js`
5. Guarda los cambios

---

## Ejemplos de Payload

### Ejemplo 1: Payload típico de medidor

**Hex:** `02 64 04 01 0A 00 00 10 68 3C 00 00 07 D0 3D 00 00 00 00`

**Decodificado:**
```json
{
  "battery": 100,
  "network_state": 1,
  "active_energy": 4200,
  "active_consumed": 2000,
  "active_generated": 0
}
```

### Ejemplo 2: Solo batería y estado (sin datos del medidor)

**Hex:** `02 5A 04 00`

**Decodificado:**
```json
{
  "battery": 90,
  "network_state": 0
}
```

### Ejemplo 3: Range Test

**Hex:** `FF 67 65 0A BC`

**Decodificado:**
```json
{
  "message_type": "range_test",
  "timestamp": 1734676156
}
```

---

## Notas

- Todos los valores multi-byte en uplinks son **Big-Endian**
- Los campos de downlink con valores multi-byte son **Little-Endian** (solo el intervalo)
- El puerto 85 está reservado para comandos de configuración
- El puerto 2 es el puerto de datos de aplicación por defecto
