# Sensor fadiga


## Dependências

* Baixar e colocar na pasta `esp-idf/components`:
    * espressif/esp32-camera: <a href="https://github.com/espressif/esp32-camera.git">ESP32-CAMERA</a> 
    * espressif/esp-dl: <a href="https://github.com/espressif/esp-dl.git">ESP-DL</a>.

## Teste de tempo




* JSON resultado do
```
{{"/sdcard/p0_0_0.jpg":{"configCam":106722,"getImage":160393,"saveFile":34262,"saveStatus":0,"release":387,"end":324}}
{"/sdcard/p0_0_1.jpg":{"configCam":105891,"getImage":160600,"saveFile":34027,"saveStatus":0,"release":382,"end":305}}
{"/sdcard/p0_0_2.jpg":{"configCam":105676,"getImage":160807,"saveFile":34827,"saveStatus":0,"release":383,"end":296}}
{"/sdcard/p0_0_3.jpg":{"configCam":105851,"getImage":160592,"saveFile":35407,"saveStatus":0,"release":382,"end":305}}
{"/sdcard/p0_0_4.jpg":{"configCam":105683,"getImage":160800,"saveFile":35750,"saveStatus":0,"release":383,"end":297}}
{"/sdcard/p0_1_0.jpg":{"configCam":105861,"getImage":160499,"saveFile":64969,"saveStatus":0,"release":407,"end":305}}
{"/sdcard/p0_1_1.jpg":{"configCam":105825,"getImage":160477,"saveFile":67554,"saveStatus":0,"release":408,"end":296}}
{"/sdcard/p0_1_2.jpg":{"configCam":105975,"getImage":160286,"saveFile":362450,"saveStatus":0,"release":408,"end":305}}
{"/sdcard/p0_1_3.jpg":{"configCam":106042,"getImage":160286,"saveFile":64821,"saveStatus":0,"release":407,"end":297}}
{"/sdcard/p0_1_4.jpg":{"configCam":105977,"getImage":160285,"saveFile":67484,"saveStatus":0,"release":408,"end":305}}
{"/sdcard/p0_2_0.jpg":{"configCam":106006,"getImage":160609,"saveFile":429977,"saveStatus":0,"release":411,"end":297}}
{"/sdcard/p0_2_1.jpg":{"configCam":105982,"getImage":160619,"saveFile":78920,"saveStatus":0,"release":414,"end":305}}
{"/sdcard/p0_2_2.jpg":{"configCam":105901,"getImage":160715,"saveFile":82868,"saveStatus":0,"release":411,"end":297}}
{"/sdcard/p0_2_3.jpg":{"configCam":105906,"getImage":160715,"saveFile":87391,"saveStatus":0,"release":411,"end":305}}
{"/sdcard/p0_2_4.jpg":{"configCam":105888,"getImage":160728,"saveFile":399736,"saveStatus":0,"release":411,"end":297}}
{"/sdcard/p0_3_0.jpg":{"configCam":105977,"getImage":160770,"saveFile":266335,"saveStatus":0,"release":402,"end":305}}
{"/sdcard/p0_3_1.jpg":{"configCam":105887,"getImage":160890,"saveFile":303807,"saveStatus":0,"release":401,"end":298}}
{"/sdcard/p0_3_2.jpg":{"configCam":105687,"getImage":161085,"saveFile":272345,"saveStatus":0,"release":402,"end":306}}
{"/sdcard/p0_3_3.jpg":{"configCam":105995,"getImage":160781,"saveFile":334504,"saveStatus":0,"release":402,"end":298}}
{"/sdcard/p0_3_4.jpg":{"configCam":105977,"getImage":160772,"saveFile":464058,"saveStatus":0,"release":402,"end":305}}
{"/sdcard/p0_4_0.jpg":{"configCam":106014,"getImage":160508,"saveFile":315723,"saveStatus":0,"release":399,"end":298}}
{"/sdcard/p0_4_1.jpg":{"configCam":105972,"getImage":160522,"saveFile":463898,"saveStatus":0,"release":399,"end":306}}
{"/sdcard/p0_4_2.jpg":{"configCam":105985,"getImage":160537,"saveFile":480834,"saveStatus":0,"release":400,"end":297}}
{"/sdcard/p0_4_3.jpg":{"configCam":105975,"getImage":160519,"saveFile":485667,"saveStatus":0,"release":399,"end":306}}
{"/sdcard/p0_4_4.jpg":{"configCam":105989,"getImage":160529,"saveFile":465746,"saveStatus":0,"release":400,"end":297}}
{"/sdcard/p0_5_0.jpg":{"configCam":105892,"getImage":160618,"saveFile":515611,"saveStatus":0,"release":405,"end":305}}
{"/sdcard/p0_5_1.jpg":{"configCam":105705,"getImage":160853,"saveFile":508314,"saveStatus":0,"release":406,"end":298}}
{"/sdcard/p0_5_2.jpg":{"configCam":105985,"getImage":160522,"saveFile":536063,"saveStatus":0,"release":405,"end":306}}
{"/sdcard/p0_5_3.jpg":{"configCam":105996,"getImage":160536,"saveFile":512388,"saveStatus":0,"release":405,"end":298}}
{"/sdcard/p0_5_4.jpg":{"configCam":105981,"getImage":160525,"saveFile":514887,"saveStatus":0,"release":406,"end":305}}
{"/sdcard/p0_6_0.jpg":{"configCam":105982,"getImage":161217,"saveFile":645089,"saveStatus":0,"release":405,"end":298}}
{"/sdcard/p0_6_1.jpg":{"configCam":105988,"getImage":161191,"saveFile":626243,"saveStatus":0,"release":405,"end":306}}
{"/sdcard/p0_6_2.jpg":{"configCam":105996,"getImage":161211,"saveFile":644664,"saveStatus":0,"release":405,"end":297}}
{"/sdcard/p0_6_3.jpg":{"configCam":105990,"getImage":161188,"saveFile":638395,"saveStatus":0,"release":405,"end":306}}
{"/sdcard/p0_6_4.jpg":{"configCam":105996,"getImage":161208,"saveFile":615663,"saveStatus":0,"release":403,"end":298}}
{"/sdcard/p0_7_0.jpg":{"configCam":106732,"getImage":599351,"saveFile":739417,"saveStatus":0,"release":408,"end":306}}
{"/sdcard/p0_7_1.jpg":{"configCam":106601,"getImage":599502,"saveFile":739262,"saveStatus":0,"release":402,"end":298}}
{"/sdcard/p0_7_2.jpg":{"configCam":106552,"getImage":599552,"saveFile":719428,"saveStatus":0,"release":404,"end":306}}
{"/sdcard/p0_7_3.jpg":{"configCam":106721,"getImage":599378,"saveFile":729608,"saveStatus":0,"release":403,"end":298}}
{"/sdcard/p0_7_4.jpg":{"configCam":106589,"getImage":599485,"saveFile":714983,"saveStatus":0,"release":402,"end":306}}
{"/sdcard/p0_8_0.jpg":{"configCam":106547,"getImage":618163,"saveFile":1183590,"saveStatus":0,"release":406,"end":298}}
{"/sdcard/p0_8_1.jpg":{"configCam":106746,"getImage":617913,"saveFile":1055137,"saveStatus":0,"release":403,"end":307}}
{"/sdcard/p0_8_2.jpg":{"configCam":106635,"getImage":618063,"saveFile":896269,"saveStatus":0,"release":405,"end":298}}
{"/sdcard/p0_8_3.jpg":{"configCam":106581,"getImage":618100,"saveFile":892634,"saveStatus":0,"release":405,"end":306}}
{"/sdcard/p0_8_4.jpg":{"configCam":106752,"getImage":617934,"saveFile":880945,"saveStatus":0,"release":405,"end":298}}
{"/sdcard/p0_9_0.jpg":{"configCam":106747,"getImage":617873,"saveFile":1388629,"saveStatus":0,"release":415,"end":307}}
{"/sdcard/p0_9_1.jpg":{"configCam":106765,"getImage":617895,"saveFile":1366075,"saveStatus":0,"release":415,"end":299}}
{"/sdcard/p0_9_2.jpg":{"configCam":106752,"getImage":617880,"saveFile":1374210,"saveStatus":0,"release":415,"end":307}}
{"/sdcard/p0_9_3.jpg":{"configCam":106764,"getImage":617896,"saveFile":1382072,"saveStatus":0,"release":415,"end":299}}
{"/sdcard/p0_9_4.jpg":{"configCam":106754,"getImage":617877,"saveFile":1349999,"saveStatus":0,"release":415,"end":307}}
{"/sdcard/p0_10_0.jpg":{"configCam":106427,"getImage":1257568,"saveFile":2272478,"saveStatus":0,"release":416,"end":299}}
{"/sdcard/p0_10_1.jpg":{"configCam":106437,"getImage":1257561,"saveFile":2233238,"saveStatus":0,"release":416,"end":307}}
{"/sdcard/p0_10_2.jpg":{"configCam":106396,"getImage":1257580,"saveFile":2221011,"saveStatus":0,"release":416,"end":300}}
{"/sdcard/p0_10_3.jpg":{"configCam":106392,"getImage":1257555,"saveFile":2231649,"saveStatus":0,"release":416,"end":307}}
{"/sdcard/p0_10_4.jpg":{"configCam":106402,"getImage":1257574,"saveFile":2231370,"saveStatus":0,"release":416,"end":299}}
{"/sdcard/p0_11_0.jpg":{"configCam":106377,"getImage":1180561,"saveFile":2611784,"saveStatus":0,"release":411,"end":308}}
{"/sdcard/p0_11_1.jpg":{"configCam":106378,"getImage":1180586,"saveFile":2600614,"saveStatus":0,"release":411,"end":300}}
{"/sdcard/p0_11_2.jpg":{"configCam":106368,"getImage":1180567,"saveFile":2622329,"saveStatus":0,"release":411,"end":308}}
{"/sdcard/p0_11_3.jpg":{"configCam":106380,"getImage":1180584,"saveFile":2592074,"saveStatus":0,"release":410,"end":299}}
{"/sdcard/p0_11_4.jpg":{"configCam":106364,"getImage":1180569,"saveFile":2603079,"saveStatus":0,"release":411,"end":308}}
{"/sdcard/p0_12_0.jpg":{"configCam":106387,"getImage":1257446,"saveFile":3735821,"saveStatus":0,"release":416,"end":299}}
{"/sdcard/p0_12_1.jpg":{"configCam":106376,"getImage":1257427,"saveFile":3710475,"saveStatus":0,"release":416,"end":307}}
{"/sdcard/p0_12_2.jpg":{"configCam":106292,"getImage":1257552,"saveFile":3689291,"saveStatus":0,"release":415,"end":300}}
{"/sdcard/p0_12_3.jpg":{"configCam":106301,"getImage":1257490,"saveFile":3742935,"saveStatus":0,"release":415,"end":308}}
{"/sdcard/p0_12_4.jpg":{"configCam":106388,"getImage":1257444,"saveFile":3717140,"saveStatus":0,"release":416,"end":299}}
{"/sdcard/p0_13_0.jpg":{"configCam":106375,"getImage":1257619,"saveFile":5413315,"saveStatus":0,"release":567,"end":515}}
{"/sdcard/p0_13_1.jpg":{"configCam":107542,"getImage":1257465,"saveFile":5421277,"saveStatus":0,"release":568,"end":515}}
{"/sdcard/p0_13_2.jpg":{"configCam":107447,"getImage":1257580,"saveFile":5425230,"saveStatus":0,"release":568,"end":515}}
{"/sdcard/p0_13_3.jpg":{"configCam":107246,"getImage":1257725,"saveFile":5441253,"saveStatus":0,"release":568,"end":515}}
{"/sdcard/p0_13_4.jpg":{"configCam":107435,"getImage":1257587,"saveFile":5430791,"saveStatus":0,"release":567,"end":515}}
{"/sdcard/p1_0_0.jpg":{"configCam":112311,"getImage":41138,"saveFile":16730,"saveStatus":0,"release":383,"end":340}}
{"/sdcard/p1_0_1.jpg":{"configCam":111327,"getImage":40968,"saveFile":15368,"saveStatus":0,"release":398,"end":340}}}
```
