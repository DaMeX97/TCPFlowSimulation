#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
 
int main(int argc, char *argv[]) {
	int sim_number = 1;
	int flow_number = 1;
	int status;

	pid_t wpid;

	if(argc < 3) {
		printf("Need these arguments: number_sim - number_flow_per_sim");
        exit(EXIT_FAILURE);
	}

	sim_number = atoi(argv[1]);
	flow_number = atoi(argv[2]);

	system("rm -rf OutputFiles && rm -rf LogFiles");
	system("mkdir OutputFiles && mkdir LogFiles");

	// build before running, without this we can have error of multiple building !!!
	system("../../waf build");

	int count = 0;
	int flag = 1;
	while(flag) {
		pid_t pid = fork();

		if(pid < 0) {
			flag = 0;
			perror("Fork error");
		}
		if(pid == 0) {
			flag = 0;
			char str[400];

			sprintf(str, "cd ../.. && ./waf --run \"TCPFlowSimulation --SimNumber=%d && --FlowNumber=%d && --Seed=%d\" > ./scratch/TCPFlowSimulation/LogFiles/output%d.txt && cd -", count, flow_number, count, count);
		
			printf("%s\n", str);

			system(str);

			exit(EXIT_SUCCESS);
		}
		else {
			count++;
			
			if(count % 4 == 0) {
				while ((wpid = wait(&status)) > 0);
			}

			if(count == sim_number)
				flag = 0;
		}
	}

	while ((wpid = wait(&status)) > 0);

	// now all simulations have finished

	double media = 0;
	double varianza = 0;
	int countTotal = 0;

	printf("Looking for stats files...\n");

	double mediumTimes[sim_number];

	for(int i = 0; i < sim_number; i++) {

		double resTime[flow_number];

		for(int q = 0; q < flow_number; q++) {
			resTime[q] = 0;
		}

		char str[12];
		char path[255];
		sprintf(str, "ris%d.txt", i);

		strcpy(path, "./OutputFiles/");
		strcat(path, str);
	
		/* declare a file pointer */
		FILE    *infile;
		char    *buffer;
		long    numbytes;

		/* open an existing file for reading */
		infile = fopen(path, "r");

		/* quit if the file does not exist */
		if(infile == NULL) {
			printf("Unable to open %s, ignoring it.\n", path);
		}
		else {
			countTotal++;
			/* Get the number of bytes */
			fseek(infile, 0L, SEEK_END);
			numbytes = ftell(infile);

			/* reset the file position indicator to 
			the beginning of the file */
			fseek(infile, 0L, SEEK_SET);	

			/* grab sufficient memory for the 
			buffer to hold the text */
			buffer = (char*)calloc(numbytes, sizeof(char));	

			/* memory error */
			if(buffer == NULL)
				return 1;

			/* copy all the text into the buffer */
			fread(buffer, sizeof(char), numbytes, infile);
			fclose(infile);

			/* confirm we have read the file by
			outputing it to the console */

			char** rows = str_split(buffer, '\n');

			double tempo = 0;
			int count = 0;
			double med = 0;

			for(int j = 0; *(rows + j); j++) {
				double n_packets = 0;
				double time = 0;

				sscanf(rows[j], "%lf,%lf", &n_packets, &time);

				resTime[j] = time;

				med += resTime[j];

				//printf("Oiiii %.2f\n", resTime[j]);

				//tempo += time;
				
				count++;

				free(*(rows+j));
			}

			free(rows);

			/* free the memory we used for the buffer */
			free(buffer);

			//double med = 0;
			//for(int q = 0; q < count; q++) {
			//	med += resTime[q];
			//}

			med = med / count;

			printf("Media della simulazione n. %d è %.2f\n", i, med);

			//resTime[i] = media; // mi son salvato il tempo medio!!!!
			//resPack[i] = var;

			mediumTimes[i] = med;

			//printf("%.2f\t%.2f\n", med, var);

			media += med;
			//varianza += var;
		}
	}

	FILE *fp;
	fp = fopen("result.csv", "w");

	media = media / countTotal;

	for(int i = 0; i < sim_number; i++) {
		varianza += pow(mediumTimes[i] - media, 2);
	}

	varianza = varianza / sim_number;

	fprintf(fp, "%lf,%lf\n", media, varianza);

	printf("Risultato finale: Media: %.2f \tVarianza: %.2f\n", media, varianza);

	fclose(fp);

	printf("\n");

	exit(EXIT_SUCCESS);
}