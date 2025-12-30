# Wedo-Energy

Firmware LoRaWAN para monitoreo remoto de medidores de energía eléctrica.

## Descripción

**Wedo-Energy** es un dispositivo IoT basado en el módulo LoRa-E5 (STM32WLE5) que permite la lectura remota de medidores de energía eléctrica y transmite los datos a través de la red LoRaWAN.

### Características principales

- 📡 **Conectividad LoRaWAN** - Compatible con LoRaWAN 1.0.4, soporta Class A
- ⚡ **Lectura de medidores** - Comunicación serial con medidores de energía eléctrica
- 🔋 **Bajo consumo** - Optimizado para operación con batería
- 🔄 **Intervalo configurable** - El período de reporte se puede ajustar via downlink
- 📊 **Datos de energía** - Transmite consumo, voltaje, corriente, factor de potencia y más
- 🧪 **Range Test** - Modo de prueba de cobertura integrado

### Medidores soportados

| Fabricante | Modelo | Estado |
|------------|--------|--------|
| Hexing | HXE310 | ✅ Soportado |
| *Otros* | *Por definir* | 🔜 Próximamente |

## Requisitos

### Hardware
- Módulo LoRa-E5 (Seeed Studio) o compatible con STM32WLE5
- Programador ST-LINK
- Cable USB-C para alimentación y comunicación serial

### Software
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) v1.7.0 o superior
- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)

### Infraestructura
- Gateway LoRaWAN
- Network Server (TTN, ChirpStack, etc.)

## Configuración

### 1. Clonar el repositorio

```bash
git clone https://github.com/we-do-iot/Wedo-Energy.git
```

### 2. Abrir en STM32CubeIDE

- Abrir STM32CubeIDE
- Seleccionar **File → Open Projects from File System**
- Navegar al directorio clonado

### 3. Configurar credenciales LoRaWAN

Abrir el archivo `Wedo-Energy.ioc` y configurar:

- **Device EUI** - Identificador único del dispositivo
- **Application EUI** - Identificador de la aplicación
- **Application Key** - Clave de cifrado
- **Región LoRaWAN** - Seleccionar según tu ubicación (AU915, US915, EU868, etc.)

### 4. Compilar y programar

- Compilar el proyecto (Ctrl+B)
- Conectar el ST-LINK a los pines SWD
- Programar el dispositivo (Run → Debug)

## Comandos Downlink

El dispositivo soporta los siguientes comandos via downlink en el **puerto 85**:

| Comando | Descripción |
|---------|-------------|
| `FF 03 XX XX` | Configurar intervalo de reporte (en segundos, big-endian) |
| `FF 99 FF` | Reset del dispositivo (borra contexto LoRaWAN) |

## Estructura del proyecto

```
Wedo-Energy/
├── Core/               # Código principal de la aplicación
│   ├── Inc/            # Headers
│   └── Src/            # Código fuente
├── LoRaWAN/            # Stack LoRaWAN
├── Drivers/            # Drivers HAL y BSP
├── Middlewares/        # Middleware ST
├── parser/             # Decodificadores para TTN y ChirpStack
└── Wedo-Energy.ioc     # Configuración STM32CubeMX
```

## Parsers

En la carpeta `parser/` se incluyen decodificadores de payload para:

- **The Things Network V3** (`ttn_decoder.js`)
- **ChirpStack V4** (`chirpstack_v4_decoder.js`)

## Licencia

Ver [LICENSE.md](LICENSE.md) para más detalles.

---

Desarrollado por [Wedo IoT](https://github.com/we-do-iot)
