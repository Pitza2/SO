#include<stdio.h>  
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
char sbuf[500];
void convert_to_grayscale(int file) {
  if (file == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  lseek(file,0,SEEK_SET);
  char header[54];
  if (read(file, header, sizeof(header)) != sizeof(header)) {
    perror("read");
    close(file);
    exit(EXIT_FAILURE);
  }

  int width = *(int*)&header[18];
  int height = *(int*)&header[22];
  int size = width * height * 3;

  unsigned char *original_image = (unsigned char*)malloc(size);
  if (original_image == NULL) {
    perror("malloc");
    close(file);
    exit(EXIT_FAILURE);
  }

  if (read(file, original_image, size) != size) {
    perror("read");
    free(original_image);
    close(file);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; i += 3) {
    unsigned char blue = original_image[i];
    unsigned char green = original_image[i + 1];
    unsigned char red = original_image[i + 2];


    unsigned char grayscale = (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);

    original_image[i] = original_image[i + 1] = original_image[i + 2] = grayscale;
  }
  lseek(file,0,SEEK_SET);

  if (write(file, header, sizeof(header)) != sizeof(header)) {
    perror("write");
    free(original_image);
    close(file);
    exit(EXIT_FAILURE);
  }

  if (write(file, original_image, size) != size) {
    perror("write");
    free(original_image);
    close(file);
    exit(EXIT_FAILURE);
  }

  free(original_image);
  close(file);
}
void printAccessRights(struct stat buf, int ofile){
  sprintf (sbuf, "drepturi de access user ");
  strcat (sbuf, ((buf.st_mode & S_IRUSR) ? "R" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IWUSR) ? "W" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IXUSR) ? "X\n" : "-\n"));
  write (ofile, sbuf, strlen (sbuf));
  sprintf (sbuf, "drepturi de access group ");
  strcat (sbuf, ((buf.st_mode & S_IRGRP) ? "R" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IWGRP) ? "W" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IXGRP) ? "X\n" : "-\n"));
  write (ofile, sbuf, strlen (sbuf));
  sprintf (sbuf, "drepturi de access other ");
  strcat (sbuf, ((buf.st_mode & S_IROTH) ? "R" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IWOTH) ? "W" : "-"));
  strcat (sbuf, ((buf.st_mode & S_IXOTH) ? "X\n" : "-\n"));
  write (ofile, sbuf, strlen (sbuf)); 
}
int main (int argc, char **argv)
{
  DIR* directory=opendir(argv[1]);
  
  if(!directory){exit(1);}
  
  struct dirent* dirfile=readdir(directory);


  int filecount=0;

  char relpath[500];
  strcpy(relpath,argv[1]);
  
  while(dirfile!=NULL){
    int id;
    if((id=fork())==-1){
      perror("fork failed");
      exit(EXIT_FAILURE);
    }

    if(id==0){

      char path[500];
      strcpy(path,relpath);
      strcat(path,dirfile->d_name);
      int file = open (path, O_RDWR);
      struct stat buf;
      lstat (path, &buf);


      switch (buf.st_mode & S_IFMT) {
        int linecount;
      case S_IFDIR:
        {
          linecount=0;
          char filepath[500]; strcpy(filepath,argv[2]);strcat(filepath,"/");
          strcat(filepath,dirfile->d_name);strcat(filepath,"/_statistica.txt");
          int ofile = open (filepath, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
          sprintf(sbuf,"nume director: %s\n",dirfile->d_name); linecount++;
          write (ofile, sbuf, strlen (sbuf));

          sprintf(sbuf,"identificatorul utilizatorului: %d\n",buf.st_uid);linecount++;
          write (ofile, sbuf, strlen (sbuf));
          printAccessRights(buf,ofile);linecount+=3;
          close (ofile);
        }
        close(file);

        break;
      case S_IFLNK:
        {
          linecount=0;
          char filepath[500]; strcpy(filepath,argv[2]);strcat(filepath,"/");
          strcat(filepath,dirfile->d_name);strcat(filepath,"_statistica.txt");
          int ofile = open (filepath, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
          sprintf(sbuf,"nume legatura: %s\n",dirfile->d_name); linecount++;
          write (ofile, sbuf, strlen (sbuf));
          struct stat buf2; fstat(file,&buf2);
          sprintf(sbuf,"dimensiunea legaturii: %ld\n",buf2.st_size); linecount++;
          write (ofile, sbuf, strlen (sbuf));
          printAccessRights(buf,ofile); linecount+=3;
          close (ofile);
        }
        close(file);
        break;


      case S_IFREG:
        {
          linecount=0;
          uint32_t rez[2];
          lseek(file, 18, SEEK_CUR);
          read(file,rez,8);

          char filepath[500]; strcpy(filepath,argv[2]);strcat(filepath,"/");
          strcat(filepath,dirfile->d_name);strcat(filepath,"_statistica.txt");
          int ofile = open (filepath, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);

          sprintf(sbuf,"nume fisier: %s\n",dirfile->d_name);
          write (ofile, sbuf, strlen (sbuf));linecount++;

          char* extDot=strrchr(dirfile->d_name, '.');
          if(extDot && !strcmp(extDot,".bmp")){
            sprintf(sbuf,"rezolution is %dx%d\n",rez[0],rez[1]);
            write (ofile, sbuf, strlen (sbuf));linecount++;

          int idgrayscale;
          if((idgrayscale=fork())==-1){
            printf("nasol");
            exit(-1);
          }
          if(idgrayscale==0){
            close(file);
            file=open(filepath,O_RDWR);
            convert_to_grayscale(file);
            exit(0);
          }else{
           int status=-1;
           int i= waitpid(idgrayscale,&status,0);
           printf("process ended with status %d and pid %d\n",status,i);
           if(WIFEXITED(status)){
            printf("1\n");
           }else if( WIFSIGNALED(status)){
            printf("2\n");
           }else if(WTERMSIG(status)){
            printf("3\n");
           }else if(WCOREDUMP(status)){
            printf("4\n");
           }else if(WIFSTOPPED(status)){
            printf("5\n");
           }else if(WSTOPSIG(status)){
            printf("6\n");
           }else if(WIFCONTINUED(status)){
            printf("7\n");
           }
         }
          }

          sprintf (sbuf, "file total size is: %ld\n", buf.st_size);
          write (ofile, sbuf, strlen (sbuf));linecount++;
          sprintf (sbuf, "number of hard links: %ld\n", buf.st_nlink);
          write (ofile, sbuf, strlen (sbuf));
          sprintf (sbuf, "User ID of Owner: %d\n", buf.st_uid);
          write (ofile, sbuf, strlen (sbuf));linecount++;
          printAccessRights(buf,ofile);linecount+=3;
          close(ofile);
        }
        close (file);
        break;
      default:
        close(file);
        break;
      }
      exit(0);
    }
    
    dirfile=readdir(directory);
    filecount++;
  }
  for(int i=0;i<filecount;i++){
    int wstatus;
    int id=wait(&wstatus);
    printf("process ended with pid: %d and status: %d\n",id,wstatus);
  }


  return 0;

}

