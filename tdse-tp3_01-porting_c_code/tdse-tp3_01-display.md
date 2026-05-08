¡Por supuesto que sí! Pasar de leer el hardware (botones) a controlar una salida visual compleja como un display LCD es el siguiente gran paso, y suele ser uno de los más gratificantes en los sistemas embebidos.

Basándonos en el título de tu trabajo práctico ("LCD Display (porting C code) - System Setup (statechart - modeling - c coding)"), el desafío se divide en tres frentes de batalla muy claros. Aquí tienes una hoja de ruta conceptual para estructurar tu trabajo:

### 1. Porting C Code (Adaptación del Controlador)
"Portar" código significa tomar una librería que fue escrita para otra plataforma (como Arduino o un microcontrolador genérico) y traducirla para que funcione con tu hardware actual (la placa STM32 y su capa HAL).

* **Aislamiento de la capa física:** Un buen driver de LCD tiene separada la lógica (qué comandos enviar para limpiar la pantalla o mover el cursor) del hardware (cómo poner un pin en alto o en bajo). 
* **Tu tarea de porting:** Consistirá en buscar dentro de la librería que te hayan dado (probablemente para un controlador estándar como el HD44780) las funciones de más bajo nivel y reemplazarlas. Por ejemplo, donde la librería original diga algo genérico como `digitalWrite(pin, HIGH)`, tú deberás inyectar tu `HAL_GPIO_WritePin(Puerto, Pin, GPIO_PIN_SET)`.

### 2. Modeling (Diseño del Statechart)
No puedes simplemente mandar texto al LCD en cualquier momento; la pantalla requiere tiempos de espera estrictos para inicializarse y procesar caracteres. Aquí es donde entra tu máquina de estados.

* **Estados Típicos:** Tu modelo debería contemplar estados como `ST_LCD_INIT` (donde se envían los comandos de configuración iniciales), `ST_LCD_IDLE` (esperando órdenes) y `ST_LCD_PRINTING` (escribiendo carácter por carácter).
* **Tipos de Datos:** Cuando modeles la lógica de los caracteres o los temporizadores internos en herramientas de diseño visual (como Itemis CREATE), asegúrate de utilizar tipos de datos explícitos del estándar C, como `uint8_t` para los caracteres o `uint32_t` para las variables de estado y tiempos. Evitar los tipos genéricos como `int` en el modelo visual te ahorrará dolores de cabeza y errores de compilación al momento de generar el código fuente.

### 3. C Coding (Integración en el Sistema ETS)
Una vez que tengas el driver portado y la máquina de estados generada, deberás integrarlo en la arquitectura orientada a eventos (ETS) que ya tienes armada del TP2.

* **La Tarea Actuador:** El LCD se convertirá en una nueva tarea (o reemplazará a la del LED).
* **Comunicación:** Utilizarás un buzón de mensajes similar al que analizamos antes. Cuando tu `task_system` decida que es momento de mostrar un mensaje (por ejemplo, porque alguien apretó un botón en tu matriz), le enviará un evento (ej. `EV_LCD_UPDATE`) a la tarea del LCD.
* **No bloqueante:** Es vital que la función que escribe en el LCD no use retardos bloqueantes (`HAL_Delay`), ya que eso arruinaría tu perfilado de tiempos (`WCET`). Si el LCD necesita 2 milisegundos para procesar un comando, la máquina de estados debe usar la variable `tick` para contar ese tiempo de forma asíncrona.

Para empezar a trabajar en el código con precisión, ¿qué modelo exacto de LCD vas a utilizar (por ejemplo, el clásico 16x2 alfanumérico) y qué tipo de interfaz de conexión tiene (módulo I2C de 4 cables o conexión paralela directa)?  

Este conjunto de archivos implementa la integración de una pantalla LCD alfanumérica dentro de tu arquitectura orientada a eventos (Event-Triggered System). El sistema aísla la capa de hardware (el driver del LCD) de la lógica de la aplicación mediante el uso de tareas y memorias intermedias (buffers).

A continuación, se detalla la función de cada módulo y el comportamiento de las máquinas de estado solicitadas:

### 1. El Núcleo del Sistema y Control de Tiempo
* **`app.c`:** Es el planificador (scheduler) principal. En su bucle infinito, ejecuta secuencialmente las tareas configuradas (`task_test` y `task_display`) basándose en una bandera de tiempo (`b_time_update_required`). Además, continúa midiendo el tiempo de ejecución (LET, BCET, WCET) de cada tarea utilizando el contador de ciclos DWT.
* **`app_it.c`:** Contiene la rutina de atención a la interrupción del temporizador del sistema (`HAL_SYSTICK_Callback`). Se ejecuta cada 1 milisegundo e incrementa la variable global `g_app_tick_cnt`, la cual es consumida por `app.c` para mantener el ritmo estricto de ejecución de las tareas.
* **`systick.c`:** Provee la función `systick_delay_us()`, la cual genera un retardo bloqueante en microsegundos leyendo directamente los registros de hardware (`SysTick->VAL` y `SysTick->LOAD`). Esta precisión sub-milisegundo es crítica para respetar los tiempos de inicialización y escritura que exige el hardware del display LCD.

