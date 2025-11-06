#include "eth.h"

int main(int argc, char *argv[]) {
  int sockfd;
  /*Esta estructura permite solicitar datos a la interface*/
  struct ifreq sirDatos;
  int i, iLen, iLenHeader, iLenTotal, iIndex;
  /*Buffer en donde estara almacenada la trama*/
  byte sbBufferEther[BUF_SIZ], sbMac[6];
  /*Para facilitar, hay una estructura para la cabecera Ethernet*/
  /*La cabecera Ethernet (psehHeaderEther) y sbBufferEther apuntan a lo mismo*/
  struct ether_header *psehHeaderEther = (struct ether_header *)sbBufferEther;
  struct sockaddr_ll socket_address; 
  /*Mensaje a enviar*/
  char scMsj[] = "En este arreglo de tipo char se coloca el mensaje que se enviara al "
                 "host deseado. La longitud maxima del mensaje es de 256 bytes dado "
                 "en ETHER_TYPE (eth.h); es posible cambiar la longitud o hacer que "
                 "sea variable, para esto ultimo tener cuidado.";
  if (argc!=3)
  {
    printf ("Error en argumentos.\n\n");
    printf ("seth INTERFACE MAC-DESTINO (Formato XXXXXXXXXXXX sin :).\n");
    printf ("Ejemplo: send_eth eth0 aabbccddeeff\n\n");
    exit (1);
  }
  /*Apartir de este este punto, argv[1] = Nombre de la interfaz y argv[2] */
  /*contiene la MAC destino, la MAC origen ya se conoce.                  */ 
  /*Abre el socket. Para que sirven los parametros empleados?*/
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) perror("socket");
  
  /* Mediante el nombre de la interface (i.e. eth0), se obtiene su indice */
  memset (&sirDatos, 0, sizeof(struct ifreq)); 
  for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];
  if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0) perror("SIOCGIFINDEX");
  iIndex = sirDatos.ifr_ifindex;
  
  /*Ahora obtenemos la MAC de la interface por donde saldran los datos */
  memset(&sirDatos, 0, sizeof(struct ifreq));
  for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];
  if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0) perror("SIOCGIFHWADDR");
  
  /*Se imprime la MAC del host*/
  printf ("Iterface de salida: %u, con MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
          (byte)(iIndex), 
          (byte)(sirDatos.ifr_hwaddr.sa_data[0]), (byte)(sirDatos.ifr_hwaddr.sa_data[1]), 
          (byte)(sirDatos.ifr_hwaddr.sa_data[2]), (byte)(sirDatos.ifr_hwaddr.sa_data[3]), 
          (byte)(sirDatos.ifr_hwaddr.sa_data[4]), (byte)(sirDatos.ifr_hwaddr.sa_data[5]));

  /*Ahora se construye la trama Ethernet empezando por su encabezado. El   */
  /*formato, para la trama IEEE 802.3 version 2, es:                       */ 
  /*6         bytes de MAC Origen                                          */
  /*6         bytes de MAC Destino                                         */
  /*2         bytes para longitud de la trama o Ether_Type                 */ 
  /*46 a 1500 bytes de payload                                             */
  /*4         bytes Frame Check Sequence                                   */
  /*Total sin contar bytes de sincronizacion, va de 64 a 1518 bytes.       */
  memset(sbBufferEther, 0, BUF_SIZ);  /*Llenamos con 0 el buffer de datos (payload)*/
  /*Direccion MAC Origen*/
  psehHeaderEther->ether_shost[0] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[0];
  psehHeaderEther->ether_shost[1] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[1];
  psehHeaderEther->ether_shost[2] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[2];
  psehHeaderEther->ether_shost[3] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[3];
  psehHeaderEther->ether_shost[4] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[4];
  psehHeaderEther->ether_shost[5] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[5];
  /*Direccion MAC destino*/
  vConvierteMAC (sbMac, argv[2]);
  psehHeaderEther->ether_dhost[0] = sbMac[0];
  psehHeaderEther->ether_dhost[1] = sbMac[1];
  psehHeaderEther->ether_dhost[2] = sbMac[2];
  psehHeaderEther->ether_dhost[3] = sbMac[3];
  psehHeaderEther->ether_dhost[4] = sbMac[4];
  psehHeaderEther->ether_dhost[5] = sbMac[5];
  /*Antes de colocar la longitud de la trama o ETHER_TYPE, colocamos*/
  /*el payload que basicamente es rellenar de letras el mensaje*/
  iLenHeader = sizeof(struct ether_header);
  if (strlen(scMsj)>ETHER_TYPE)
  {
    printf ("El mensaje debe ser mas corto o incremente ETHER_TYPE\n");
    close (sockfd);
    exit (1);
  }
  for (i=0; ((scMsj[i])&&(i<ETHER_TYPE)); i++) sbBufferEther[iLenHeader+i] = scMsj[i];
  if (i<ETHER_TYPE)
  { /*Rellenamos con espacios en blanco*/
    while (i<ETHER_TYPE)
    {
      sbBufferEther[iLenHeader+i] = ' ';  i++;
    }
  }
  iLenHeader = iLenHeader + i;   
  /*Tipo de protocolo o la longitud del paquete*/
  psehHeaderEther->ether_type = htons(ETHER_TYPE); 
  /*Finalmente FCS*/
  for (i=0; i<4; i++) sbBufferEther[iLenHeader+i] = 0;
  iLenTotal = iLenHeader + 4; /*Longitud total*/

  /*Procedemos al envio de la trama*/
  socket_address.sll_ifindex = iIndex;
  socket_address.sll_halen = ETH_ALEN;
  socket_address.sll_addr[0] = sbMac[0];
  socket_address.sll_addr[1] = sbMac[1];
  socket_address.sll_addr[2] = sbMac[2];
  socket_address.sll_addr[3] = sbMac[3];
  socket_address.sll_addr[4] = sbMac[4];
  socket_address.sll_addr[5] = sbMac[5];
  iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
  if (iLen<0) printf("Send failed\n");
  printf ("\nContenido de la trama enviada:\n\n");
  vImprimeTrama (sbBufferEther);
  /*Cerramos*/
  close (sockfd);
  return 0;
}
