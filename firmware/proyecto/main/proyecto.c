/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32 EDU-CIAA  |
 * |:--------------:|:------------------|
 * |  HC-SR04 Echo  |   GPIO_3          |
 * |  HC-SR04 Trig  |   GPIO_2          |
 * |  NeoPixel      |   GPIO_8          |
 * |  Switch 1      |   GPIO_4          |
 * |  Switch 2      |   GPIO_15         |
 * |  Rele (Bomba)  |   GPIO_16          |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hc_sr04.h"
#include "switch.h"
#include "delay_mcu.h"
#include "neopixel_stripe.h"
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"

#define RELE_PIN GPIO_19 // pin para el relé
#define NEOPIXEL_PIN GPIO_18 //pin neopixel
#define NEOPIXEL_LEN 12
#define VASO_UMBRAL_CM 5
#define SEGUNDOS_ENTRE_DESCARGAS 5 // 30 minutos
#define TIEMPO_REFRESCO_CONTADOR 1 // 1 segundo

static neopixel_color_t colores[NEOPIXEL_LEN];
static uint32_t contador_segundos = 0;

void BombaOn(void)
{
    GPIOState(RELE_PIN, false);   // activa el relé
    printf("💡 Bomba encendida\n");
}

void BombaOff(void)
{
    GPIOState(RELE_PIN, true); //apaga el relé
    printf("💡 Bomba apagada\n");
}

void DispensarAgua(void)
{
    printf("💧 DISPENSANDO AGUA\n");
    BombaOn();
    DelaySec(3);
    NeoPixelAllColor(NEOPIXEL_COLOR_CYAN);
    BombaOff();
}

/**
 * @brief Tarea principal del sistema.
 * Se ejecuta continuamente cada 1 segundo y verifica si se cumple el tiempo de descarga.
 */
void TareaPrincipal(void *pvParameters)
{
    while (1)
    {
        contador_segundos++;

        // Verificar si se alcanzó el tiempo de dispensado
        if (contador_segundos >= SEGUNDOS_ENTRE_DESCARGAS)
        {
            contador_segundos = 0;
            printf("⏰ Tiempo cumplido — revisando vaso...\n");

            uint16_t distancia = HcSr04ReadDistanceInCentimeters();
            if (distancia <= VASO_UMBRAL_CM)
            {
                printf("verde");
                NeoPixelAllColor(NEOPIXEL_COLOR_GREEN);
                DispensarAgua();
            }
            else
            {
                NeoPixelAllColor(NEOPIXEL_COLOR_RED);
                printf("⚠ No hay vaso — no se dispensa\n");
            }
        }

        DelaySec(TIEMPO_REFRESCO_CONTADOR); // cada 1 segundo
    }
}

/**
 * @brief Tarea encargada de leer las teclas.
 * Tecla 1: dispensar ahora.
 * Tecla 2: reiniciar el contador.
 */
void TareaTeclas(void *pvParameters)
{
    while (1)
    {
        int8_t estado = SwitchesRead();

        if (estado & SWITCH_1)
        {
            printf("🔘 Tecla 1 presionada — dispensar inmediato\n");
            contador_segundos = SEGUNDOS_ENTRE_DESCARGAS; // fuerza evento
        }

        if (estado & SWITCH_2)
        {
            printf("🔘 Tecla 2 presionada — reiniciar contador\n");
            contador_segundos = 0;
        }

        DelayMs(200); // antirrebote ?
    }
}

void app_main(void)
{
    printf("🚰 Iniciando Mini Dispenser Inteligente...\n");

    // Inicialización de periféricos
    HcSr04Init(GPIO_3, GPIO_2); // echo, trigger
    SwitchesInit();

    // Inicialización del relé
    GPIOInit(RELE_PIN, GPIO_OUTPUT);
    GPIOOn(RELE_PIN);

    // Inicialización del neo pixel
    NeoPixelInit(NEOPIXEL_PIN, NEOPIXEL_LEN, colores);
    NeoPixelBrightness(80);
    NeoPixelAllColor(NEOPIXEL_COLOR_BLUE);

    // Crear tareas
    xTaskCreate(TareaPrincipal, "TareaPrincipal", 2048, NULL, 1, NULL);
    xTaskCreate(TareaTeclas, "TareaTeclas", 2048, NULL, 1, NULL);
}