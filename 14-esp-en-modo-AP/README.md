# modo AP

## ¿que es el modo AP?

El modo AP convierte al dispositivo en un punto de acceso inalámbrico, como un router basico. Al estar en este modo:
- El ESP32 crea y transmite un SSID (nombre de la red Wi-Fi).
- Los dispositivos cercanos pueden buscar este SSID e intentar conectarse usando una contraseña (si está configurada).
- El ESP32 puede asignar direcciones IP a los dispositivos conectados mediante DHCP (Protocolo de Configuración Dinámica de Host).
- Actúa como un servidor para cualquier comunicación dentro de esa red.

## caracteristicas

- red local: los dispositivos conectados al AP forman una red local, pero el AP no tiene acceso a internet a menos que este configurado como un AP+estación

- Capacidad de Conexiones: El ESP32 puede aceptar hasta 10 conexiones simultáneas en modo AP, pero el valor típico es de 4 para mantener un buen rendimiento.

- Dirección IP: El AP asigna direcciones IP locales (generalmente en el rango 192.168.4.x) mediante su servidor DHCP.

## componentes del modo AP

Al configurar el ESP32 como AP, intervienen varios componentes:

1. SSID y Contraseña:

    - SSID (Service Set Identifier): Es el nombre de la red Wi-Fi creada.
    - Contraseña: Protege la red para que solo dispositivos autorizados puedan conectarse. Si no se configura una contraseña, la red estará abierta.

2. Canal Wi-Fi:

    - Especifica el canal (1-11 generalmente) que utiliza la red. pro defecto, se puede usar el canal 1.

3. Autenticacion:

    - define el nivel de seguridad de la red:
        - WIFI_AUTH_OPEN: Red abierta (sin contraseña)
        - WIFI_AUTH_WPA2_PSK: Cifra las conexiones usando una contraseña (la más común)

4. Servidor DHCP:
    
    - Asigna automaticamente direcciones IP a los dispositivos conectados.

5. Dirección IP del AP:

    - Por defecto, el ESP32 utiliza la dirección 192.168.4.1

## Ventajas:

- Independencia: No necesita un router externo. El ESP32 puede crear su propia red.

- Configuración Local: Es ideal para configuraciones iniciales donde se necesita ingresar datos, como credenciales de una red Wi-Fi.

- Conexiones Directas: Permite comunicación directa entre los dispositivos conectados.

## Funcionalidad:

El modo AP es comúnmente utilizado en:

- Configuraciones de IoT (Internet of Things):
    - El ESP32 actúa como un punto de acceso para que un usuario ingrese el SSID y la contraseña de la red Wi-Fi doméstica.
    - Una vez configurado, el ESP32 cambia al modo Estación para conectarse a esa red.

- Redes Locales para Aplicaciones Personalizadas:

    - Controlar dispositivos mediante una red local creada por el ESP32, como un servidor web o una aplicación móvil.

- Pruebas y Desarrollo:

    - Permite conectarse al ESP32 sin necesidad de una red externa.

## Limitaciones del modo AP

- Sin acceso a Internet: Por sí solo, el modo AP no tiene acceso a Internet.
    - Para proporcionar acceso a Internet, se debe usar el modo AP+Estación.

- Capacidad limitada de dispositivos: Aunque el ESP32 soporta hasta 10 conexiones, el rendimiento puede degradarse con muchas conexiones simultáneas.

- Cobertura Wi-Fi limitada: La potencia de transmisión del ESP32 es más baja que la de un router comercial, lo que limita el alcance de la red.

## Modo AP vs Modo estacion

| Característica       | Modo AP                                     | Modo Estación                              |
|----------------------|---------------------------------------------|--------------------------------------------|
| **Función principal**| Crear una red Wi-Fi                        | Conectarse a una red Wi-Fi existente       |
| **Dirección IP**     | Generalmente `192.168.4.1`                 | Asignada por el router                     |
| **Usuarios conectados** | Dispositivos conectados al ESP32          | Solo el ESP32 conectado al router          |
| **Uso típico**       | Configuración inicial o redes locales       | Acceso a Internet y redes externas         |

## Flujo básico del modo AP

1. Configuración inicial:
    - Define el SSID, contraseña, canal, y seguridad.
    - COnfigura el servidor DHCP y asigna la dirección IP del AP

2. Inicio del Wi-Fi:
    - Inicia el modo AP y comienza a transmitir la red.
3. Conexión de dispositivos:
    - Los clientes se conectan al SSID usando la contraseña.
4. Comunicación:
    - El ESP32 puede enviar y recibir datos de los dispositivos conectados.
