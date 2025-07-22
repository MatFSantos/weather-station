#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/html_page.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#include "lib/ssd1306.h"
#include "lib/push_button.h"
#include "lib/ws2812b.h"
#include "lib/led.h"
#include "lib/aht20.h"
#include "lib/bmp280.h"
#include "lib/pwm.h"
#include <math.h>

// WIFI credentials
#include "credenciais.h"

/** pin definitions */
#define LED_PIN CYW43_WL_GPIO_LED_PIN /* LED of the rp pico w board */

#define I2C0_PORT i2c0                 // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C0_SDA 0                     // 0 ou 2
#define I2C0_SCL 1                     // 1 ou 3
#define SEA_LEVEL_PRESSURE 101325.0   // Pressão ao nível do mar em Pa

#define BUZZER 21
#define PWM_DIVISER 20
#define PWM_WRAP 2000 // aprox 3.5kHz freq

/** global variables */
double temp = 0.0;      // °C
double max_temp = 50.0; // critical level of the temperature
double min_temp = 0; // critical level of the temperature
double offset_temp = 0.0; // offset of the temperature

double hum = 0.0;        // %
double max_hum = 100;   // critical level of the humidity
double min_hum = 0;   // critical level of the humidity
double offset_hum = 0.0; // offset of the humidity

double pres = 0.0;        // kPa
double max_pres = 150;   // critical level of the pressure
double min_pres = 0;   // critical level of the pressure
double offset_pres = 0.0; // offset of the pressure
static volatile bool screen = false; // Flag to indicate if the button was pressed
static volatile uint32_t last_time = 0;

ssd1306_t ssd;
_ws2812b *ws; /* 5x5 matrix */

/**
 * @brief Function callback to close tcp conection
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);

/**
 * @brief Function callback called to accept tcp conection
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

/**
 * @brief Function to make the response string according GET route called
 *
 * @param request The received request data
 * @param buffer point to buffer string that will be used to send data
 * @param buffer_size buffer size
 */
void user_request(char **request, char *buffer, size_t buffer_size);

/**
 * @brief Function callback used to process the received request data
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

/**
 * @brief Initialize the SSD1306 display
 *
 */
void init_display(void);

/**
 * @brief Initialize the all GPIOs that will be used in project
 *      - I2C;
 *      - Blue LED;
 */
void init_gpio(void);

/**
 * @brief Update the display informations
 *
 * @param message the message that will be ploted in display OLED
 * @param y position on vertical that the message will be ploted
 * @param clear if the display must be cleaned
 *
 */
void update_display(char * message, uint8_t y, bool clear);

/**
 * @brief Function to read the humidity sensor
 *
 */
void read_humidity();

/**
 * @brief Function to read the temperature sensor
 * 
 * @param params Pointer to the BMP280 calibration parameters structure
 *
 */
void read_temperature(struct bmp280_calib_param *params);

/**
 * @brief Function to read the pressure sensor
 *
 * @param params Pointer to the BMP280 calibration parameters structure
 */
void read_pressure(struct bmp280_calib_param * params);

/**
 * @brief Handler function to interruption
 *
 * @param gpio GPIO that triggered the interruption
 * @param event The event which caused the interruption
 */
void gpio_irq_handler(uint gpio, uint32_t event)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time >= 200) { // debounce
        screen = !screen; // toggle screen flag
        last_time = current_time;
    }
}

