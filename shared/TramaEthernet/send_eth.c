/* Envia tramas Ethernet, con mecanismo de consulta por nombre (mini-ARP) */
#include "eth.h"

/* MAC broadcast */
const byte BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* Envía una trama tipo Q (consulta por nombre) */
void enviarConsulta(int sockfd, struct ifreq *sirDatos, int iIndex, char *nombreBuscado) {
  byte sbBufferEther[BUF_SIZ];
  struct ether_header *hdr = (struct ether_header *)sbBufferEther;
  struct sockaddr_ll sa;

  memset(sbBufferEther, 0, BUF_SIZ);
  /* MAC origen */
  for (int i = 0; i < 6; i++)
    hdr->ether_shost[i] = (unsigned char)sirDatos->ifr_hwaddr.sa_data[i];
  /* MAC destino = broadcast */
  for (int i = 0; i < 6; i++)
    hdr->ether_dhost[i] = BROADCAST_MAC[i];
  hdr->ether_type = htons(ETHER_TYPE);

  /* Payload: 'Q' + nombre buscado */
  char payload[64];
  sprintf(payload, "Q%s", nombreBuscado);
  strcpy((char *)(sbBufferEther + TRAMA_PAYLOAD), payload);

  memset(&sa, 0, sizeof(sa));
  sa.sll_family = AF_PACKET;
  sa.sll_protocol = htons(ETHER_TYPE);
  sa.sll_ifindex = iIndex;
  sa.sll_halen = ETH_ALEN;
  for (int i = 0; i < 6; i++)
    sa.sll_addr[i] = BROADCAST_MAC[i];

  sendto(sockfd, sbBufferEther, TRAMA_PAYLOAD + strlen(payload) + 4, 0,
         (struct sockaddr *)&sa, sizeof(sa));

  printf("→ Consulta enviada para host: %s\n", nombreBuscado);
}

/* Espera respuesta y obtiene MAC destino */
int esperarRespuesta(int sockfd, char *macDest) {
  byte sbBufferEther[BUF_SIZ];
  struct sockaddr saddr;
  int saddr_size = sizeof saddr;
  int len;

  printf("Esperando respuesta...\n");
  while (1) {
    len = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr,
                   (socklen_t *)(&saddr_size));
    char *payload = (char *)(sbBufferEther + TRAMA_PAYLOAD);
    if (payload[0] == 'R') {
      strcpy(macDest, payload + 1);
      printf("← Respuesta ARP recibida: %s\n", macDest);
      return 1;
    }
  }
  return 0;
}

/*Convierte una MAC en texto ("aa:bb:cc...") a bytes*/
void parseMacFromString(byte *dest, char *src) {
  unsigned int values[6];
  sscanf(src, "%x:%x:%x:%x:%x:%x",
         &values[0], &values[1], &values[2],
         &values[3], &values[4], &values[5]);
  for (int i = 0; i < 6; i++)
    dest[i] = (byte)values[i];
}

int main(int argc, char *argv[]) {
  int sockfd;
  struct ifreq sirDatos;
  int i, iLen, iLenHeader, iLenTotal, iIndex;
  byte sbBufferEther[BUF_SIZ], sbMac[6];
  struct ether_header *psehHeaderEther = (struct ether_header *)sbBufferEther;
  struct sockaddr_ll socket_address;
  char macDestinoTexto[32];

  if (argc != 3) {
    printf("Error en argumentos.\n\n");
    printf("Uso: send_eth INTERFACE NOMBRE_DESTINO\n");
    printf("Ejemplo: send_eth eth0 pc3\n\n");
    exit(1);
  }

  /*Abrir socket raw*/
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
    perror("socket");

  /*Obtener índice e interfaz*/
  memset(&sirDatos, 0, sizeof(struct ifreq));
  strncpy(sirDatos.ifr_name, argv[1], IFNAMSIZ - 1);
  if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0)
    perror("SIOCGIFINDEX");
  iIndex = sirDatos.ifr_ifindex;

  /*Obtener MAC local*/
  if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0)
    perror("SIOCGIFHWADDR");

  printf("Interfaz %s (idx %d) con MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
         argv[1], iIndex,
         (byte)sirDatos.ifr_hwaddr.sa_data[0], (byte)sirDatos.ifr_hwaddr.sa_data[1],
         (byte)sirDatos.ifr_hwaddr.sa_data[2], (byte)sirDatos.ifr_hwaddr.sa_data[3],
         (byte)sirDatos.ifr_hwaddr.sa_data[4], (byte)sirDatos.ifr_hwaddr.sa_data[5]);


  /*Enviar consulta por nombre */
  enviarConsulta(sockfd, &sirDatos, iIndex, argv[2]);

  /*Esperar respuesta con MAC */
  if (!esperarRespuesta(sockfd, macDestinoTexto)) {
    printf("No se obtuvo respuesta.\n");
    close(sockfd);
    return 1;
  }

  /*Construir y enviar la trama normal */
  parseMacFromString(sbMac, macDestinoTexto);

  /*Llenamos con 0 el buffer de datos (payload)*/
  memset(sbBufferEther, 0, BUF_SIZ);
  /*Dirección MAC Origen*/
  for (i = 0; i < 6; i++)
    psehHeaderEther->ether_shost[i] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[i];
  /*Dirección MAC destino*/
  for (i = 0; i < 6; i++)
    psehHeaderEther->ether_dhost[i] = sbMac[i];

  char scMsj[] = "D¡Hola desde emisor con mini-ARP!";
  strcpy((char *)(sbBufferEther + TRAMA_PAYLOAD), scMsj);

  iLenHeader = sizeof(struct ether_header);
  if (strlen(scMsj) > ETHER_TYPE) {
    printf("El mensaje debe ser mas corto o incremente ETHER_TYPE\n");
    close(sockfd);
    exit(1);
  }
  for (i = 0; (scMsj[i] && (i < ETHER_TYPE)); i++) {
    sbBufferEther[iLenHeader + i] = scMsj[i];
  }
  /* Rellenamos con espacios en blanco */
  if (i < ETHER_TYPE) {
    while (i < ETHER_TYPE) {
      sbBufferEther[iLenHeader + i] = ' ';
      i++;
    }
  }
  iLenHeader = iLenHeader + i;

  /*Tipo de protocolo o la longitud del paquete*/
  psehHeaderEther->ether_type = htons(ETHER_TYPE);
  /*Finalmente FCS*/
  for (i = 0; i < 4; i++) {
    sbBufferEther[iLenHeader + i] = 0;
  }
  /*Longitud total*/
  iLenTotal = iLenHeader + 4;

  /*Procedemos al envio de la trama*/
  memset(&socket_address, 0, sizeof(socket_address));
  socket_address.sll_family = AF_PACKET;
  socket_address.sll_protocol = htons(ETHER_TYPE);
  socket_address.sll_ifindex = iIndex;
  socket_address.sll_halen = ETH_ALEN;
  for (i = 0; i < 6; i++)
    socket_address.sll_addr[i] = sbMac[i];

  sendto(sockfd, sbBufferEther, iLenTotal, 0,
         (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));

  printf("→ Mensaje enviado a %s (%s)\n", argv[2], macDestinoTexto);

  close(sockfd);
  return 0;
}

