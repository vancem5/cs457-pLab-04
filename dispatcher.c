/*-------------------------------------------------------------------------------

FILE:   dispatcher.c

Written By:
     1- Dr. Mohamed Aboutabl
     2- Jack Delano
     3- Mike Vance
Submitted on: 30OCT26
-------------------------------------------------------------------------------*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "wrappers.h"

#define READ_END 0
#define WRITE_END 1
#define STDIN 0
#define STDOUT 1
//--------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    pid_t amalPID, basimPID, kdcPID;
    int AtoKDC[2], KDCtoA[2], AtoB[2], BtoA[2];
    char arg1[20], arg2[20], arg3[20], arg4[20];

    Pipe(AtoKDC); // Amal to KDC Pipe
    Pipe(KDCtoA); // KDC to Amal Pipe
    Pipe(AtoB);   // Amal to Basim Pipe
    Pipe(BtoA);   // Basim to Amal Pipe

    printf("\nDispatcher started and created these pipes\n");
    printf("1) Amal-to-KDC   protocol pipe: read=%d  write=%d\n", AtoKDC[READ_END], AtoKDC[WRITE_END]);
    printf("2) KDC-to-Amal   protocol pipe: read=%d  write=%d\n", KDCtoA[READ_END], KDCtoA[WRITE_END]);
    printf("3) Amal-to-Basim protocol pipe: read=%d  write=%d\n", AtoB[READ_END], AtoB[WRITE_END]);
    printf("4) Basim-to-Amal protocol pipe: read=%d  write=%d\n", BtoA[READ_END], BtoA[WRITE_END]);

    // Create child processes:
    amalPID = Fork();
    if (amalPID == 0)
    {
        // This is the Amal process.
        // Amal will not use these ends of the pipes, decrement their 'Ref Count'
        close(AtoKDC[READ_END]);
        close(KDCtoA[WRITE_END]);
        close(AtoB[READ_END]);
        close(BtoA[WRITE_END]);

        // Prepare the file descriptors as args to Amal
        snprintf(arg1, 20, "%d", KDCtoA[READ_END]);
        snprintf(arg2, 20, "%d", AtoKDC[WRITE_END]);
        snprintf(arg3, 20, "%d", BtoA[READ_END]);
        snprintf(arg4, 20, "%d", AtoB[WRITE_END]);

        // Now, Start Amal
        char *cmnd = "./amal/amal";
        execlp(cmnd, "Amal", arg1, arg2, arg3, arg4, NULL);

        // the above execlp() only returns if an error occurs
        perror("ERROR starting Amal");
        exit(-1);
    }
    else
    {
        // This is still the Dispatcher process
        kdcPID = Fork();
        if (kdcPID == 0)
        {
            // This is the KDC process.
            // KDC only uses two pipes
            close(AtoKDC[WRITE_END]);
            close(KDCtoA[READ_END]);
            close(AtoB[READ_END]);
            close(AtoB[WRITE_END]);
            close(BtoA[READ_END]);
            close(BtoA[WRITE_END]);

            // Prepare the file descriptors as args to KDC
            snprintf(arg1, 20, "%d", AtoKDC[READ_END]);
            snprintf(arg2, 20, "%d", KDCtoA[WRITE_END]);

            // Now, Start KDC
            char *cmnd = "./kdc/kdc";
            execlp(cmnd, "KDC", arg1, arg2, NULL);

            // the above execlp() only returns if an error occurs
            perror("ERROR starting KDC");
            exit(-1);
        }
        else
        {
            // This is still the Dispatcher process
            basimPID = Fork();
            if (basimPID == 0)
            {
                // This is the Basim process
                // Basim will not use these ends of the pipes, decrement their 'count'
                close(AtoKDC[WRITE_END]);
                close(AtoKDC[READ_END]);
                close(KDCtoA[WRITE_END]);
                close(KDCtoA[READ_END]);
                close(AtoB[WRITE_END]);
                close(BtoA[READ_END]);

                // Prepare the file descriptors as args to Basim
                snprintf(arg1, 20, "%d", AtoB[READ_END]);
                snprintf(arg2, 20, "%d", BtoA[WRITE_END]);

                char *cmnd = "./basim/basim";
                execlp(cmnd, "Basim", arg1, arg2, NULL);

                // the above execlp() only returns if an error occurs
                perror("ERROR starting Basim");
                exit(-1);
            }
            else
            { // This is still the parent Dispatcher  process
                // close all ends of the pipes so that their 'count' is decremented
                close(AtoKDC[WRITE_END]);
                close(AtoKDC[READ_END]);
                close(KDCtoA[WRITE_END]);
                close(KDCtoA[READ_END]);
                close(AtoB[WRITE_END]);
                close(AtoB[READ_END]);
                close(BtoA[WRITE_END]);
                close(BtoA[READ_END]);

                printf("\nDispatcher is now waiting for Amal to terminate\n");
                int exitStatus;
                waitpid(amalPID, &exitStatus, 0);

                printf("\nDispatcher is now waiting for Basim to terminate\n");
                waitpid(basimPID, &exitStatus, 0);

                printf("\nDispatcher is now waiting for the KDC to terminate\n");
                waitpid(kdcPID, &exitStatus, 0);

                printf("\nThe Dispatcher process has terminated\n\n");
            }
        }
    }
}
