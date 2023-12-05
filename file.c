#include <stdio.h>
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
#include <linux/stat.h>
char sbuf[500];
void convertToGray(const char *inputPath)
{
	int inputFile = open(inputPath, O_RDWR);

	if (inputFile == -1)
	{
		perror("Error opening input file");
		exit(EXIT_FAILURE);
	}
	unsigned char pixel[3];
	lseek(inputFile, 54, SEEK_SET);

	while (read(inputFile, pixel, 3) == 3)
	{
		unsigned char grayValue = (unsigned char)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);

		lseek(inputFile, -3, SEEK_CUR);
		unsigned char grayPixel[3] = {grayValue, grayValue, grayValue};
		write(inputFile, grayPixel, 3);
	}

	close(inputFile);
}
void printAccessRights(struct stat buf, int ofile)
{
	sprintf(sbuf, "drepturi de access user ");
	strcat(sbuf, ((buf.st_mode & S_IRUSR) ? "R" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IWUSR) ? "W" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IXUSR) ? "X\n" : "-\n"));
	write(ofile, sbuf, strlen(sbuf));
	sprintf(sbuf, "drepturi de access group ");
	strcat(sbuf, ((buf.st_mode & S_IRGRP) ? "R" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IWGRP) ? "W" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IXGRP) ? "X\n" : "-\n"));
	write(ofile, sbuf, strlen(sbuf));
	sprintf(sbuf, "drepturi de access other ");
	strcat(sbuf, ((buf.st_mode & S_IROTH) ? "R" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IWOTH) ? "W" : "-"));
	strcat(sbuf, ((buf.st_mode & S_IXOTH) ? "X\n" : "-\n"));
	write(ofile, sbuf, strlen(sbuf));
}

