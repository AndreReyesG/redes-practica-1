/*Escucha todos los paquetes ethernet que llegan, pero se*/
/*desea el que le corresponde*/
#include "eth.h"

 int main(int argc, char *argv[])
 {
   int sockfd, i, iTramaLen;
   ssize_t numbytes;
   byte sbBufferEther[BUF_SIZ];
   /*La cabecera Ethernet (eh) y sbBufferEther apuntan a lo mismo*/
   struct ether_header *eh = (struct ether_header *)sbBufferEther;
   int saddr_size; 
   struct sockaddr saddr;    
   struct ifreq sirDatos;
   int iEtherType;
  
   if (argc!=2)
   {
     printf ("Error en argumentos.\n\n");
     printf ("ethdump INTERFACE\n");
     printf ("Ejemplo: recv_eth eth0\n\n");
     exit (1);
   }
   /*Apartir de este este punto, argv[1] = Nombre de la interfaz.          */
   
   /*Podriamos recibir tramas de nuestro "protocolo" o de cualquier protocolo*/
   /*sin embargo, evitaremos esto y recibiremos de todo. Hay que tener cuidado*/
   /*if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1)*/
   /*Se abre el socket para "escuchar" todo sin pasar por al CAR*/
   if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) 
   {
     perror("Listener: socket"); 
     return -1;
   }

   /*Ahora obtenemos la MAC de la interface del host*/
   memset(&sirDatos, 0, sizeof(struct ifreq));
   for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];
   if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0) perror("SIOCGIFHWADDR");
   
   /*Se imprime la MAC del host*/
   printf ("Direccion MAC de la interfaz de entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(sirDatos.ifr_hwaddr.sa_data[0]), (byte)(sirDatos.ifr_hwaddr.sa_data[1]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[2]), (byte)(sirDatos.ifr_hwaddr.sa_data[3]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[4]), (byte)(sirDatos.ifr_hwaddr.sa_data[5])); 
   
   /*Se mantiene en escucha*/ 
   do 
   { /*Capturando todos los paquetes*/   
     saddr_size = sizeof saddr;
     iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size)); 
     /*Recibe todo lo que llegue. Llegara el paquete a otras capas dentro del host?*/   
     if (iLaTramaEsParaMi(sbBufferEther, &sirDatos))
     {
       printf ("\nContenido de la trama recibida:\n");    
       vImprimeTrama (sbBufferEther);
     }
   } while (1);
   close(sockfd); 
   return (0);
 }