int main() {
    // set clock freq to 128MHz
    bool ok = set_sys_clock_khz(128000, false);
    if (ok)
        printf("clock set to %ld\n", clock_get_hz(clk_sys));
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();
    init_gpio();

    // get ws and ssd struct
    init_display();
    update_display("WEBSERVER", 18, true);
    update_display("Iniciando...", 28, false);

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        update_display("Falha", 18, true);
        update_display("iniciar WiFi", 28, false);
        update_display("Aguarde...", 38, false);
        sleep_ms(1000);
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        update_display("WiFi", 18, true);
        update_display("Conectando", 28, false);
        update_display("Aguarde...", 38, false);
        sleep_ms(200);
    }
    update_display("Conectado WiFi", 8, true);

    if (netif_default)
        update_display(ipaddr_ntoa(&netif_default->ip_addr), 18, false);

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        update_display("Falha", 18, true);
        update_display("Criar server", 28, false);
        update_display("Reinicie o RP", 38, false);
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        update_display("Falha", 18, true);
        update_display("Abrir server", 28, false);
        update_display("Reinicie o RP", 38, false);
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    update_display("PORT 80", 28, false);


    // Inicializa o I2C
    i2c_init(I2C0_PORT, 400 * 1000);
    gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA);
    gpio_pull_up(I2C0_SCL);

    // Inicializa o BMP280
    bmp280_init(I2C0_PORT);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C0_PORT, &params);

    // Inicializa o AHT20
    aht20_reset(I2C0_PORT);
    aht20_init(I2C0_PORT);

    // init 5x5 matrix
    PIO pio = pio0;
    ws = init_ws2812b(pio, PIN_WS2812B);
    ws2812b_turn_off(ws);

    // configure interruptions handlers
    gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);

    while (true) {
        read_temperature(&params);
        read_humidity();
        read_pressure(&params);

        if (temp > max_temp || temp < min_temp) { // verifica nível crítico da temperatura
            gpio_put(PIN_RED_LED, 1);
            gpio_put(PIN_GREEN_LED, 0);
            ws2812b_plot(ws, &RED_TL);
            pwm_set_gpio_level(BUZZER, PWM_WRAP / 2); // ativa o buzzer
        } else {
            if (hum > max_hum || hum < min_hum) { // verifica nível crítico da umidade
                gpio_put(PIN_RED_LED, 1);
                gpio_put(PIN_GREEN_LED, 0);
                ws2812b_plot(ws, &RED_TL);
                pwm_set_gpio_level(BUZZER, PWM_WRAP / 2); // ativa o buzzer
            } else {
                if (pres > max_pres || pres < min_pres) { // verifica nível crítico da pressão
                    gpio_put(PIN_RED_LED, 1);
                    gpio_put(PIN_GREEN_LED, 0);
                    ws2812b_plot(ws, &RED_TL);
                    pwm_set_gpio_level(BUZZER, PWM_WRAP / 2); // ativa o buzzer
                } else {
                    gpio_put(PIN_RED_LED, 0); // Desliga o LED se tudo estiver dentro dos limites
                    gpio_put(PIN_GREEN_LED, 1); // Liga o LED verde se tudo estiver dentro dos limites
                    ws2812b_plot(ws, &GREEN_TL);
                    pwm_set_gpio_level(BUZZER, 0); // ativa o buzzer
                }
            }
        }

        if (screen) {
            update_display(NULL, -1, true);
        } else {
            update_display("Conectado WiFi", 8, true);
            update_display(ipaddr_ntoa(&netif_default->ip_addr), 18, false);
            update_display("PORT 80", 28, false);
        }
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        sleep_ms(500);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Functions --------------------------------- //

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("Data sent successfully, length: %d\n", len);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);   // callback to received
    tcp_sent(newpcb, tcp_sent_callback); // callback to sent
    return ERR_OK;
}

