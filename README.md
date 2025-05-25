# Prototipo_ESP32-C3-LCD
Archivos del prorotipo de la placa para el taller de FreeRTOS, es un diseño rápido con componentes que tenía a mano.. 

Código de prueba para verificar el Hardware, no usar el código como referencia!

No se han medido las señales de la placa con el osciloscopio, por lo que aún está por verificar el correcto funcionamiento del pcb a nivel de señal y consumos de corriente (a nivel de funcionalidad-programación parece correcto).

Cambios hardware para futuras revisiones:
- Condensadores y diodos TVS a la entrada del USB, ver compatibilidad con distintas marcas de cargadores.
- Cambiar la ubicación del sensor de temperatura (se calienta con la batería en carga) y aplicarle un "moat".
- Reducir espacios en las DRC e incrementar algunos planos de disipación térmica.
- Calcular y ajustar los condensadores (2*(Cl-Cs)) del cristal del RTC, medir la desviación de este.
- Reducir el número de valores distintos de resistencias y condensadores en el BOM.

Es un diseño hardware MUY rápido para evaluar la herramienta de KiCad, en ningún caso usar como referencia. 

Si el proyecto tiene interés entre los asistentes a las reuniones de la asociación, se hará una revisión del PCB más despacio: mejorando el esquemático, el ruteado del PCB y buscando optimizar el consumo de corriente seleccionando los distintos componentes de la placa (cuando hay batería cada uA cuenta!).
