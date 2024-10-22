#include "DHT.h" // biblioteca do sensor DHT

#define pindht 2           // Pino conectado ao sensor DHT22
#define DHTTYPE DHT22      // Tipo do sensor (DHT22)
#define pinValvula 5       // Pino que controla a válvula de irrigação
#define pinbotao 0         // Pino conectado ao botão de controle manual
#define leituras 24        // Quantidade de leituras para cálculo da média de ET
#define pinled 13          // Pino que controla o LED (indicador de modo manual)


float valores_ET[leituras]; // Array para armazenar os valores de evapotranspiração (ET)
int indiceET = 0;           // Índice atual de leitura no array de ET
unsigned long tempodecorrido = 0;  // Tempo desde a última leitura
unsigned long tempoligado = 0;     // Tempo que a válvula está ligada
float TI = 0;                      // Tempo necessário para irrigação (em minutos)
boolean estadoBotao = true;         // Estado atual do botão (pressionado ou não)
boolean estadoAntbotao = true;      // Estado anterior do botão (para detectar mudança)
boolean estadoDesligado = false;    // Indica se o sistema está desligado manualmente

// Instancia o sensor DHT com o pino e o tipo definidos
DHT dht(pindht, DHTTYPE);

void setup() {
    
    Serial.begin(9600);
    Serial.println(F("Controle de irrigação por evapotranspiração!")); 

    dht.begin();  // Inicializa o sensor DHT22

    // Configura os pinos como entrada ou saída
    pinMode(pinValvula, OUTPUT);  // Válvula como saída
    pinMode(pinbotao, INPUT);     // Botão como entrada
    pinMode(pinled, OUTPUT);      // LED como saída
}

void loop() {
    // Lê o estado do botão para alternar entre modo manual (ligado/desligado)
    estadoBotao = digitalRead(pinbotao);  
    if (!estadoBotao && estadoAntbotao) {  
        estadoDesligado = !estadoDesligado;  
    }

    // Se o sistema está desligado manualmente
    if (estadoDesligado) {
        digitalWrite(pinled, HIGH);  // Acende o LED indicando o modo desligado
        digitalWrite(pinValvula, LOW);  // Garante que a válvula está desligada
    } else {
        digitalWrite(pinled, LOW);  // Apaga o LED
    }

    estadoAntbotao = estadoBotao;  // Atualiza o estado anterior do botão

    // Verifica se passou 1 segundo desde a última leitura
    if (millis() - tempodecorrido >= 1000) {
        float h = dht.readHumidity();  // Lê a umidade do sensor
        float t = dht.readTemperature();  // Lê a temperatura

        // Se as leituras são válidas
        if (!isnan(h) && !isnan(t)) {
            // Exibe as leituras no monitor serial
            Serial.print(F("Umidade: "));
            Serial.print(h);
            Serial.print(F("% "));
            Serial.print(F("Temperatura: "));
            Serial.print(t);
            Serial.print(F("ºC "));
            Serial.print(F("Índice: "));
            Serial.println(indiceET);

            // Cálculo da evapotranspiração (ET)
            float kl = 0.71;  // Coeficiente de localidade
            float kc = 0.40;  // Coeficiente de cultura
            float es = 0.6108 * exp((17.27 * t) / (t + 237.3));  // Pressão de saturação
            float ea = (es * h) / 100;  // Pressão real de vapor
            float et0 = (2.5982 * pow((1 + (t / 25)), 2) * (1 - (ea / es))) + 0.797;  // ET base
            float et = et0 * kc * kl;  // ET ajustada para a cultura e localidade

            // Armazena a leitura de ET no array
            valores_ET[indiceET] = et;

            // Se é a última leitura do ciclo
            if (indiceET == (leituras - 1)) {
                float soma_et = 0;
                // Calcula a soma de todas as leituras de ET
                for (int i = 0; i < leituras; i++) {
                    soma_et += valores_ET[i];
                }

                // Exibe a ET média no monitor serial
                Serial.print(F("Evapotranspiração Média = "));
                Serial.println(soma_et / leituras);

                // Cálculo do tempo de irrigação
                float N = 1;  // Número de aspersores
                float q = 2.1;  // Vazão (litros/minuto)
                float A = 0.35;  // Área irrigada (m²)
                float ia = (N * q) / A;  // Intensidade de aplicação
                float LB = (soma_et / leituras) / ia * kl;  // Lâmina bruta
                TI = (LB / ia) * 60;  // Tempo de irrigação (em minutos)

                // Exibe o tempo de irrigação no monitor serial
                Serial.print(F("Tempo de irrigação: "));
                Serial.print(TI);
                Serial.println(F(" min"));

                // Liga a válvula se o sistema não estiver desligado manualmente
                if (!estadoDesligado) {
                    digitalWrite(pinValvula, HIGH);
                }

                // Reinicia as leituras e o tempo ligado
                indiceET = 0;
                tempoligado = 0;
            } else {
                indiceET++;  // Avança para a próxima leitura
            }

            tempodecorrido = millis();  // Atualiza o tempo da última leitura
        }
    }

    // Desliga a válvula após o tempo necessário de irrigação
    if (tempoligado >= TI * 60000) {  // Converte minutos para milissegundos
        digitalWrite(pinValvula, LOW);  // Desliga a válvula
        tempoligado = 0;  // Reinicia o tempo ligado
        TI = 0;  // Zera o tempo de irrigação
    }

    tempoligado += 10;  // Incrementa o tempo ligado
    delay(10);  //pequeno delay para evitar um loop muito rápido
}

