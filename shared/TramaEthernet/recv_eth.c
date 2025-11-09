/* Escucha todos los paquetes ethernet que llegan y responde a consultas por nombre (mini-ARP) */
#include "eth.h"

/* Dirección broadcast */
const byte BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* Función auxiliar: compara si una trama es broadcast */
int esBroadcast(char *psTrama) {
  for (int i = 0; i < LEN_MAC; i++)
    if ((unsigned char)psTrama[TRAMA_DESTINATION + i] != 0xff)
      return 0;
  return 1;
}

/* Envia una trama de respuesta con la MAC del host local */
void enviarRespuesta(int sockfd, struct ifreq *psirDatos, char *psTramaRecibida) {
  byte sbTrama[BUF_SIZ];
  struct ether_header *ehResp = (struct ether_header *)sbTrama;
  struct sockaddr_ll sa;

  /* MAC destino = origen de la trama recibida */
  for (int i = 0; i < 6; i++)
    ehResp->ether_dhost[i] = (unsigned char)psTramaRecibida[TRAMA_SOURCE + i];

  /* MAC origen = la de este host */
  for (int i = 0; i < 6; i++)
    ehResp->ether_shost[i] = (unsigned char)psirDatos->ifr_hwaddr.sa_data[i];

  ehResp->ether_type = htons(ETHER_TYPE);

  /* Payload: R + dirección MAC en formato texto */
  char payload[64];
  sprintf(payload, "R%02x:%02x:%02x:%02x:%02x:%02x",
          (byte)psirDatos->ifr_hwaddr.sa_data[0], (byte)psirDatos->ifr_hwaddr.sa_data[1],
          (byte)psirDatos->ifr_hwaddr.sa_data[2], (byte)psirDatos->ifr_hwaddr.sa_data[3],
          (byte)psirDatos->ifr_hwaddr.sa_data[4], (byte)psirDatos->ifr_hwaddr.sa_data[5]);

  strcpy((char *)(sbTrama + TRAMA_PAYLOAD), payload);

  /* Enviar la trama */
  memset(&sa, 0, sizeof(sa));
  sa.sll_family = AF_PACKET;
  sa.sll_protocol = htons(ETHER_TYPE);
  sa.sll_ifindex = if_nametoindex(psirDatos->ifr_name);
  sa.sll_halen = ETH_ALEN;
  for (int i = 0; i < 6; i++)
    sa.sll_addr[i] = ehResp->ether_dhost[i];

  sendto(sockfd, sbTrama, TRAMA_PAYLOAD + strlen(payload) + 4, 0,
         (struct sockaddr *)&sa, sizeof(sa));

  printf("→ Respondiendo consulta ARP con mi MAC.\n");
}

int main(int argc, char *argv[]) {
  int sockfd, i, iTramaLen;
  byte sbBufferEther[BUF_SIZ];
  struct sockaddr saddr;
  struct ifreq sirDatos;
  int saddr_size;
  char nombreLocal[64];

  if (argc != 2) {
    printf("Error en argumentos.\n\n");
    printf("Uso: recv_eth INTERFACE\n");
    printf("Ejemplo: recv_eth eth0\n\n");
    exit(1);
  }

  /* Obtener nombre del host */
  gethostname(nombreLocal, sizeof(nombreLocal));
  printf("Nombre local: %s\n", nombreLocal);

  /* Abrir socket raw */
  if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
    perror("Listener: socket");
    return -1;
  }

  /* Obtener la MAC local */
  memset(&sirDatos, 0, sizeof(struct ifreq));
  strncpy(sirDatos.ifr_name, argv[1], IFNAMSIZ - 1);
  if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0)
    perror("SIOCGIFHWADDR");

  printf("MAC local: %02x:%02x:%02x:%02x:%02x:%02x\n",
         (byte)sirDatos.ifr_hwaddr.sa_data[0], (byte)sirDatos.ifr_hwaddr.sa_data[1],
         (byte)sirDatos.ifr_hwaddr.sa_data[2], (byte)sirDatos.ifr_hwaddr.sa_data[3],
         (byte)sirDatos.ifr_hwaddr.sa_data[4], (byte)sirDatos.ifr_hwaddr.sa_data[5]);

  /* Escuchar tramas */
  while (1) {
    saddr_size = sizeof saddr;
    iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr,
                         (socklen_t *)(&saddr_size));

    if (iLaTramaEsParaMi(sbBufferEther, &sirDatos) || esBroadcast(sbBufferEther)) {
      char *payload = (char *)(sbBufferEther + TRAMA_PAYLOAD);
      char tipo = payload[0];

      if (tipo == 'Q') {
        printf("Consulta recibida: %s\n", payload + 1);
        if (strcmp(payload + 1, nombreLocal) == 0)
          enviarRespuesta(sockfd, &sirDatos, sbBufferEther);
      } else if (tipo == 'R') {
        printf("Respuesta recibida (MAC): %s\n", payload + 1);
      } else if (tipo == 'D') {
        printf("\nTrama normal recibida:\n");
        vImprimeTrama(sbBufferEther);
      }
    }
  }

  close(sockfd);
  return 0;
}

