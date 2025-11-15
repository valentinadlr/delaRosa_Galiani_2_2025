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
 * |  Rele (Bomba)  |   GPIO_16         |
 * |  Buffer        |   GPIO_17         |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Valentina de la Rosa y Fiorella Galiani (valentina.delarosa@ingenieria.uner.edu.ar) (fiorella.galiani@ingenieria.uner.edu.ar)
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
#include "uart_mcu.h"
#include "esp_log.h"
#include "buzzer.h"
#include "buzzer_melodies.h"

#define RELE_PIN GPIO_19     
#define NEOPIXEL_PIN GPIO_18 
#define NEOPIXEL_LEN 12
#define VASO_UMBRAL_CM 5
#define SEGUNDOS_ENTRE_DESCARGAS 1800 // 30 minutos
#define TIEMPO_REFRESCO_CONTADOR 1    // 1 segundo

static neopixel_color_t colores[NEOPIXEL_LEN];
static uint32_t contador_segundos = 0;

int putchar(int c) {
    char ch = (char)c;
    UartSendByte(UART_PC, &ch);
    return c;
}

void BombaOn(void)
{
    GPIOState(RELE_PIN, false);
    printf("💡 Bomba encendida\n");
}

void BombaOff(void)
{
    GPIOState(RELE_PIN, true); // apaga el relé
    printf("💡 Bomba apagada\n");
}

void DispensarAgua(void)
{
    printf("💧 DISPENSANDO AGUA\n");
    BombaOn();
    NeoPixelAllColor(NEOPIXEL_COLOR_CYAN);
    DelaySec(25); // tiempo que dispensa el agua
    NeoPixelAllColor(NEOPIXEL_COLOR_ROSE);
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
                BuzzerPlayRtttl(songSpiderman);
            }
        }
        printf("%lu\n", contador_segundos);
        DelaySec(TIEMPO_REFRESCO_CONTADOR); 
    }
}

/**
 * @brief Tarea encargada de leer las teclas.
 * Tecla 1: dispensar ahora.
 * Tecla 2: reiniciar el contador.
 */
void TareaTeclas(void *pvParameters)
{
    int8_t estado;

    while (1)
    {
        estado = SwitchesRead();

        switch (estado)
        {
        case SWITCH_1:
            printf("Tenemos sed? Esta bien!\n");
            contador_segundos = SEGUNDOS_ENTRE_DESCARGAS;
            break;

        case SWITCH_2:
            printf("Salteamos este ciclo\n");
            contador_segundos = 0;
            break;

        default:
            break;
        }

        DelayMs(200);
    }
}


int my_vprintf(const char *fmt, va_list args)
{
    char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

    UartSendBuffer(UART_PC, buffer, len); 
    return len;
}

void app_main(void)
{
    serial_config_t uart_cfg = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = UART_NO_INT,
        .param_p = NULL};

    UartInit(&uart_cfg);

    esp_log_set_vprintf(my_vprintf); //saca por la uart todos los print 

    printf("🚰 Iniciando Mini Dispenser Inteligente...\n");

    HcSr04Init(GPIO_3, GPIO_2); // echo, trigger
    SwitchesInit();

    GPIOInit(RELE_PIN, GPIO_OUTPUT);
    GPIOOn(RELE_PIN);

    BuzzerInit(GPIO_17);
    BuzzerOn();


    NeoPixelInit(NEOPIXEL_PIN, NEOPIXEL_LEN, colores);
    NeoPixelBrightness(80);
    NeoPixelAllColor(NEOPIXEL_COLOR_BLUE);

    xTaskCreate(TareaPrincipal, "TareaPrincipal", 2048, NULL, 1, NULL);
    xTaskCreate(TareaTeclas, "TareaTeclas", 2048, NULL, 1, NULL);
}