#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

// Sarantinos Sarantis

#define MAX_BUFFER_SIZE 1024

void check_and_execute(char *command);

void removeSpaces(char *str) {
    int len = strlen(str);
    int count = 0;

    for (int i = 0; i < len; i++) {
        if (!isspace(str[i])) {
            str[count++] = str[i];
        }
    }
    
    str[count] = '\0';  // Null-terminate the modified string
}

void execute_single_command(char *command) {

    // Περίπτωση εντολής "quit"
    if(strcmp(command, "quit") == 0){
        exit(0); // Τερματίζει το shell
    }

    // Περίπτωση εντολής "chdir"
    if (strstr(command, "chdir") != NULL) {
        // Η strstr βρίσκει την θέση ενος substring σε ένα string
        // αν δεν το βρει επιστρέφει NULL

        // H strtok επιστρέφει το επόμενο string μετά από κενό " ", από εκει που
        // έχει μείνει η ανάγνωση του input
        // με σκοπό να ληφθεί το όρισμα της εντολής (νέο directory)
        char *arg = strtok(command, " ");
        arg = strtok(NULL, " ");
        
        if (arg != NULL) {
            if (chdir(arg) == 0) {
                printf("Changed directory to: %s\n", arg);
            } else {
                perror("chdir");
            }
        } else {
            printf("Usage: chdir <directory>\n");
        }
        return;
    }

    // Διαχωρισμός εντολών με βάση τον χαρακτήρα ';'
    char *token = strtok(command, ";");
    while (token != NULL) {

        // Δημιουργία πίνακα για πιθανά ορίσματα
        char* args[MAX_BUFFER_SIZE];
        int arg_count = 0;
        
        // Διαχωρισμός ορισμάτων
        char* tempArg = strtok(token, " ");
        
        // Η εντολή μπαίνει στο args[0] και ακολουθούν τα ορίσματα
        while (tempArg != NULL) {
            args[arg_count] = tempArg;
            arg_count++;
            tempArg = strtok(NULL, " ");
        }
        
        args[arg_count] = NULL; // Κάνει NULL την θέση μετά το τελευταίο στοιχείο

        // Δημιουργία νέας διεργασίας παιδί
        pid_t pid = fork();
        
        if (pid == 0) {         // Παιδί
            // Κάνουμε εκτέλεση της εντολής στο παιδί μέσω της συνάρτησης execvp
            // Η execvp εκτελεί μια εντολή και δέχεται έναν πίνακα με τα ορίσματα της εντολής.
            execvp(args[0], args); 
            // Αν πετύχει η execvp τότε το παιδί τερματίζεται εδώ και δεν εκτελείται
            // ο παρακάτω κώδικας
            // Σε περίπτωση που αποτύχει η execvp εκτυπώνει ένα μήνυμα σφάλματος  μέσω της
            // παρακάτω perror και έπιτα το παιδι αυτοκτωνεί με exit(1)
            perror("execvp");
            exit(1);
        } else if (pid > 0){    // Γονέας
            // Περιμένουμε την ολοκλήρωση του παιδί
            wait(NULL);
        } else {                // (pid < 0) Το fork απέτυχε
            perror("fork");
            exit(1);
        }
        
        // περνάει στο token την επόμενη διαχωρισμένη με ';' εντολή
        token = strtok(NULL, ";");
    }
}

