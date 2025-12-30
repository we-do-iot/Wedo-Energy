# Range Test Feature

## Activación
**Doble pulsación** del botón físico

## Comportamiento
- Envía payload: `0xFF` + timestamp UNIX (4 bytes big-endian)
- **Data Rate: DR3** (SF9, 53 bytes max con Dwell Time)
- **Mensaje confirmado** (ACK requerido)
- **Sin lectura del medidor** - envío inmediato

## Log esperado
```
Doble pulsación detectada - Test de alcance LoRaWAN
Range Test: Construyendo mensaje de prueba...
Range Test payload: FF XX XX XX XX (timestamp=XXXXX)
Range Test: Modo confirmado activado (ACK)
Range Test: ADR deshabilitado: OK
Range Test: DR3 configurado OK
TX on freq XXXXXX Hz at DR 3
```

## Data Rates AU915 con Dwell Time

| DR | SF | Max Payload | Uso |
|----|-----|-------------|-----|
| DR2 | SF10 | 11 bytes | Mínimo permitido |
| **DR3** | SF9 | **53 bytes** | ✅ **Range test + medidor (42 bytes)** |
| DR4 | SF8 | 125 bytes | |
| DR5 | SF7 | 242 bytes | Máximo alcance corto |

## Archivos modificados
- `lora_app.c`: Handler doble pulsación + lógica range test con DR3
