#include <AFMotor.h> 
#include <NewPing.h>
#include <Servo.h>
#include <math.h>


#define baud_rate 115200 // 115200 permite ver os resultados de ping
#define pin_trigger 14 // trigger sonar
#define pin_echo 15 // echo sonar
#define max_distancia_sonar 20 // máxima distância a ser escaneada pelo sonar
#define max_amplitude_sonar 5 // amplitude da varredura em graus
#define pos_sonar_mid 65 // meio ("90 graus") do varredor
#define pos_sonar_min pos_sonar_mid - max_amplitude_sonar // angulação mínima para varredura
#define pos_sonar_max pos_sonar_mid + max_amplitude_sonar // angulação máxima para varredura
#define pin_garras 3 // motor servo da gara
#define pin_varredor 5 // motor servo do sonar
#define pin_rodas_esq 6 // motor rodas da esquerda
#define pin_rodas_dir 9 // motor rodas da direita
#define tempo_giro 1 // gira o carro em 45 graus
#define velocidade_rodas 106 //

int pos_garras = 0; // posicao do motor das garras
int pos_varredor = 0; // posicao do motor do sensor
int pos_inicio_visao; // posicao na qual o sensor começa a "ver" o objeto
int pos_fim_visao; // posicao na qual o sensor deixa de "ver" o objeto
int pos_objeto; // posicao na qual o sensor está alinhado (meio) com o objeto
int loop_do_encontro; // número do loop (1 || 2) onde foi encontrado o objeto
boolean primeiro_loop = true; // flag de primeiro loop
unsigned int distancia = 0; // distância do objeto
boolean angulo_limite_atingido = false; // flag para saber se chegou no ângulo máx do varredor
char sentido_rotacao; // indica para qual lado o carro deve girar para se alinhar ao objeto

// Inicia sonar e motores
NewPing sonar(pin_trigger, pin_echo, max_distancia_sonar);
Servo varredor;
Servo garras;
AF_DCMotor rodas_esquerda(pin_rodas_esq);
AF_DCMotor rodas_direita(pin_rodas_dir);

// Funções
void gira_servo();
void varredura();
int pos_media();
void alinha_carro();
void pega_objeto();
void rotacao_direita();
void rotacao_esquerda();
void avanca();

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup () {
  Serial.begin(baud_rate);

  delay(1500);
  
  // Inicia garras
  garras.attach(pin_garras);
  garras.write(0);
  Serial.println("Garras iniciadas");

  // Inicia Varredor
  varredor.attach(pin_varredor);
  varredor.write(pos_sonar_min); 
  Serial.println("Varredor iniciado");
  
  // Inicia motores das rodas
  rodas_esquerda.setSpeed(velocidade_rodas);
  rodas_direita.setSpeed(velocidade_rodas);
//  pinMode(pin_rodas_esq, OUTPUT);
//  pinMode(pin_rodas_dir, OUTPUT);
  Serial.println("Rodas iniciadas");
}


// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop () {  
  varredura(); // TODO: se não encontrar...girar o carro e realizar nova busca

  pos_objeto = pos_media(pos_inicio_visao, pos_fim_visao);

  Serial.print("Início da visão: ");
  Serial.println(pos_inicio_visao);
  Serial.print("Fim da visão: ");
  Serial.println(pos_fim_visao);

//   fecha garra
//  garras.write(180);
//  delay(1000);  
//  garras.write(0);

  // decide pra qual lado o carro deve girar
  if (pos_objeto > pos_sonar_mid) {
    sentido_rotacao = "d"; // vira pra direita
  } else if (pos_objeto < pos_sonar_mid) {
    sentido_rotacao = "e"; // vira pra esquerda
  } else {
    sentido_rotacao = "c"; // não vira...vai reto
  }
  
  alinha_carro(pos_objeto, sentido_rotacao);

  pega_objeto();

  distancia = 0; // TODO: apagar após teste
}