void execute_pipe_command(char *command) {
    // Δημιουργία πίπας για επικοινωνία μεταξύ παιδιών
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        // πίπα απέτυχε
        perror("pipe");
        return;
    }

    // Διαχωρισμός εντολής σε πρώτο και δεύτερο μέρος πίπας
    char *command1 = strtok(command, "|");
    char *command2 = strtok(NULL, "|");

    // Δημιουργία δυο παιδιών (Δ child1 --> Δ child2)
    pid_t child1, child2;
    child1 = fork();

    if (child1 < 0) {
        perror("fork");
        return;
    }

    if (child1 == 0) {  // Child process 1

        // Close the read end of the pipe
        // Το παιδί 1 γράφει δεδομένα δεν διαβάζει
        close(pipe_fd[0]);
        
        // Redirect stdout to the write end of the pipe // stdout --> pipe_fd[1]
        // Στέλνει τα δεδομένα που θα πήγαιναν στο stdout στο pipe_fd[1]
        // STDOUT_FILENO είναι αριθμός που αναπαριστά το standard output και αντιστοιχεί στο 1
        dup2(pipe_fd[1], STDOUT_FILENO);    
        // Εφόσον η ροή των δεδομένων πλέον έχει ως child1 --> stdout --> pipe_fd[1]
        // Δεν χρειάζεται η επικοινωνία παιδιού 1 με την πίπα, και την κλείνουμε
        close(pipe_fd[1]);
        
        // Execute the first command before the pipe
        check_and_execute(command1);
        // Κανουμε πάλι check για την περιπτωση να υπάρχουν Ανακατευθυντήριοι χαρακτήρες

        exit(0); // τερματισμός παιδιού 1
    } else {
        // Parent process

        child2 = fork();

        if (child2 < 0) {
            perror("fork");
            return;
        }

        if (child2 == 0) {  // Child process 2
            // Close the write end of the pipe
            // Το παιδί 2 διαβάζει δεν γράφει δεδομένα
            close(pipe_fd[1]);
            
            // Redirect stdin to the read end of the pipe
            dup2(pipe_fd[0], STDIN_FILENO);
            // child1 --> stdout(c1) --> pipe_fd[1] --> pipe_fd[0] --> stdin(c2) --> child2
            close(pipe_fd[0]);
            
            // Execute the second command after the pipe
            check_and_execute(command2);
            
            //printf("--Child 2 dead\n");
            exit(0);
        } else {
            // Parent process

            // Close both ends of the pipe
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            
            // Wait for both child processes to finish
            wait(NULL);
            wait(NULL);
        }
    }
}

void execute_redirect_command(char *command) {

    pid_t pid = fork();
        
    if (pid == 0) {         // Παιδί
        // Ανακατεύθυνση εισόδου
        if (strstr(command, "<") != NULL){
            // Διαχωρισμός εντολής και αρχείου
            char *finalCommand = strtok(command, "<");
            char *fileName = strtok(NULL, "<");
            removeSpaces(fileName);

            // Ανακατεύθυνση εισόδου από το αρχείο input.txt
            if (freopen(fileName, "r", stdin) == NULL){
                // Απέτυχε
                perror("freopen");
                return;
            }
            // Εκτέλεση σε περίπτωση που πέτυχε
            execute_single_command(finalCommand);
        }
        // Ανακατεύθυνση εξώδου με εγγραφή στο τέλος
        else if(strstr(command, ">>") != NULL) {

            char *finalCommand = strtok(command, ">>");
            char *fileName = strtok(NULL, ">>");
            removeSpaces(fileName);

            if (freopen(fileName, "a", stdout) == NULL){
                perror("freopen");
                return;
            }
            execute_single_command(finalCommand);
        }
        // Ανακατεύθυνση εξώδου με εγγραφή και αντικατάσταση
        else {

            char *finalCommand = strtok(command, ">");
            char *fileName = strtok(NULL, ">");
            removeSpaces(fileName);

            if (freopen(fileName, "w", stdout) == NULL){
                perror("freopen");
                return;
            }
            execute_single_command(finalCommand);
        }
        exit(0);    // Σκοτώνω το παιδί
    } else if (pid > 0){    // Γονέας
        wait(NULL);
    } else {                // Το fork απέτυχε
        perror("fork");
        exit(1);
    }
}

void check_and_execute(char *command) {

    // Έλεγχος για ύπαρξη συμβόλου πίπας '|'
    if (strstr(command, "|") != NULL) {
        execute_pipe_command(command);
    } 
    // Έλεγχος για ύπαρξη ανακατευδυντήριου συμβόλου
    else if (strstr(command, ">") != NULL || strstr(command, "<") != NULL){
        execute_redirect_command(command);
    }
    else {  // Normal boring command
        execute_single_command(command);
    }
}

int main() {
    
    char *student_id = "csd4511";
    char input[MAX_BUFFER_SIZE];    // Είσοδος χρήστη

    // Καθορισμός μεταβλητών για τον χρήστη που είναι συνδεδεμένος στο σύστημα 
    // και τον τρέχον κατάλογο εργασίας
    char *user = getlogin();
    char current_dir[MAX_BUFFER_SIZE];
    getcwd(current_dir, sizeof(current_dir));

    while (1) {
        // Εμφάνιση του κελύφους της μορφής <ΑΜ>-hy345sh@<Χρήστης>:<Κατάλογος>
        printf("%s-hy345sh@%s:%s$ ", student_id, user, current_dir);

        // Διάβασμα της εισόδου του χρήστη
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Αφαίρεση χαρακτήρα νέας γραμμής από είσοδο
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Εναρξη διαδικασίας εκτέλεσης δοσμένης εντολής από χρήστη
        check_and_execute(input);
    }

    return 0;
}
