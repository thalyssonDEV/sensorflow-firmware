#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"

#include "inc/bmp280/bmp280.h"
#include "inc/aht10/aht10.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

#define SEND_INTERVAL_MS 5000

// --- ESTRUTURA PARA GERIR O ESTADO DA CONEXÃO ---
typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *pcb;
    ip_addr_t remote_addr;
    bool complete;
    char http_response[1024];
    float temperature;
    float humidity;
    float pressure;
    char board_id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
} TCP_CLIENT_T;


// --- CALLBACKS DA PILHA TCP/IP ---
static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T *)arg;
    state->complete = true; // Sinaliza para o loop de espera terminar
    if (state->pcb != NULL) {
        tcp_arg(state->pcb, NULL);
        tcp_poll(state->pcb, NULL, 0);
        tcp_sent(state->pcb, NULL);
        tcp_recv(state->pcb, NULL);
        tcp_err(state->pcb, NULL);
        tcp_close(state->pcb);
        state->pcb = NULL;
    }
    return ERR_OK;
}

static err_t callback_response_received(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T *)arg;
    if (!p) {
        printf("[INFO] Conexao fechada pelo servidor.\n");
        return tcp_client_close(state);
    }
    if (p->tot_len > 0) {
        u16_t copy_len = p->tot_len > sizeof(state->http_response) - 1 ? sizeof(state->http_response) - 1 : p->tot_len;
        pbuf_copy_partial(p, state->http_response, copy_len, 0);
        state->http_response[copy_len] = '\0';
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    
    return tcp_client_close(state);
}

static err_t callback_connected(void *arg, struct tcp_pcb *pcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T *)arg;
    if (err != ERR_OK) {
        printf("[ERRO] Falha na conexao TCP: %d\n", err);
        return tcp_client_close(state);
    }
    printf("[OK]   Conexao TCP estabelecida.\n");
    tcp_recv(pcb, callback_response_received);

    char json_body[256];
    snprintf(json_body, sizeof(json_body),
             "{\"temperature\":%.2f, \"humidity\":\"%.2f\", \"pressure\":\"%.2f\", \"sensor_id\":\"%s\"}",
             state->temperature, state->humidity, state->pressure, state->board_id);

    printf("[DADOS] Preparando o seguinte JSON para envio:\n       -> %s\n", json_body);

    char request[1024];
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "x-api-key: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             TARGET_PATH, TARGET_SERVER_IP, API_KEY, (int)strlen(json_body), json_body);
    
    printf("[INFO] Enviando requisicao POST...\n");
    cyw43_arch_lwip_begin();
    err_t write_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (write_err == ERR_OK) {
        tcp_output(pcb);
        printf("[OK]   Requisicao enviada. Aguardando resposta.\n");
    } else {
        printf("[ERRO] Falha ao enviar dados TCP: %d\n", write_err);
        cyw43_arch_lwip_end();
        return tcp_client_close(state);
    }
    cyw43_arch_lwip_end();
    return ERR_OK;
}

void send_data_to_cloud(float temp, float hum, float pres, const char* board_id) {
    printf("\n****************************************************\n");
    printf("* INICIANDO CICLO DE ENVIO                     *\n");
    printf("****************************************************\n");
    
    // 1. Aloca memória para o estado da conexão
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        printf("[ERRO] Falha ao alocar memoria para o estado.\n");
        return;
    }
    state->temperature = temp;
    state->humidity = hum;
    state->pressure = pres;
    strncpy(state->board_id, board_id, sizeof(state->board_id) - 1);

    printf("[INFO] Tentando conectar ao servidor %s:%d...\n", TARGET_SERVER_IP, TARGET_PORT);
    
    ip4addr_aton(TARGET_SERVER_IP, &state->remote_addr);
    state->pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->pcb) {
        printf("[ERRO] Falha ao criar o PCB.\n");
        free(state); // Liberta a memória se a criação do PCB falhar
        return;
    }

    tcp_arg(state->pcb, state);
    
    cyw43_arch_lwip_begin();
    err_t error = tcp_connect(state->pcb, &state->remote_addr, TARGET_PORT, callback_connected);
    cyw43_arch_lwip_end();

    if (error != ERR_OK) {
        printf("[ERRO] Falha ao iniciar conexao TCP: %d\n", error);
        // A função tcp_client_close não será chamada, então libertamos a memória aqui
        free(state);
        return;
    }

    // Loop de espera síncrono
    int timeout_s = 20;
    while (!state->complete && timeout_s-- > 0) {
        cyw43_arch_poll();
        sleep_ms(1000);
    }

    // 2. Liberta a memória no final do ciclo, independentemente do resultado
    if (!state->complete) {
        printf("[ERRO] Timeout no ciclo de envio!\n");
        tcp_abort(state->pcb); // Aborta a conexão em caso de timeout
    }
    
    printf("[INFO] Ciclo de envio concluido. A libertar memoria.\n");
    free(state); // A correção principal está aqui!
}

int wifi_init() {
    printf("[INFO] Inicializando Wi-Fi...\n");
    if (cyw43_arch_init()) {
        printf("[ERRO] Falha na inicializacao do chip Wi-Fi.\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("[INFO] Conectando a rede \"%s\"...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("[ERRO] Nao foi possivel conectar ao Wi-Fi.\n");
        return 1;
    }
    printf("[OK]   Wi-Fi conectado com sucesso!\n");
    printf("[INFO] Endereco IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    return 0;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    char board_id_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(board_id_str, sizeof(board_id_str));

    printf("\n====================================================\n");
    printf("=           MONITOR DE SENSOR | RASPBERRY PI PICO      =\n");
    printf("= ID Unico da Placa: %s =\n", board_id_str);
    printf("====================================================\n");

    if (wifi_init() != 0) {
        while(1);
    }

    bmp280_init();
    aht10_init();

    printf("[INFO] Iniciando ciclos de envio a cada %d segundos.\n", SEND_INTERVAL_MS / 1000);
    
    while (1) {
        bmp280_data_t bmp_data;
        bmp280_read(&bmp_data);
        float humidity = aht10_read_humidity();

        printf("\nDados lidos -> Temp: %.2f C | Hum: %.2f %% | Pres: %.2f hPa\n", 
               bmp_data.temperature_c, 
               humidity,
               bmp_data.pressure_hpa);
        
        send_data_to_cloud(bmp_data.temperature_c, humidity, bmp_data.pressure_hpa, board_id_str);
        
        printf("\nA aguardar %d segundos para o proximo envio...\n", SEND_INTERVAL_MS / 1000);
        sleep_ms(SEND_INTERVAL_MS);
    }
    return 0;
}