void processDir(char *inFile, char *outFile, struct dirent *dirfile, int *writtenLines)
{
	*writtenLines = 0;
	struct stat buf;
	stat(inFile, &buf);

	int ofile = open(outFile, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (ofile == -1)
	{
		exit(EXIT_FAILURE);
	}
	sprintf(sbuf, "nume director: %s\n", dirfile->d_name);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	sprintf(sbuf, "identificatorul utilizatorului: %d\n", buf.st_uid);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	printAccessRights(buf, ofile);
	(*writtenLines) += 3;
	close(ofile);
}
void processLink(char *inFile, char *outFile, struct dirent *dirfile, int *writtenLines)
{
	*writtenLines = 0;
	struct stat buf;
	stat(inFile, &buf);
	int ofile = open(outFile, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (ofile == -1)
	{
		exit(EXIT_FAILURE);
	}
	sprintf(sbuf, "nume legatura: %s\n", dirfile->d_name);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	sprintf(sbuf, "dimensiunea legaturii: %ld\n", buf.st_size);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	printAccessRights(buf, ofile);
	(*writtenLines) += 3;
	close(ofile);
}

void processRegularFile(char *inFile, char *outFile, struct dirent *dirfile, int *writtenLines)
{
	*writtenLines = 0;
	struct stat buf;
	stat(inFile, &buf);
	int ofile = open(outFile, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (ofile == -1)
	{
		exit(EXIT_FAILURE);
	}
	sprintf(sbuf, "nume fisier: %s\n", dirfile->d_name);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	if (strstr(dirfile->d_name, ".bmp") != NULL)
	{
		int fd = open(inFile, O_RDONLY);
		char widthxheight[8];
		lseek(fd, 32, SEEK_SET);
		read(fd, widthxheight, 8);
		sprintf(sbuf, "inaltime:%d\n lungime:%d\n", *(uint32_t *)widthxheight, *(uint32_t *)&widthxheight[4]);
		write(ofile, sbuf, strlen(sbuf));
		(*writtenLines) += 2;
		close(fd);
	}
	sprintf(sbuf, "dimensiunea fisierului: %ld\n", buf.st_size);
	write(ofile, sbuf, strlen(sbuf));
	(*writtenLines)++;
	printAccessRights(buf, ofile);
	(*writtenLines) += 3;
	close(ofile);
}
int main(int argc, char **argv)
{
	DIR *directory = opendir(argv[1]);

	if (!directory)
	{
		exit(1);
	}
	if (access(argv[2], F_OK) != -1)
	{
		printf("Directory already exists.\n");
	}
	else
	{
		// Create the directory
		if (mkdir(argv[2], 0777) == 0)
		{
			printf("Directory created successfully.\n");
		}
		else
		{
			perror("Error creating directory");
			return EXIT_FAILURE;
		}
	}

	struct dirent *dirfile = readdir(directory);

	char statisticaPath[500];
	sprintf(statisticaPath, "%s%s", argv[2], "statistica.txt");
	printf("statisticapath: %s\n\n", statisticaPath);
	int statfile = open(statisticaPath, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0666);
	close(statfile);
	char relpath[500];
	strcpy(relpath, argv[1]);

	while (dirfile != NULL)
	{
		char path[500];
		strcpy(path, relpath);
		strcat(path, dirfile->d_name);
		struct stat buf;
		lstat(path, &buf);
		int wLines = 0;
		switch (buf.st_mode & S_IFMT)
		{

		case S_IFDIR:

			int id_dirProcess = fork();
			if (id_dirProcess == -1)
			{
				perror("fork");
				exit(EXIT_FAILURE);
			}
			if (id_dirProcess == 0)
			{
				char filepath[500];
				sprintf(filepath, "%s%s_statistica.txt", argv[2], dirfile->d_name);
				printf("DIR: %s\n", filepath);
				processDir(path, filepath, dirfile, &wLines);
				int statFile = open(statisticaPath, O_WRONLY | O_APPEND);
				if (statFile == -1)
				{
					perror("scriere in stat linii");
					exit(EXIT_FAILURE);
				}
				sprintf(sbuf, "process wrote %d lines\n", wLines);
				write(statFile, sbuf, strlen(sbuf));
				close(statFile);

				exit(EXIT_SUCCESS);
			}
			int status;
			if (waitpid(id_dirProcess, &status, 0) == id_dirProcess)
			{
				printf("process with id: %d ended with status: %d\n\n", id_dirProcess, status);
			}

			break;
		case S_IFLNK:

			int id_linkProcess = fork();
			if (id_linkProcess == -1)
			{
				perror("fork");
				exit(EXIT_FAILURE);
			}
			if (id_linkProcess == 0)
			{
				char filepath[500];
				sprintf(filepath, "%s%s_statistica.txt", argv[2], dirfile->d_name);
				printf("LINK: %s\n", filepath);

				processLink(path, filepath, dirfile, &wLines);
				int statFile = open(statisticaPath, O_WRONLY | O_APPEND);
				if (statFile == -1)
				{
					perror("scriere in stat linii");
					exit(EXIT_FAILURE);
				}
				sprintf(sbuf, "process wrote %d lines\n", wLines);
				write(statFile, sbuf, strlen(sbuf));
				close(statFile);

				exit(EXIT_SUCCESS);
			}

			if (waitpid(id_linkProcess, &status, 0) == id_dirProcess)
			{
				printf("process with id: %d ended with status: %d\n\n", id_linkProcess, status);
			}

			break;

		case S_IFREG:

			int id_regularFileprocess = fork();

			if (id_regularFileprocess == -1)
			{
				perror("fork");
				exit(EXIT_FAILURE);
			}
			if (id_regularFileprocess == 0)
			{
				char filepath[500];
				sprintf(filepath, "%s%s_statistica.txt", argv[2], dirfile->d_name);
				printf("FILE: %s\n", filepath);

				processRegularFile(path, filepath, dirfile, &wLines);
				int statFile = open(statisticaPath, O_WRONLY | O_APPEND);
				if (statFile == -1)
				{
					perror("scriere in stat linii");
					exit(EXIT_FAILURE);
				}
				sprintf(sbuf, "process wrote %d lines\n", wLines);
				write(statFile, sbuf, strlen(sbuf));
				close(statFile);

				exit(EXIT_SUCCESS);
			}

			if (waitpid(id_regularFileprocess, &status, 0) == id_regularFileprocess)
			{
				printf("process with id: %d ended with status: %d\n\n", id_regularFileprocess, status);
			}
			if (strstr(dirfile->d_name, ".bmp") != NULL)
			{
				{
					// bmp stuff

					int id_grayscale = fork();
					if (id_grayscale == -1)
					{
						perror("fork");
						exit(EXIT_FAILURE);
					}
					if (id_grayscale == 0)
					{
						convertToGray(path);
						exit(EXIT_SUCCESS);
					}
					if (waitpid(id_regularFileprocess, &status, 0) == id_regularFileprocess)
					{
						printf("process with id: %d ended with status: %d\n\n", id_regularFileprocess, status);
					}
				}
			}

			break;
		default:
			break;
		}
		dirfile = readdir(directory);
	}
	return 0;
}