// -----------------------------------------------------------------------------
// GIRA O MOTOR SERVO DO SONAR NOS 2 SENTIDOS (ESQ-DIR, DIR-ESQ) ATÉ ENCONTRAR OBJETO
// -----------------------------------------------------------------------------
void varredura () {
  int pos;
  int descanso = 90; // período para o motor realizar o movimento

  Serial.println("Iniciando varredura()");

  // Encontra a posição inicial do objeto
  while (distancia == 0) {
    // Varredura da esquerda pra direita (angulo crescente)  
    for (pos = pos_sonar_min; pos <= pos_sonar_max; pos += 1) {       
      varredor.write(pos);              
      delay(descanso);   
      distancia = sonar.ping_cm(max_distancia_sonar); // tenta encontrar distância até o objeto
      
      if (distancia != 0) {
        loop_do_encontro = 1;
        Serial.print("Objeto encontrado no ângulo: ");
        Serial.println(pos);
        Serial.print("Distância do objeto: ");
        Serial.println(distancia);
        break; // sai do loop for                          
      }
    }

    if (distancia != 0) {
    // Varredura da direita pra esquerda (angulo decrescente)  
      for (pos = pos_sonar_max; pos >= pos_sonar_min; pos -= 1) {       
        varredor.write(pos);              
        delay(descanso);
        distancia = sonar.ping_cm(max_distancia_sonar); // tenta encontrar distância até o objeto 
        
        if (distancia != 0) {
          loop_do_encontro = 2;
          Serial.print("Objeto encontrado no ângulo: ");
          Serial.println(pos);
          Serial.print("Distância do objeto: ");
          Serial.println(distancia);
          break; // sai do loop for                        
        }                                             
      }      
    }
  }

  // Salva a posição inicial do objeto
  pos_inicio_visao = varredor.read();


  // Encontra a posição final do objeto (ou vai até o final)
  while (distancia != 0 || !angulo_limite_atingido) { 
    // O objeto foi encontrado no Loop 1 (angulação crescente)
    if (loop_do_encontro == 1) {
      // Continua a "varreção" (crescendo o ângulo)
      for (pos = varredor.read(); pos <= pos_sonar_max; pos += 1) { 
        varredor.write(pos);              
        delay(descanso);
        distancia = sonar.ping_cm(max_distancia_sonar); // tenta encontrar distância até o objeto 
        
        if (distancia == 0) {
          Serial.print("Objeto perdido no ângulo: ");
          Serial.println(pos);
          Serial.print("Distância: ");
          Serial.println(distancia);
          break; // sai do loop for                           
        }
      }
    // O objeto foi encontrado no Loop 2 (angulação decrescente)
    } else if (loop_do_encontro == 2) {
      // Continua a "varreção" (decrescendo o ângulo)  
      for (pos = varredor.read(); pos >= pos_sonar_min; pos -= 1) { 
        varredor.write(pos);              
        delay(descanso);
        distancia = sonar.ping_cm(max_distancia_sonar); // tenta encontrar distância até o objeto 
        
        if (distancia == 0) {
          Serial.print("Objeto perdido no ângulo: ");
          Serial.println(pos);
          Serial.print("Distância: ");
          Serial.println(distancia);
          break; // sai do loop for                           
        }

        if (pos == pos_sonar_min) {
          Serial.println("O objeto foi visto até o final da amplitude de varredura.");
          angulo_limite_atingido = true;
        }
      }
    }    
  }
  
  pos_fim_visao = varredor.read();
}

// -----------------------------------------------------------------------------
// CALCULA O ANGULO EM QUE O OBJETO SE ENCONTRA (MEIO ENTRE POS_INICIO E POS_FINAL)
// -----------------------------------------------------------------------------
int pos_media (int pos_inicial, int pos_final) {
  if (pos_inicial > pos_final) {
    return round(pos_inicial / pos_final);
  } else {
    return round(pos_final / pos_inicial);    
  }
}

// -----------------------------------------------------------------------------
// ALINHA O CARRO COM O VARREDOR
// -----------------------------------------------------------------------------
void alinha_carro (int pos_alvo, char sentido) {
  Serial.println("Iniciando alinha_carro()");

  if (sentido == "d") {
    rotacao_direita(pos_alvo);
  } else if (sentido == "e") {
    rotacao_esquerda(pos_alvo);
  } else {  
    avanca();
  }
  
  // TODO: alinhar o carro com sensor...angulo do sonar == 0
}

// -----------------------------------------------------------------------------
// GIRA O CARRO (NO MESMO EIXO) PARA A DIREITA
// -----------------------------------------------------------------------------
void rotacao_direita (int angulo) {
  Serial.println("Iniciando rotacao_direita()");
  rodas_direita.run(BACKWARD);
  rodas_esquerda.run(FORWARD);
  delay(tempo_giro);  
  rodas_direita.run(RELEASE);  // TODO: APAGAR APÓS TESTE 
  rodas_esquerda.run(RELEASE);  // TODO: APAGAR APÓS TESTE 
}

// -----------------------------------------------------------------------------
// GIRA O CARRO (NO MESMO EIXO) PARA A ESQUERDA
// -----------------------------------------------------------------------------
void rotacao_esquerda (int angulo) {
  Serial.println("Iniciando rotacao_esquerda()");
  rodas_direita.run(FORWARD);
  rodas_esquerda.run(BACKWARD);
  delay(tempo_giro);  
  rodas_direita.run(RELEASE);  // TODO: APAGAR APÓS TESTE 
  rodas_esquerda.run(RELEASE); // TODO: APAGAR APÓS TESTE 
}

// -----------------------------------------------------------------------------
// GIRA O CARRO (NO MESMO EIXO) PARA A ESQUERDA
// -----------------------------------------------------------------------------
void avanca () {
  Serial.println("Iniciando avanca()");
  rodas_direita.run(FORWARD);
  rodas_esquerda.run(FORWARD);
  delay(1000);  
  // TODO: APAGAR APÓS TESTE 
  rodas_direita.run(RELEASE);
  rodas_esquerda.run(RELEASE);
}




// -----------------------------------------------------------------------------
// PEGA O OBJETO
// -----------------------------------------------------------------------------
void pega_objeto () {
  Serial.println("Iniciando pega_objeto()");
    // TODO: verificar se a pinça pegou algo ou se escapuliu
}

