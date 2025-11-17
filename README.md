# Práctica 2: Comunicación entre dominios de colisión
En esta actividad se planteó un escenario compuesto por cinco hosts conectados
en dos dominios de colisión distintos. La topología se configuró de la siguiente
manera:
- Dominio A: pc1, pc2 y pc3
- Dominio B: pc3, pc4 y pc5

Esto significa que pc3 dispone de dos interfaces de red, una en cada dominio,
actuando como punto central entre ambas redes.

La configuración del archivo `lab.conf` quedó así:
```
# Dominio A
pc1[0]="A"
pc2[0]="A"
pc3[0]="A"

# Dominio B
pc3[1]="B"
pc4[0]="B"
pc5[0]="B"
```
Con esta configuración, Kathara crea dos switches virtuales independientes (A y B).
Sin embargo, los dominios permanecen completamente aislados entre sí, por lo que
los mensajes broadcast utilizados por el mini-ARP no pueden cruzar de un dominio
al otro.
Esto significa que, inicialmente, un host como pc1 no puede descubrir la MAC de
pc5, ya que ambos se encuentran en redes separadas.

## Solución: Unificar los dominios mediante un bridge
Para permitir que todos los hosts se comunicaran entre sí, se implementó un
**bridge** dentro de pc3.
Un bridge funciona como un switch software que reenvía tramas Ethernet entre
interfaces, permitiendo unir varios dominios de colisión en uno solo.

Dentro de pc3 se ejecutaron los siguientes comandos:
```bash
# Crear el bridge: esto crea un dispositivo virtual equivalente a un switch.
ip link add br0 type bridge

# Añadir las interfaces físicas al bridge: Al unirlas bajo el mismo bridge,
# ambas redes quedan integradas en un solo dominio lógico.
ip link set eth0 master br0
ip link set eth1 master br0

# Activar el bridge.
ip link set br0 up
```

Una vez creado el bridge, los dominios A y B quedaron conectados.
Esto permite que las tramas broadcast generadas por el mini-ARP puedan viajar
libremente entre todos los hosts.

## Resultado
Tras unir ambos dominios mediante el bridge, se verificó que:
- Un host en el dominio A (por ejemplo, pc1) puede enviar una solicitud de
  nombre hacia un host en el dominio B (por ejemplo, pc5).
- El broadcast llega a pc5, que responde correctamente.
- El mini-ARP funciona de extremo a extremo, permitiendo enviar tramas de datos
  entre hosts de diferentes dominios.

Cualquier host puede descubrir la MAC de cualquier otro host y enviar tramas
sin restricciones.

De esta manera se cumple el objetivo de la actividad: permitir que todos los
hosts se comuniquen entre sí aun estando originalmente en dominios de colisión
distintos.
