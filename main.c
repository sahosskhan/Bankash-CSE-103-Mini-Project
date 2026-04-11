#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define DATA_FILE "BankData.txt"
#define MAX_ACCOUNTS 100
#define START_ACC_NO 1001

// --- Data Structures ---
typedef struct {
    int accNumber;
    char name[50];
    char pin[5];
    int type; // 1 for Admin, 2 for User
    double balance;
} Account;

Account accounts[MAX_ACCOUNTS];
int accountCount = 0;
int loggedInIndex = -1;

// --- Function Prototypes ---
void loadData();
void saveData();
void mainMenu();
void createAccount();
void login();
void userDashboard();
void adminDashboard();
void deposit();
void withdraw();
void checkDetails();
void sendMoney();
void viewAllAccounts();
void manageAccounts();
int findAccountByNumber(int accNo);
int isNumeric(char str[]);
void clearBuffer();

// --- Main Execution ---
int main() {
    loadData();
    mainMenu();
    return 0;
}

// --- Utility Functions ---
void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int isNumeric(char str[]) {
    for (int i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

int findAccountByNumber(int accNo) {
    for (int i = 0; i < accountCount; i++) {
        if (accounts[i].accNumber == accNo) return i;
    }
    return -1;
}

// --- File Handling (Structured for names with spaces) ---
void loadData() {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) return;

    accountCount = 0;
    while (accountCount < MAX_ACCOUNTS &&
           fscanf(file, "%d\n", &accounts[accountCount].accNumber) != EOF) {
        fgets(accounts[accountCount].name, 50, file);
        accounts[accountCount].name[strcspn(accounts[accountCount].name, "\n")] = 0; // Remove newline
        fscanf(file, "%s\n%d\n%lf\n", accounts[accountCount].pin,
               &accounts[accountCount].type, &accounts[accountCount].balance);
        accountCount++;
    }
    fclose(file);
}

void saveData() {
    FILE *file = fopen(DATA_FILE, "w");
    if (!file) return;
    for (int i = 0; i < accountCount; i++) {
        fprintf(file, "%d\n%s\n%s\n%d\n%.2f\n", accounts[i].accNumber,
                accounts[i].name, accounts[i].pin, accounts[i].type, accounts[i].balance);
    }
    fclose(file);
}

// --- Main Menu ---
void mainMenu() {
    int choice;
    while (1) {
        printf("\n====================================================================\n");
        printf("                         BANKASH SYSTEM\n");
        printf("====================================================================\n");
        printf("        Secure | Simple | Fast Console-Based Banking Solution\n");
        printf("====================================================================\n");
        printf("1. Create a New Account\n2. Login to Your Account\n3. Exit Application\n");
        printf("--------------------------------------------------------------------\n");
        printf("Please enter your choice (1-3): ");

        if (scanf("%d", &choice) != 1) {
            clearBuffer();
            continue;
        }

        switch (choice) {
            case 1: createAccount(); break;
            case 2: login(); break;
            case 3: saveData(); printf("Exiting system. Goodbye.\n"); exit(0);
            default: printf("Invalid choice! Try again.\n");
        }
    }
}

void createAccount() {
    if (accountCount >= MAX_ACCOUNTS) {
        printf("System capacity reached!\n");
        return;
    }

    Account newAcc;

    newAcc.accNumber = (accountCount == 0) ? START_ACC_NO : accounts[accountCount - 1].accNumber + 1;

    printf("\nEnter Account Holder Name: ");
    clearBuffer();
    scanf("%[^\n]", newAcc.name);

    printf("Create 4-digit PIN: ");
    scanf("%s", newAcc.pin);

    if (strlen(newAcc.pin) != 4 || !isNumeric(newAcc.pin)) {
        printf("Error: PIN must be exactly 4 numeric digits.\n");
        return;
    }

    printf("Select Account Type (1: Admin, 2: User): ");
    scanf("%d", &newAcc.type);
    newAcc.balance = 0.0;

    accounts[accountCount++] = newAcc;
    saveData();

    printf("\nAccount created successfully!\n");
    printf("----------------------------------\n");
    printf("YOUR ACCOUNT NUMBER: %d\n", newAcc.accNumber);
    printf("----------------------------------\n");
    printf("Please remember this number for login.\n");
}

// --- Simplified Login ---
void login() {
    int accNo;
    char pin[5];

    printf("\n--- LOGIN ---\n");
    printf("Account Number: ");
    scanf("%d", &accNo);
    printf("PIN: ");
    scanf("%s", pin);

    int idx = findAccountByNumber(accNo);

    // Check if account exists and PIN matches
    if (idx != -1 && strcmp(accounts[idx].pin, pin) == 0) {
        loggedInIndex = idx;
        // Automatically route based on stored account type
        if (accounts[idx].type == 1) {
            adminDashboard();
        } else {
            userDashboard();
        }
    } else {
        printf("Login Failed: Invalid account number or PIN.\n");
    }
}

// --- User Section ---
void userDashboard() {
    int choice;
    while (1) {
        printf("\n====================================================================\n");
        printf("                        USER DASHBOARD\n");
        printf("       Welcome: %s | Acc No: %d\n", accounts[loggedInIndex].name, accounts[loggedInIndex].accNumber);
        printf("====================================================================\n");
        printf("1. Deposit Funds\n2. Withdraw Funds\n3. Check Account Details & Balance\n4. Send Money\n5. Logout\n");
        printf("--------------------------------------------------------------------\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: deposit(); break;
            case 2: withdraw(); break;
            case 3: checkDetails(); break;
            case 4: sendMoney(); break;
            case 5: loggedInIndex = -1; return;
            default: printf("Invalid choice.\n");
        }
    }
}

void deposit() {
    double amt;
    printf("Enter amount to deposit: ");
    scanf("%lf", &amt);
    if (amt > 0) {
        accounts[loggedInIndex].balance += amt;
        saveData();
        printf("Success! Updated Balance: %.2f\n", accounts[loggedInIndex].balance);
    } else printf("Invalid amount.\n");
}

void withdraw() {
    double amt;
    printf("Enter amount to withdraw: ");
    scanf("%lf", &amt);
    if (amt > 0 && amt <= accounts[loggedInIndex].balance) {
        accounts[loggedInIndex].balance -= amt;
        saveData();
        printf("Success! Updated Balance: %.2f\n", accounts[loggedInIndex].balance);
    } else printf("Insufficient balance or invalid amount.\n");
}

void checkDetails() {
    printf("\n--- ACCOUNT OVERVIEW ---\n");
    printf("Account No : %d\n", accounts[loggedInIndex].accNumber);
    printf("Holder Name: %s\n", accounts[loggedInIndex].name);
    printf("Balance    : %.2f\n", accounts[loggedInIndex].balance);
}

void sendMoney() {
    int targetAcc;
    double amt;

    printf("Enter Recipient Account Number: ");
    scanf("%d", &targetAcc);
    int receiverIdx = findAccountByNumber(targetAcc);
    if (receiverIdx == -1) {
        printf("Transfer Failed: Recipient account %d does not exist.\n", targetAcc);
        return;
    }

    if (receiverIdx == loggedInIndex) {
        printf("Transfer Failed: You cannot send money to yourself.\n");
        return;
    }

    printf("Enter amount to transfer: ");
    scanf("%lf", &amt);
    if (amt <= 0) {
        printf("Transfer Failed: Amount must be positive.\n");
    }
    else if (amt > accounts[loggedInIndex].balance) {
        printf("Transfer Failed: Insufficient balance (Current: %.2f).\n", accounts[loggedInIndex].balance);
    }
    else {

        accounts[loggedInIndex].balance -= amt;
        accounts[receiverIdx].balance += amt;

        saveData();

        printf("Success! Sent %.2f to %s (Acc: %d).\n",
               amt, accounts[receiverIdx].name, accounts[receiverIdx].accNumber);
        printf("Your New Balance: %.2f\n", accounts[loggedInIndex].balance);
    }
}

void adminDashboard() {
    int choice;
    while (1) {
        printf("\n====================================================================\n");
        printf("                        ADMIN DASHBOARD\n");
        printf("====================================================================\n");
        printf("1. View All Accounts\n2. Manage Accounts\n3. Save Bank Data\n4. Load Bank Data\n5. Logout\n");
        printf("--------------------------------------------------------------------\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: viewAllAccounts(); break;
            case 2: manageAccounts(); break;
            case 3: saveData(); printf("Data saved successfully.\n"); break;
            case 4: loadData(); printf("Data reloaded from file.\n"); break;
            case 5: loggedInIndex = -1; return;
        }
    }
}

void viewAllAccounts() {
    printf("\n%-8s %-20s %-10s %-10s\n", "ACC NO", "NAME", "TYPE", "BALANCE");
    printf("--------------------------------------------------------------------\n");
    for (int i = 0; i < accountCount; i++) {
        printf("%-8d %-20s %-10s %.2f\n", accounts[i].accNumber, accounts[i].name,
               (accounts[i].type == 1) ? "Admin" : "User", accounts[i].balance);
    }
}

void manageAccounts() {
    int sub;
    printf("\n1. Search Account\n2. Delete Account\n3. Back\nChoice: ");
    scanf("%d", &sub);

    if (sub == 3) return;

    int target;
    printf("Enter Account Number: ");
    scanf("%d", &target);
    int idx = findAccountByNumber(target);

    if (idx == -1) {
        printf("Account not found.\n");
        return;
    }

    if (sub == 1) {
        printf("Result: %s | Balance: %.2f | Type: %s\n",
               accounts[idx].name, accounts[idx].balance,
               (accounts[idx].type == 1) ? "Admin" : "User");
    } else if (sub == 2) {
        printf("Confirm deletion of account %d? (y/n): ", target);
        char confirm;
        scanf(" %c", &confirm);
        if (confirm == 'y' || confirm == 'Y') {
            for (int i = idx; i < accountCount - 1; i++) accounts[i] = accounts[i+1];
            accountCount--;
            saveData();
            printf("Account deleted successfully.\n");
        }
    }
}
