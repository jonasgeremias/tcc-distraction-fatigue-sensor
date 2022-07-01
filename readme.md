# DESENVOLVIMENTO DE UM SISTEMA DETECTOR DE DISTRAÇÃO PARA USO EM VEÍCULOS UTILIZANDO ESP32 E VISÃO COMPUTACIONAL

## Resumo: 

* Contexto:

Com a necessidade de minimizar os acidentes no trânsito, dispositivos diversos foram desenvolvidos e implementados nos automóveis. 
O cinto de segurança, espelhos retrovisores, airbags, freios ABS, para-brisas laminados são alguns exemplos de equipamentos de segurança veicular, porém estes não evitam o maior causador de acidentes: a distração. 

* Objetivo:

Desta forma, o presente artigo apresentou o desenvolvimento de uma solução de hardware utilizando o chip ESP32 como base e conceitos de visão computacional para aplicação em veículos com objetivo de detectar a distração do motorista. Foram apresentadas as etapas de definição do hardware, coleta e análise dos dados, criação de firmware e testes funcioinais. 

* Testes:

Os testes aplicados mensuraram o tempo de reposta para a aquisição de imagem de diversos tamanhos, processamento utilizando visão computacional e deep learning, com biblioteca disponibilizada pelo fabricante do chip, e criação da lógica da aplicação final usando os periféricos como câmera, wi-fi e cartão de memória. 

* Resultados:

Os resultados mostraram que o ESP32 em sistemas mais complexos, envolvendo processamento de imagem ou deep learning, apresentou-se lento nessas aplicações, sendo possível ser utilizado na aplicação desenvolvida neste projeto, mas impossibilitando melhorias na solução, como a identificação de motoristas e detecção de uso de celular ou fumaça. 

Durante os testes de detecção de distração, foram observados os principais fatores que devem analisados nas aplicações futuras, são eles: tamanho e qualidade da imagem, tempo de salvamento, tempo de detecção, luminosidade, e tarefas em execução paralela.

* Melhorias:

Mostra-se desta forma o potencial a ser explorado nesta área de desenvolvimento tecnológico e a possibilidade de aprofundamento nos campos de integração de sistemas embarcados e visão computacional para a implementação com maior assertividade desta solução.

## Princiais bibliotecas

* Câmera OV2640
* ESP-DL
* LED / Buzzer
* Cartão de memória
* Wi-FI
* AP server

## Autor

* Jonas P. Geremias