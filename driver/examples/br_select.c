#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/if_tun.h>

#define max(a,b) ((a)>(b) ? (a):(b))

int main(int argc, char *argv[])
{
   char buf[1600];
   int f1,f2,l,fm;
   fd_set fds;
 
   if(argc < 2) {
      printf("Usage: bridge tap|tun\n");
      exit(1);
   }

   sprintf(buf,"/dev/%s%d",argv[1],0);
   f1 = open(buf, O_RDWR);

   sprintf(buf,"/dev/%s%d",argv[1],1);
   f2 = open(buf, O_RDWR);

   fm = max(f1, f2) + 1;

   ioctl(f1, TUNSETNOCSUM, 1); 
   ioctl(f2, TUNSETNOCSUM, 1); 

   while(1){
	FD_ZERO(&fds);
        FD_SET(f1, &fds);
        FD_SET(f2, &fds);

	select(fm, &fds, NULL, NULL, NULL);

	if( FD_ISSET(f1, &fds) ) {
	   l = read(f1,buf,sizeof(buf));
           write(f2,buf,l);
	}
	if( FD_ISSET(f2, &fds) ) {
	   l = read(f2,buf,sizeof(buf));
           write(f1,buf,l);
	}
   }
}
