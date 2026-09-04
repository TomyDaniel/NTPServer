# Seguridad en NTP

## ¿Cuáles son las vulnerabilidades de NTP?

- **Sin autenticación por defecto**: los paquetes UDP del protocolo NTP clásico no tienen forma nativa de verificar que provienen de quien dicen venir. Cualquiera que pueda inyectar tráfico puede hacerse pasar por un server legítimo y devolver una hora falsa.
- **UDP sin estado**: al no haber un handshake previo como en TCP, es más fácil spoofear la IP de origen de los paquetes.
- **Amplificación para DDoS**: el comando histórico `monlist` permitía que un paquete chico generara una respuesta gigante, algo que se explotó para ataques de amplificación/reflexión contra terceros.
- **Delay attacks**: un atacante posicionado en el camino de la red puede introducir latencia asimétrica en los paquetes para desviar el offset calculado por el cliente, sin necesariamente falsificar el contenido del timestamp en sí.

## ¿Qué implicancias podría tener que se exploten esas vulnerabilidades?

- Romper la validación de certificados TLS, que dependen de que la hora del sistema sea razonablemente correcta para chequear expiración de certificados.
- Habilitar ataques de replay en sistemas que dependen de ventanas de tiempo.
- Falsear logs y evidencia forense — timestamps incorrectos complican la investigación de incidentes de seguridad.
- En sistemas financieros o de trading, desincronizar la hora puede alterar el orden percibido de las transacciones.
- Uso como vector de DDoS por amplificación contra terceros.

## ¿Puedes realizar un ataque de tipo MITM sobre un servicio NTP?

Sí, es técnicamente posible en el protocolo NTP clásico, precisamente por las vulnerabilidades listadas arriba: al no haber autenticación criptográfica entre cliente y servidor, un atacante en posición de intermediario puede interceptar o suplantar las respuestas NTP y hacer que el cliente acepte una hora arbitraria.

## Configuración NTP actual

Como parte del trabajo práctico se implementó un servidor NTP simplificado sobre una ESP8266:

- La placa se conecta en modo dual **AP + STA** (`WIFI_AP_STA`): por STA se sincroniza contra `pool.ntp.org` usando `configTime()`, y por AP levanta una red propia ("Server Test") donde actúa como servidor NTP para otros dispositivos.
- El servidor escucha en el puerto UDP **123** mediante `WiFiUDP`, arma una respuesta NTP de 48 bytes con Mode = 4 (server), y completa los campos de timestamp (Reference, Originate, Receive, Transmit) usando la hora obtenida de `time()` convertida al epoch NTP (1900).
- No implementa autenticación ni NTS — es una implementación básica con fines de aprendizaje del protocolo, por lo que en su forma actual es vulnerable a las mismas debilidades que el NTP clásico.

## Cómo configurarías tu host para evitar problemas con NTP, y cómo verificás que la configuración es correcta

**Configuración recomendada:**
- Usar una implementación moderna como **NTPsec**, que reduce la superficie de ataque respecto al `ntpd` clásico.
- Habilitar **NTS** cuando el servidor lo soporte, para autenticar criptográficamente el intercambio y evitar que un atacante inyecte respuestas falsas sin ser detectado.
- Configurar múltiples servidores de tiempo confiables y comparar sus respuestas, en vez de depender de una única fuente.
- Restringir por firewall qué IPs pueden enviar respuestas NTP hacia el host.
- Loguear y alertar ante saltos anómalos de offset.

**Cómo verificar que la configuración es correcta:**
- En Linux, `chronyc sources -v` o `ntpq -p` muestran contra qué servidores está sincronizado el sistema y el offset actual.
- Revisar los logs de `chronyd`/`ntpsec` para confirmar que las sesiones indican uso de NTS cuando corresponde.

## ¿Conocés ntp.inti.gob.ar? ¿Es seguro?

Sí, es el servidor de hora oficial de Argentina, mantenido por el INTI, que es la autoridad de referencia horaria del país. Es un servidor legítimo y de uso común como fuente confiable en configuraciones locales.