void user_request(char **request, char *buffer, size_t buffer_size) {
    if (strncmp(*request, "GET /data", strlen("GET /data")) == 0) { // rota para capturar os dados que são mostrados na tela
        char json[256];
        snprintf(
            json,
            sizeof(json),
            "{\"temp\":%.2f,\"hum\":%.2f,\"pres\":%.2f}",
            temp,
            hum,
            pres);
        snprintf(buffer, buffer_size,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(json), json
        );
    } else if (strncmp(*request, "GET / ", strlen("GET / ")) == 0) { // rota home, para retorno da página html
        snprintf(buffer, buffer_size, html_page);
    } else if (strncmp(*request, "POST /config ", strlen("POST /config ")) == 0) {
        char *body = strstr(*request, "\r\n\r\n");
        if (body) {
            body += 4; // pula os "\r\n\r\n"
            printf("Received config: %s\n", body);

            int temp_min, temp_max, temp_offset;
            int hum_min, hum_max, hum_offset;
            int pres_min, pres_max, pres_offset;

            int parsed = sscanf(body,
                   "{\"temp_min\":%d,\"temp_max\":%d,\"offset_temp\":%d,"
                   "\"hum_min\":%d,\"hum_max\":%d,\"offset_hum\":%d,"
                   "\"pres_min\":%d,\"pres_max\":%d,\"offset_pres\":%d}",
                   &temp_min, &temp_max, &temp_offset,
                   &hum_min, &hum_max, &hum_offset,
                   &pres_min, &pres_max, &pres_offset);
            
            if (parsed == 9) {
                // Atualiza as variáveis globais com os valores recebidos
                double test = 40.0; 
                min_temp = (double) temp_min;
                max_temp = (double) temp_max;
                offset_temp = (double) temp_offset;

                min_hum = (double)hum_min;
                max_hum = (double) hum_max;
                offset_hum = (double) hum_offset;

                min_pres = (double) pres_min;
                max_pres = (double) pres_max;
                offset_pres = (double) pres_offset;

                char json[256];
                snprintf(
                    json,
                    sizeof(json),
                    "{\"max_temp\":%.2f,\"min_temp\":%.2f,\"offset_temp\":%.2f,\"max_hum\":%.2f,\"min_hum\":%.2f,\"offset_hum\":%.2f,\"max_pres\":%.2f,\"min_pres\":%.2f,\"offset_pres\":%.2f}",
                    max_temp,
                    min_temp,
                    offset_temp,
                    max_hum,
                    min_hum,
                    offset_hum,
                    max_pres,
                    min_pres,
                    offset_pres);
                snprintf(buffer, buffer_size,
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Content-Length: %lu\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         strlen(json), json);
            } else {
                const char *json = "{\"message\":\"Dados malformados\"}";
                snprintf(buffer, buffer_size, // Formatar uma string e armazená-la em um buffer de caracteres
                         "HTTP/1.1 400 BAD REQUEST\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: %lu\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         strlen(json), json);
            }
        } else {
            const char *json = "{\"message\":\"Requisição malformada\"}";
            snprintf(buffer, buffer_size, // Formatar uma string e armazená-la em um buffer de caracteres
                     "HTTP/1.1 400 BAD REQUEST\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %lu\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "%s",
                     strlen(json), json);
        }
    } else if (strncmp(*request, "GET /config ", strlen("GET /config ")) == 0) {
        char json[256];
        snprintf(
            json,
            sizeof(json),
            "{\"max_temp\":%.2f,\"min_temp\":%.2f,\"offset_temp\":%.2f,\"max_hum\":%.2f,\"min_hum\":%.2f,\"offset_hum\":%.2f,\"max_pres\":%.2f,\"min_pres\":%.2f,\"offset_pres\":%.2f}",
            max_temp,
            min_temp,
            offset_temp,
            max_hum,
            min_hum,
            offset_hum,
            max_pres,
            min_pres,
            offset_pres);
        snprintf(buffer, buffer_size,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Length: %lu\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 strlen(json), json);
    } else { // caso não encontre a rota chamada, retorna 404
        const char *json = "{\"message\":\"Unknown route\"}";
        snprintf(buffer, buffer_size, // Formatar uma string e armazená-la em um buffer de caracteres
            "HTTP/1.1 404 NOT FOUND\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(json), json
        );
    }
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // monta resposta
    char res[4096 * 2];
    user_request(&request, res, 4096 * 2);

    tcp_write(tpcb, res, strlen(res), TCP_WRITE_FLAG_COPY); // escreve para envio
    tcp_output(tpcb); //envia dados

    free(request); // libera ponteiro alocado
    pbuf_free(p); // libera um buffer de pacote (pbuf) alocado

    return ERR_OK;
}

void init_display() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, I2C_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

void init_gpio() {
    /** initialize i2c communication */
    int baudrate = 400 * 1000; // 400kHz baud rate for i2c communication
    i2c_init(I2C_PORT, baudrate);

    // set GPIO pin function to I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL);

    /** initialize RED and GREEN LED */
    gpio_init(PIN_RED_LED);
    gpio_set_dir(PIN_RED_LED, GPIO_OUT);
    gpio_init(PIN_GREEN_LED);
    gpio_set_dir(PIN_GREEN_LED, GPIO_OUT);

    /** initialize buttons */
    init_push_button(PIN_BUTTON_A);

    /** initialize buzzer */
    pwm_init_(BUZZER);
    pwm_setup(BUZZER, PWM_DIVISER, PWM_WRAP);
    pwm_start(BUZZER, 0);
}

void update_display(char * message, uint8_t y, bool clear) {
    if(clear)
        ssd1306_fill(&ssd, false);
    
    if (!screen) {
        ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
        ssd1306_draw_string(&ssd, message, 64 - (strlen(message) * 4), y);
    } else {
        char buff[256];
        ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);

        snprintf(buff, sizeof(buff), "Temp: %.2f", temp);
        ssd1306_draw_string(&ssd, buff, 4, 4);

        snprintf(buff, sizeof(buff), "Hum: %.2f", hum);
        ssd1306_draw_string(&ssd, buff, 4, 14);

        snprintf(buff, sizeof(buff), "Pres: %.2f", pres);
        ssd1306_draw_string(&ssd, buff, 4, 24);
    }
    ssd1306_send_data(&ssd); // update display
}

void read_humidity() {
    AHT20_Data data;
    if (aht20_read(I2C0_PORT, &data)) {
        hum = data.humidity + offset_hum; // aplica o offset
    } else {
        printf("Erro na leitura do AHT10!\n\n\n");
        hum = offset_hum; // se falhar, usa apenas a temperatura do BMP280
    }   
}

void read_temperature(struct bmp280_calib_param* params) {
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;
    bmp280_read_raw(I2C0_PORT, &raw_temp_bmp, &raw_pressure);

    int32_t temperature = bmp280_convert_temp(raw_temp_bmp, params);
    float temp_bmp = temperature / 100.0f; // Convertendo para °C

    if (aht20_read(I2C0_PORT, &data)) {
        temp = (temp_bmp + data.temperature ) / 2; // media das temperaturas
    } else {
        printf("Erro na leitura do AHT10!\n\n\n");
        temp = temp_bmp; // se falhar, usa apenas a temperatura do BMP280
    }
    temp = temp + offset_temp; // aplica o offset
}

void read_pressure(struct bmp280_calib_param * params) {
    int32_t raw_temp_bmp;
    int32_t raw_pressure;
    bmp280_read_raw(I2C0_PORT, &raw_temp_bmp, &raw_pressure);
    int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, params);
    pres = pressure / 1000.0f;
    pres = pres + offset_pres; // aplica o offset
}