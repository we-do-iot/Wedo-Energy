# Guía para Proteger Código Personalizado de STM32CubeMX

## Problema
Cuando regeneras el código con el archivo `.ioc`, STM32CubeMX sobrescribe ciertos archivos y pierde configuraciones personalizadas.

## Regla General
**SOLO el código entre `/* USER CODE BEGIN */` y `/* USER CODE END */` se preserva durante la regeneración.**

---

## 1. lora_app.c - Defines de Configuración

### ❌ Problema
Los defines se generan fuera de las secciones USER CODE:
```c
#define LED_PERIOD_TIME 500
#define JOIN_TIME 2000
#define LORAWAN_NVM_BASE_ADDRESS ((void *)0x0803F000UL)
```

### ✅ Solución Actual (Ya Implementada)
Ya tienes la solución correcta en las líneas 48-59:

```c
/* USER CODE BEGIN Includes */
#ifndef LED_PERIOD_TIME
#define LED_PERIOD_TIME 200U
#endif

#ifndef JOIN_TIME
#define JOIN_TIME 600000U
#endif

#ifndef LORAWAN_NVM_BASE_ADDRESS
#define LORAWAN_NVM_BASE_ADDRESS ((void *)0x080E0000)
#endif
/* USER CODE END Includes */
```

**Nota**: Estos valores NO se pueden configurar desde el archivo .ioc. Esta es la única forma de protegerlos.

---

## 2. adc_if.h - Configuración de Batería

### ❌ Problema
CubeMX regenera automáticamente estos defines fuera de USER CODE:
```c
#define BAT_CR2032    ((uint32_t) 3000)
#define VDD_BAT       BAT_CR2032
#define VDD_MIN       1800
```

### ✅ Solución Definitiva: Usar #undef para Redefinir

**Ubicación**: Entre líneas 55-74 (`/* USER CODE BEGIN EC */` y `/* USER CODE END EC */`)

```c
/* USER CODE BEGIN EC */

/**
  * IMPORTANTE: CubeMX genera automáticamente defines para batería CR2032 arriba.
  * Los redefinimos aquí para usar valores de batería Li-ion.
  * Usamos #undef para evitar warnings de redefinición.
  */

/* Primero eliminamos las definiciones generadas por CubeMX */
#undef VDD_BAT
#undef VDD_MIN

/**
  * @brief Typical Li-ion battery nominal values (mV)
  */
#define BAT_LI_ION                  ((uint32_t) 4200)

/**
  * @brief Maximum battery level in mV (Li-ion)
  * REDEFINIDO: CubeMX genera VDD_BAT = BAT_CR2032, nosotros usamos BAT_LI_ION
  */
#define VDD_BAT                     BAT_LI_ION

/**
  * @brief Minimum battery level in mV for reporting (below this is considered empty)
  * REDEFINIDO: CubeMX genera VDD_MIN = 1800, nosotros usamos 3000 para Li-ion
  */
#define VDD_MIN                     3000

/* USER CODE END EC */
```

**Por qué funciona**: 
1. CubeMX genera los defines para CR2032 arriba (fuera de USER CODE)
2. Dentro de USER CODE, usamos `#undef` para eliminar esas definiciones
3. Luego redefinimos con nuestros valores de Li-ion
4. El compilador usa las últimas definiciones (las nuestras)

**Importante**: NO intentes eliminar los defines que CubeMX genera arriba, porque los volverá a crear en la próxima regeneración.

---

## 3. sys_app.c - Función GetBatteryLevel()

### ❌ Problema
CubeMX elimina el código personalizado de conversión de batería y lo reemplaza con un cálculo lineal simple.

### ✅ Solución
Tu código personalizado YA ESTÁ en la sección USER CODE correcta (líneas 170-225):

```c
/* USER CODE BEGIN GetBatteryLevel_2 */

/* Map battery mV to a percentage using a piecewise-linear Li-ion curve (typical) */
{
  const uint16_t volts[] = {3000, 3200, 3400, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
  const uint8_t  pc[]    = {   0,    5,   20,   50,   65,   75,   85,   93,   98,  100};
  // ... resto del código
}

/* USER CODE END GetBatteryLevel_2 */
```

**Nota**: Este código DEBE permanecer entre `USER CODE BEGIN GetBatteryLevel_2` y `USER CODE END GetBatteryLevel_2`.

### ⚠️ Verificar
Asegúrate de que NO haya código duplicado fuera de las secciones USER CODE. El código entre las líneas 162-168 es el código generado por CubeMX y debe permanecer, pero tu lógica personalizada debe estar solo en USER CODE.

---

## 4. Workflow Recomendado

### Antes de Regenerar con CubeMX:

