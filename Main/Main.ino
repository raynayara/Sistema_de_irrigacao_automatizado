#include "DHT.h" 
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

#define DHTPIN 15           // Pino conectado ao sensor DHT22
#define DHTTYPE DHT22       // Tipo do sensor (DHT22)
#define pinValvula 2        // Pino que controla a válvula de irrigação
#define leituras 24         // Quantidade de leituras para cálculo da média de ET

float valores_ET[leituras]; // Array para armazenar os valores de evapotranspiração (ET)
int indiceET = 0;           // Índice atual de leitura no array de ET
unsigned long tempodecorrido = 0;  // Tempo desde a última leitura
unsigned long tempoligado = 0;     // Tempo que a válvula está ligada
float TI = 0;                      // Tempo necessário para irrigação (em minutos)

DHT dht(DHTPIN, DHTTYPE); // Instancia o sensor DHT com o pino e o tipo definidos
LiquidCrystal_PCF8574 lcd(0x27);    // Endereço padrão do display

void setup() {
  Wire.begin();             // Inicia o barramento I2C
  lcd.begin(16, 2);         // Define o display como 16x2
  lcd.setBacklight(255);    // Luz do display
  lcd.print("Iniciando...");

  dht.begin();              // Inicializa o sensor DHT22
  delay(2000);              // Aguarda a estabilização do sensor

  // Configura o pino da válvula como saída
  pinMode(pinValvula, OUTPUT);
}

void loop() {
    // Verifica se passou 1 segundo desde a última leitura
    if (millis() - tempodecorrido >= 1000) {
        float h = dht.readHumidity();       // Lê a umidade do sensor
        float t = dht.readTemperature();    // Lê a temperatura

        // Se as leituras são inválidas, exibe mensagem de erro
        if (isnan(h) || isnan(t)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Erro de leitura");
            lcd.setCursor(0, 1);
            lcd.print("do sensor DHT22");
        } else {
            // Exibe as leituras no display LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Umidade: ");
            lcd.print(h);
            lcd.print("%");

            lcd.setCursor(0, 1);
            lcd.print("Temp: ");
            lcd.print(t);
            lcd.print("C");

            // Cálculo da evapotranspiração (ET)
            float kl = 0.71;                   // Coeficiente de localidade
            float kc = 0.40;                   // Coeficiente de cultura
            float es = 0.6108 * exp((17.27 * t) / (t + 237.3));  // Pressão de saturação
            float ea = (es * h) / 100;         // Pressão real de vapor
            float et0 = (2.5982 * pow((1 + (t / 25)), 2) * (1 - (ea / es))) + 0.797;  // ET base
            float et = et0 * kc * kl;          // ET ajustada para a cultura e localidade

            // Armazena a leitura de ET no array
            valores_ET[indiceET] = et;

            // Se é a última leitura do ciclo
            if (indiceET == (leituras - 1)) {
                float soma_et = 0;
                // Calcula a soma de todas as leituras de ET
                for (int i = 0; i < leituras; i++) {
                    soma_et += valores_ET[i];
                }

                // Exibe a ET média e o tempo de irrigação no display LCD
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("ET Media:");
                lcd.print(soma_et / leituras);
                delay(5000);

                // Cálculo do tempo de irrigação
                float N = 1;                    // Número de aspersores
                float q = 2.1;                  // Vazão (litros/minuto)
                float A = 0.35;                 // Área irrigada (m²)
                float ia = (N * q) / A;         // Intensidade de aplicação
                float LB = (soma_et / leituras) / ia * kl;  // Lâmina bruta
                TI = (LB / ia) * 60;            // Tempo de irrigação (em minutos)

                lcd.setCursor(0, 1);
                lcd.print("Tempo Irrig:");
                lcd.print(TI);
                lcd.print(" min");

                // Liga a válvula
                digitalWrite(pinValvula, HIGH);

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
    if (tempoligado >= TI * 60000) {     // Converte minutos para milissegundos
        digitalWrite(pinValvula, LOW);    // Desliga a válvula
        tempoligado = 0;                  // Reinicia o tempo ligado
        TI = 0;                           // Zera o tempo de irrigação
    }

    tempoligado += 10;  // Incrementa o tempo ligado
    delay(10);          // Pequeno delay para evitar um loop muito rápido
}
