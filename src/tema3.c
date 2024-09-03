#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define Peer 1
#define seeder 2
#define terminated 3

typedef struct __attribute__((packed)){
    int nr_seg ;
    char segmente[MAX_CHUNKS][HASH_SIZE+1];
    int contine_fisier;
    int rank;
    int status;
} client;
  

typedef struct __attribute__((packed)){
    char filename[MAX_FILENAME];
    client *clients;
    int nr_clients;
} file;

 
void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;

    char fisier[MAX_FILENAME];
    int numar_fisiere_detinute;
    int numar_fisiere_dorite;
    sprintf(fisier, "in%d.txt", rank);
    FILE *f = fopen(fisier, "r");
    if (f == NULL) {
        printf("Eroare la deschiderea fisierului \n");
        exit(-1);
    }
    fscanf(f, "%d", &numar_fisiere_detinute);
    MPI_Send(&numar_fisiere_detinute, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
    for(int i = 0; i < numar_fisiere_detinute; i++) {
        char nume_fisier[MAX_FILENAME];
        int segmente = 0;
        char segment[HASH_SIZE];
        fscanf(f, "%s", nume_fisier);
        int size_nume_fisier = strlen(nume_fisier);
        MPI_Send(&size_nume_fisier, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        MPI_Send(nume_fisier, strlen(nume_fisier), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        fscanf(f, "%d", &segmente);
        MPI_Send(&segmente, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        for(int j = 0; j < segmente; j++) {
            fscanf(f, "%s", segment);
            segment[HASH_SIZE] = '\0';
            MPI_Send(segment, HASH_SIZE ,MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        }  
    }
    fscanf(f, "%d", &numar_fisiere_dorite);
    char nume_fisier[numar_fisiere_dorite][MAX_FILENAME];
    for(int i = 0; i < numar_fisiere_dorite; i++) {
        fscanf(f, "%s", nume_fisier[i]);
    }
    fclose(f);

    int ok;
    MPI_Recv(&ok, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int fisier_current = 0;
    int index_segmente = 0;
    int total_seg_fisier = 0;
    client prorpiu;
    prorpiu.contine_fisier = 1;
    prorpiu.rank = rank;
    while (ok == 1 && numar_fisiere_dorite > 0 && fisier_current < numar_fisiere_dorite) {
        int nr_clients;
        MPI_Status status;
        int tip_mesaj = 0;
        MPI_Send(&tip_mesaj, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        int size_nume_fisier = strlen(nume_fisier[fisier_current]);
        MPI_Send(&size_nume_fisier, 1, MPI_INT, TRACKER_RANK, 1, MPI_COMM_WORLD);
        MPI_Send(nume_fisier[fisier_current], strlen(nume_fisier[fisier_current]), MPI_CHAR, TRACKER_RANK, 1, MPI_COMM_WORLD);
        MPI_Recv(&nr_clients, 1, MPI_INT, TRACKER_RANK, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        client clienti[nr_clients];
        for(int i = 0; i < nr_clients; i++) {
            MPI_Recv(&clienti[i], sizeof(client), MPI_BYTE, TRACKER_RANK, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(total_seg_fisier < clienti[i].nr_seg) {
                total_seg_fisier = clienti[i].nr_seg;
            }   
        }
        int index_client = rand() % nr_clients;
        for(int i = 0; i < 10; i++) {
            int ack = 0;
            while(clienti[index_client%nr_clients].nr_seg-1 < index_segmente) {
                index_client++;
            }
            MPI_Send(&ack, 1, MPI_INT, clienti[index_client%nr_clients].rank,10, MPI_COMM_WORLD);
            MPI_Recv(&ack, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            if(ack == 0) {
                exit(-1);
            }
            strcpy(prorpiu.segmente[index_segmente], clienti[index_client%nr_clients].segmente[index_segmente]);
            index_client++;
            index_segmente++;
            if(index_segmente == total_seg_fisier) {
                tip_mesaj = 3;
                break;
            } 
        }
        prorpiu.nr_seg = index_segmente;
        MPI_Send(&tip_mesaj, 1, MPI_INT, TRACKER_RANK, 2, MPI_COMM_WORLD);
        MPI_Send(&size_nume_fisier, 1, MPI_INT, TRACKER_RANK, 3, MPI_COMM_WORLD);
        MPI_Send(nume_fisier[fisier_current], size_nume_fisier, MPI_CHAR, TRACKER_RANK, 3, MPI_COMM_WORLD);
        MPI_Send(&prorpiu, sizeof(client), MPI_BYTE, TRACKER_RANK,3, MPI_COMM_WORLD);

        MPI_Recv(&ok, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(ok == 0) {
            exit(-1);
        }
        if(index_segmente == total_seg_fisier) {
            char filename[50];
            sprintf(filename, "client%d_%s", rank,nume_fisier[fisier_current]);
            FILE *f = fopen(filename, "w+");
            if (f == NULL) {
                printf("Error opening file %s\n", filename);
                exit(-1);
            }
            for(int i = 0; i < prorpiu.nr_seg-1; i++) {
                fprintf(f, "%s\n", prorpiu.segmente[i]);
               
            }
            fprintf(f, "%s", prorpiu.segmente[prorpiu.nr_seg-1]);
            fclose(f);
            fisier_current++;
            index_segmente = 0;
            prorpiu.nr_seg = 0;
            total_seg_fisier = 0;
        }
   }
   int final = 1;
   MPI_Send(&final, 1, MPI_INT, TRACKER_RANK, 4, MPI_COMM_WORLD);
   return NULL;
}

void *upload_thread_func(void *arg)
{
    while (1) {
        MPI_Status status;
        int ack;
        MPI_Recv(&ack, 1, MPI_INT, MPI_ANY_SOURCE,10, MPI_COMM_WORLD, &status);
        if(ack == 10) {
            break;
        }
        ack = 1;
        MPI_Send(&ack, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
    }
    return NULL;
}



void tracker(int numtasks, int rank) {
    MPI_Status status;
    int numar_fisiere_peers = 0;
    int numar_fisiere = 0;
    file *files = (file *) malloc(MAX_FILES * sizeof(file));
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].clients = (client *) malloc(numtasks * sizeof(client));
        files[i].nr_clients = 0;
        for(int j = 0; j < numtasks; j++) {
            files[i].clients[j].nr_seg = 0;
            files[i].clients[j].contine_fisier = 0;
        }
    }

    for (int i = 1; i < numtasks; i++) {
        MPI_Recv(&numar_fisiere_peers, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        for (int j = 0; j < numar_fisiere_peers; j++) {
            char nume_fisier[MAX_FILENAME];
            int segmente;
            int size_nume_fisier;
            MPI_Recv(&size_nume_fisier, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(nume_fisier, size_nume_fisier, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
            nume_fisier[size_nume_fisier] = '\0'; 
            int index = -1;
            for(int l = 0; l < numar_fisiere; l++) {
                if(strcmp(files[l].filename, nume_fisier) == 0) {
                    index = l;
                    break;
                }
            }
            if(index == -1) {
                strcpy(files[numar_fisiere].filename, nume_fisier);
                index = numar_fisiere;
                numar_fisiere++;
            }
            MPI_Recv(&segmente, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            files[index].clients[i].nr_seg = segmente;
            files[index].clients[i].contine_fisier = 1;
            files[index].clients[i].status = seeder;
            files[index].nr_clients++;

            for (int k = 0; k < segmente; k++) {
                char segment[HASH_SIZE];
                MPI_Recv(segment, HASH_SIZE, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
                segment[HASH_SIZE] = '\0';
                strncpy(files[index].clients[i].segmente[k], segment,HASH_SIZE);
                files[index].clients[i].segmente[k][HASH_SIZE] = '\0';
            }  
        }
    }
    for(int i = 1; i < numtasks; i++) {
        int ok = 1;
        MPI_Send(&ok, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    
    int shutdown = 0;
    int nr_clienti_terminati = 0;
    while(shutdown == 0) {
        int tip_mesaj;
        MPI_Recv(&tip_mesaj, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if(status.MPI_TAG == 0) {  
            int size_nume_fisier;
            char nume_fisier[MAX_FILENAME];
            MPI_Recv(&size_nume_fisier, 1, MPI_INT,status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(nume_fisier, size_nume_fisier, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
            nume_fisier[size_nume_fisier] = '\0';
            for(int j = 0; j < numar_fisiere; j++) {
                if(strcmp(files[j].filename, nume_fisier) == 0) {
                    int nr = files[j].nr_clients;
                    if (files[j].clients[status.MPI_SOURCE].contine_fisier == 1) {
                        nr--;
                    }
                    MPI_Send(&nr, 1, MPI_INT,status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                    for(int k = 1; k < numtasks; k++) {
                        if(files[j].clients[k].contine_fisier == 1 && k != status.MPI_SOURCE) {
                            files[j].clients[k].rank = k;
                            MPI_Send(&files[j].clients[k], sizeof(client), MPI_BYTE,status.MPI_SOURCE, 1, MPI_COMM_WORLD); 
                        }
                    }
                    break;
                }
            }
        } else if(status.MPI_TAG == 2) {
            client client;
            int size_nume_fisier=0;
            char nume_fisier[MAX_FILENAME];

            MPI_Recv(&size_nume_fisier, 1, MPI_INT, status.MPI_SOURCE, 3, MPI_COMM_WORLD, &status);
            MPI_Recv(nume_fisier, size_nume_fisier, MPI_CHAR, status.MPI_SOURCE, 3, MPI_COMM_WORLD, &status);
            MPI_Recv(&client, sizeof(client), MPI_BYTE, status.MPI_SOURCE, 3, MPI_COMM_WORLD, &status);
            nume_fisier[size_nume_fisier] = '\0';
            
            for (int i = 0; i < numar_fisiere; i++) {
                if(strcmp(files[i].filename, nume_fisier) == 0) {
                    if(client.nr_seg == 10) {
                        files[i].nr_clients++;
                        files[i].clients[status.MPI_SOURCE].status = Peer;
                    } else if(tip_mesaj == 3) {
                        files[i].clients[status.MPI_SOURCE].status = seeder;
                    }
                    memcpy(&files[i].clients[client.rank], &client, sizeof(client));
                    break;
                }
            }
            int ok = 1;
            MPI_Send(&ok, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        } else if(status.MPI_TAG == 4) {
            nr_clienti_terminati++;
            for(int i = 0; i < numar_fisiere; i++) {
                if(files[i].clients[status.MPI_SOURCE].contine_fisier == 1 ) {
                    files[i].clients[status.MPI_SOURCE].status = terminated;
                }
            }
            if (nr_clienti_terminati == numtasks - 1) {
                int ack = 10;
                for(int i = 1; i < numtasks; i++) {
                    MPI_Send(&ack, 1, MPI_INT, i, 10, MPI_COMM_WORLD);
                }
                shutdown = 1;
            }
        }
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;
    


    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
