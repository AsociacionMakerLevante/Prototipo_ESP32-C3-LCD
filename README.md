# Prototipo_ESP32-C3-LCD
Archivos del prorotipo de placa para el taller de FreeRTOS, es un diseño rápido con componentes que tenía a mano.. 

AÚN POR VERIFICAR SU CORRECTO FUNCIONAMIENTO A NIVEL DE SEÑAL!!

Código rápido de prueba para verificar el Hardware, no usar el código como referencia!

No se han medido las señales de la placa con el osciloscopio, por lo que aún está por verificar el correcto funcionamiento del pcb a nivel de señal (a nivel de funcionalidad-programación parece correcto).

Cambios hardware para futuras revisiones:
- Condensadores y diodos TVS a la entrada del USB, ver compatibilidad con distintas marcas de cargadores.
- Cambiar la ubicación del sensor de temperatura (se calienta más de lo esperado) y aplicarle un "moat".

Es un diseño hardware MUY rápido para evaluar la herramienta de KiCad, en ningún caso usar de referencia. 

Si el proyecto tiene interés entre los asistentes a las reuniones de la asociación, se hará una revisión del PCB más despacio: mejorando el esquemático, el ruteado del PCB y buscando optimizar el consumo de corriente seleccionando los distintos componentes de la placa (cuando hay batería cada uA cuenta!).
