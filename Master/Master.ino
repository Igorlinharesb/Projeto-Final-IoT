// Bibliotecas necessárias para o uso do RFID
  #include <SPI.h>
  #include <MFRC522.h>

// Pinos para configuração do RFID
  #define RST_PIN    D3
  #define SS_PIN     D8
  #define AUTH       D1 // Pino que recebe a autenticação pelo RFID

// Configurando o ESP Master
    extern "C" {
      #include <espnow.h> //Biblioteca do ESP-Now
    }
    
    //Estruturas de dados que será enviada
    struct DADOS {
      uint16_t valor = LOW;
      uint32_t tempo = 0;
    };

    //Modo de operação do protocolo
    #define MASTER 1 //Pareia e envia os dados
    #define SLAVE 2 //É pareado e recebe os dados
    #define MASTER_SLAVE 3 //Pareia, envia e recebe dados
    
    //Definindo o modo de operação
    #define MODO MASTER
    
    #define CANAL 1 //Canal de comunicação
    
    uint8_t MACslave[6] = {0x2C, 0x3A, 0xE8, 0x43, 0x93, 0x37}; //Endereço MAC do ESP nomeado como SLAVE
    
    //Métodos úteis para o protocolo ESP-Now
      void IniciaESPNow() { //Inicia ESP-Now
        if (esp_now_init() != 0) {
          Serial.println("Falha ao iniciar protocolo ESP-Now; Reiniciando...");
          ESP.restart();
          delay(1);
        }
      
        esp_now_set_self_role(MODO); //Setando modo de operação
      
        //Chave da comunicação
        uint8_t KEY[0] = {}; //Definindo chave como NULL
        uint8_t KEY_LEN = sizeof(KEY);
      
        esp_now_add_peer(MACslave, MODO, CANAL, KEY, KEY_LEN); //Pareando ao escravo
      }
      
      void Envia(uint16_t valor) { //Função para enviar os dados ao escravo pareado
        DADOS dados;
        dados.valor = valor;
        dados.tempo = millis();
      
        //Debug
        Serial.println("Dados enviados:");
        Serial.print("Valor: ");
        Serial.print(dados.valor);
        Serial.print(", Tempo: ");
        Serial.println(dados.tempo);
      
        //Montando o pacote de envio
        uint8_t data[sizeof(dados)];
        memcpy(data, &dados, sizeof(dados));
        uint8_t len = sizeof(data);
      
        esp_now_send(MACslave, data, len); //Envia dados para o escravo
        //delay(3);
      }
      
      void Enviou(uint8_t* mac, uint8_t status) { //Função Callback para verificar se foi recebido (ACK)
        char MACescravo[6];
        sprintf(MACescravo, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.print("Enviado p/ ESP de MAC: ");
        Serial.print(MACescravo);
        Serial.print(" Recepcao: ");
        Serial.println((status == 0 ? "OK" : "ERRO"));
      }

MFRC522 mfrc522(SS_PIN, RST_PIN); // Cria uma instância MFRC522

void setup() {
  Serial.begin(115200);   // Inicia a serial
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
  pinMode(D0, OUTPUT);
  pinMode(D2, OUTPUT);
  //trecho de setup para a comunicação ESP-Now
  IniciaESPNow();
  esp_now_register_send_cb(Enviou); //Setando Callback de envio do ESP-Now
}

void loop() {
  //delay(3000);
  digitalWrite(D0, LOW);
  digitalWrite(D2, LOW);
  // Procura por cartao RFID
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Seleciona o cartao RFID
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  //Mostra UID na serial
  Serial.print("UID da tag :");
  String conteudo= "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  conteudo.toUpperCase();

  //imprime os detalhes tecnicos do cartão/tag
  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

  if ((conteudo.substring(1) == "30 3C 3C A6")) //UID 1 - Cartao
  {
    Serial.println("Acesso liberado !");
    Serial.println();
    Envia((uint16_t)1); //Envia os dados a cada 1 seg
    digitalWrite(D0, HIGH);
    delay(500);
    
  }
  else
  {
    Serial.println("Acesso negado !");
    Serial.println();
    Envia((uint16_t)0); //Envia os dados a cada 1 seg
    digitalWrite(D2, HIGH);
    delay(500);
  }

}