### 2. Capa de Abstracción de Hardware (Driver del LCD)
* **`display.h` y `display.c`:** Representan el código "portado". Adaptan un controlador estándar de LCD (probablemente HD44780) al ecosistema de STM32. 
    * `display.h` define los tipos de conexión permitidos (4 bits o 8 bits).
    * `display.c` traduce las funciones de alto nivel (como inicializar el display o escribir un string) en secuencias precisas de unos y ceros enviadas a pines físicos mediante `displayPinWrite`. Internamente, se encarga de alternar el pin *Enable* (`DISPLAY_PIN_EN`) y gestionar si los datos se envían como comandos o caracteres.

### 3. Tarea de Aplicación: Generación de Datos
* **`task_test_attribute.h`:** Define la estructura de datos `task_test_dta_t`, que aloja dos variables enteras: `tick` y `counter`.
* **`task_test.c`:** Es una tarea de prueba que simula un proceso del sistema requiriendo mostrar datos en la pantalla. En su inicialización (`task_test_init`), envía los textos iniciales "LCD Display Test" y " Porting C code " a la pantalla.
* **Comportamiento de `task_test_statechart(void)`:** Actúa como un temporizador por software que se evalúa cada milisegundo:
    1.  Incrementa permanentemente su variable `counter`.
    2.  Verifica si la variable `tick` es mayor a 0 y la decrementa.
    3.  Cuando `tick` llega a 0 (lo que ocurre cada 1000 ms, definido por `DEL_TEST_XX_MAX`), la función recarga el `tick` a 1000.
    4.  Acto seguido, envía dos órdenes de escritura a la interfaz del LCD: el texto estático "Test Nro: ******" y el valor actual de los segundos transcurridos (calculado como `counter / DEL_TEST_XX_MAX`), convertido a texto mediante `snprintf`.

### 4. Tarea del LCD: Presentación Visual
* **`task_display_attribute.h`:** Define una matriz llamada `ddram` de 2 filas por 17 columnas (16 caracteres + terminador nulo) que actúa como una "memoria de video" (Buffer) local en la RAM del microcontrolador.
* **`task_display_interface.c`:** Es el "buzón" entre el sistema y la pantalla. La función `put_event_task_display()` recibe unas coordenadas (fila y columna) y un texto. Su trabajo es copiar carácter por carácter ese texto dentro de la matriz `ddram` local, levantar la bandera de la tarea (`flag = true`) y asignarle el evento `EV_DSP_UPDATE`.
* **Comportamiento de `task_display_statechart(void)`:**
    Es la máquina de estados que interactúa físicamente con el display. Protege al sistema de enviar datos al hardware más rápido de lo que este puede procesarlos:
    1.  **Estado `ST_DSP_IDLE`:** La tarea permanece en reposo evaluando si llegó un nuevo evento. Si la bandera es verdadera (`true`) y el evento es `EV_DSP_UPDATE`, transiciona al estado de actualización (`ST_DSP_UPDATE`).
    2.  **Estado `ST_DSP_UPDATE`:** Aquí se realiza el volcado de memoria. Baja la bandera (`flag = false`). Luego, le indica al driver físico que mueva el cursor a la posición 0 de la primera fila (`displayCharPositionWrite(0, 0)`) y envía el contenido completo de la primera fila de la matriz `ddram` (`displayStringWrite`). Inmediatamente repite este proceso exacto para la segunda fila (coordenada 0, 1). 
    3.  Al finalizar el volcado de las dos filas, retorna de manera automática al estado `ST_DSP_IDLE` para quedar a la espera de una nueva actualización solicitada por el sistema.
		
# Registro de Inicialización del Sistema (Boot Log)

Al ejecutar el programa en el microcontrolador, la terminal (Logger) arroja la siguiente secuencia de inicialización, confirmando el correcto arranque del planificador de tareas y los módulos integrados:

```text
[info]  
[info]  app_init is running - Tick [mS] = 0
[info]   app is a Bare Metal - Event-Triggered Systems (ETS)
[info]   app is a App - Porting C code - C codig
[info]   app is a (Update by Time Code, period = 1mS)
[info]   g_app_cnt = 0
[info]  
[info]   task_test_init is running - Tick [mS] = 0
[info]    task_test is a Task Test (Test Code Integration)
[info]    task_test is a Non-Blocking & Update By Time Code
[info]    task_test is a (Update by Time Code, period = 1mS)
[info]    tick = 1000
[info]  
[info]   task_display_init is running - Tick [mS] = 1
[info]    task_display is a Task Display (Display Statechart)
[info]    task_display is a Non-Blocking Code
[info]    task_display is a (Update by Time Code, period = 1mS)
[info]  
[info]    state = 0   event = 0   b_event = false