1. **Hacer commit de Git** de todos los cambios actuales
2. **Verificar** que todo el código personalizado esté en secciones USER CODE
3. **Documentar** cualquier cambio manual que hayas hecho

### Después de Regenerar con CubeMX:

1. **Revisar diferencias** con `git diff`
2. **Verificar** que las secciones USER CODE se preservaron
3. **Compilar y probar** para asegurar que todo funciona

### Si CubeMX Sobrescribe Algo:

1. **Usar `git diff`** para ver qué cambió
2. **Restaurar** el código desde el commit anterior
3. **Mover** el código a la sección USER CODE apropiada
4. **Regenerar** nuevamente

---

## 5. Secciones USER CODE Disponibles

### En archivos .c:
- `USER CODE BEGIN Header` - Comentarios de encabezado
- `USER CODE BEGIN Includes` - Includes adicionales
- `USER CODE BEGIN PV` - Variables privadas
- `USER CODE BEGIN PFP` - Prototipos de funciones privadas
- `USER CODE BEGIN 0` - Código antes de main()
- `USER CODE BEGIN 1` - Código al inicio de main()
- `USER CODE BEGIN 2` - Código al final de main()
- `USER CODE BEGIN FunctionName_X` - Dentro de funciones específicas

### En archivos .h:
- `USER CODE BEGIN Includes` - Includes adicionales
- `USER CODE BEGIN EC` - Constantes exportadas
- `USER CODE BEGIN ET` - Tipos exportados
- `USER CODE BEGIN EFP` - Prototipos de funciones exportadas
- `USER CODE BEGIN EM` - Macros exportadas

---

## 6. ⚠️ IMPORTANTE: NO Puedes Crear Tus Propias Secciones USER CODE

### ❌ Esto NO Funciona:

```c
/* USER CODE BEGIN MiSeccionPersonalizada */
#define MI_VALOR 123
/* USER CODE END MiSeccionPersonalizada */
```

### ¿Por Qué?

1. **STM32CubeMX mantiene un template interno** de cada archivo que genera
2. **Solo reconoce las secciones USER CODE que él mismo creó** durante la generación inicial
3. **Cuando regenera**, busca esas secciones específicas por nombre
4. **Cualquier sección USER CODE que agregues manualmente será IGNORADA y sobrescrita**

### ✅ Soluciones Alternativas:

#### Opción 1: Usar Secciones Existentes
Coloca tu código en las secciones USER CODE que ya existen en el archivo.

#### Opción 2: Crear Archivos Separados
Si necesitas mucho código personalizado, créalo en archivos separados que CubeMX no toque:

```
LoRaWAN/App/
├── lora_app.c          (generado por CubeMX)
├── lora_app.h          (generado por CubeMX)
├── obis_helpers.c      (TU archivo, CubeMX no lo toca)
└── obis_helpers.h      (TU archivo, CubeMX no lo toca)
```

#### Opción 3: Usar #undef para Redefinir
Como vimos en `adc_if.h`, puedes usar `#undef` dentro de USER CODE para redefinir valores generados por CubeMX.

### 📋 Archivos que CubeMX Genera y Mantiene:

CubeMX **SOLO** agrega secciones USER CODE en archivos que él genera:
- ✅ `main.c`, `main.h`
- ✅ `stm32xxxx_it.c`, `stm32xxxx_it.h`
- ✅ Archivos de periféricos (ej: `usart.c`, `gpio.c`)
- ✅ Archivos de middleware (ej: `lora_app.c`, `sys_app.c`)
- ❌ Archivos que TÚ creas manualmente (CubeMX no los toca)

---

## 7. Resumen de Acciones Necesarias

### ✅ Ya Protegido (No requiere acción):
- [x] `lora_app.c` - Defines con guards `#ifndef`
- [x] `adc_if.h` - Usando `#undef` para redefinir valores de batería
- [x] `sys_app.c` - Función `GetBatteryLevel()` personalizada

---

## 8. Notas Importantes

1. **No se puede configurar desde .ioc**: Los valores de batería, timers personalizados, y direcciones de memoria NVM no tienen opciones en la interfaz gráfica de CubeMX.

2. **Siempre usa Git**: Antes de regenerar con CubeMX, haz commit de tus cambios.

3. **Verifica después de regenerar**: Siempre revisa los cambios con `git diff` después de regenerar.

4. **Documentación**: Mantén este archivo actualizado con cualquier otro código que necesites proteger.

5. **NO puedes crear secciones USER CODE personalizadas**: Solo funcionan las que STM32CubeMX coloca automáticamente.

---

## 9. Referencias

- [STM32CubeMX User Manual](https://www.st.com/resource/en/user_manual/um1718-stm32cubemx-for-stm32-configuration-and-initialization-c-code-generation-stmicroelectronics.pdf)
- Sección sobre "USER CODE sections"

