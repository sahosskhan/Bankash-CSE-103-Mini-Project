#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>
#include <time.h>

#define DATA_FILE "GBBL_Accounts.txt"
#define TRANS_FILE "GBBL_Ledger.txt"
#define MAX_ACCOUNTS 100
#define MAX_REQUESTS 50
#define START_ACC_NO 1001
#define ADMIN_ACC 9999

// Color Palette
#define C_GOLD 6
#define C_GREEN 10
#define C_WHITE 15
#define C_CYAN 11
#define C_RED 12
#define C_GRAY 8

typedef struct {
    int accNumber;
    char name[50];
    char pin[6];
    double balance;
    double loanBalance;
    int isResetRequested;
    int isActiveStatus;
    char creationDate[30];
} Account;

typedef struct {
    int userIdx;
    double amount;
    int isActive;
} DepositRequest;

typedef struct {
    int userIdx;
    double amount;
    int isActive;
} LoanRequest;

typedef struct {
    int fromIdx;
    int toIdx;
    double amount;
    int isActive;
} MoneyRequest;

Account accounts[MAX_ACCOUNTS];
DepositRequest requests[MAX_REQUESTS];
LoanRequest loanReqs[MAX_REQUESTS];
MoneyRequest p2pRequests[MAX_REQUESTS];
int accountCount = 0;
int requestCount = 0;
int loanReqCount = 0;
int p2pCount = 0;
int loggedInIndex = -1;

// --- UI Engine ---
void setUI(int color, char* title) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    if(title) SetConsoleTitle(title);
}

void resetColor() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7); }

void clearBuffer() {
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

void animate(char* msg) {
    printf("\n\t");
    setUI(C_GRAY, NULL);
    printf("%s", msg);
    for(int i = 0; i < 3; i++) { Sleep(150); printf("."); }
    printf("\n");
    resetColor();
}

void header(char* pageName, int color) {
    system("cls");
    setUI(color, pageName);
    printf("\n\t****************************************************\n");
    printf("\t    GOLDEN BANGLA BANK LTD. | %s\n", pageName);
    printf("\t****************************************************\n");
    resetColor();
}

void footer(char* quote, int back) {
    setUI(C_GRAY, NULL);
    printf("\n\t----------------------------------------------------\n\t");
    printf("\"%s\"\n\t", quote);
    if (back) { setUI(C_WHITE, ""); printf("[0] Back | [99] Refresh"); }
    resetColor();
    printf("\n\t----------------------------------------------------\n");
}

// --- Data Logic ---
void logAction(int accNo, char* type, double amt, char* info) {
    FILE *f = fopen(TRANS_FILE, "a");
    if(f) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        int hour = tm.tm_hour;
        char ampm[3];
        strcpy(ampm, (hour >= 12) ? "PM" : "AM");
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

        fprintf(f, "[%02d-%02d-%d %02d:%02d %s] ACC:%d | %-8s | $%-10.2f | %s\n",
                tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, hour, tm.tm_min, ampm, accNo, type, amt, info);
        fclose(f);
    }
}

void saveData() {
    FILE *f = fopen(DATA_FILE, "w");
    if (!f) return;
    for (int i = 0; i < accountCount; i++) {
        fprintf(f, "%d\n%s\n%s\n%.2lf\n%.2lf\n%d\n%d\n%s\n",
                accounts[i].accNumber, accounts[i].name, accounts[i].pin,
                accounts[i].balance, accounts[i].loanBalance, accounts[i].isResetRequested,
                accounts[i].isActiveStatus, accounts[i].creationDate);
    }
    fclose(f);
}

void initAdmin() {
    if(accountCount == 0) {
        accounts[0].accNumber = ADMIN_ACC;
        strcpy(accounts[0].name, "GBBL Admin");
        strcpy(accounts[0].pin, "9876");
        accounts[0].balance = 1000000000.0;
        accounts[0].loanBalance = 0.0;
        accounts[0].isResetRequested = 0;
        accounts[0].isActiveStatus = 1;
        strcpy(accounts[0].creationDate, "System Genesis");
        accountCount = 1;
        saveData();
    }
}

void loadData() {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) { initAdmin(); return; }
    accountCount = 0;
    while (accountCount < MAX_ACCOUNTS && fscanf(f, "%d\n", &accounts[accountCount].accNumber) != EOF) {
        fgets(accounts[accountCount].name, 50, f);
        accounts[accountCount].name[strcspn(accounts[accountCount].name, "\n")] = 0;
        fscanf(f, "%s\n%lf\n%lf\n%d\n%d\n",
               accounts[accountCount].pin, &accounts[accountCount].balance,
               &accounts[accountCount].loanBalance, &accounts[accountCount].isResetRequested,
               &accounts[accountCount].isActiveStatus);
        fgets(accounts[accountCount].creationDate, 30, f);
        accounts[accountCount].creationDate[strcspn(accounts[accountCount].creationDate, "\n")] = 0;
        accountCount++;
    }
    fclose(f);
    if(accountCount == 0) initAdmin();
}

int findAcc(int num) {
    for (int i = 0; i < accountCount; i++) if (accounts[i].accNumber == num) return i;
    return -1;
}

void cleanupRequests() {
    int newCount = 0;
    for (int i = 0; i < requestCount; i++) {
        if (requests[i].isActive) requests[newCount++] = requests[i];
    }
    requestCount = newCount;

    int newLoanCount = 0;
    for (int i = 0; i < loanReqCount; i++) {
        if (loanReqs[i].isActive) loanReqs[newLoanCount++] = loanReqs[i];
    }
    loanReqCount = newLoanCount;

    int newP2P = 0;
    for (int i = 0; i < p2pCount; i++) {
        if (p2pRequests[i].isActive) p2pRequests[newP2P++] = p2pRequests[i];
    }
    p2pCount = newP2P;
}

// --- Peer Request Logic ---
void sendMoneyRequest() {
    header("REQUEST MONEY", C_CYAN);
    int tAcc; double amt;
    printf("\tEnter Target Account ID (0: Back | 99: Refresh): "); scanf("%d", &tAcc);
    if(tAcc == 99) { sendMoneyRequest(); return; }
    if(tAcc == 0) return;

    int tIdx = findAcc(tAcc);
    if(tIdx != -1 && tIdx != loggedInIndex && tIdx != 0) {
        printf("\tRequest Amount: $"); scanf("%lf", &amt);
        if(amt > 0) {
            if(p2pCount < MAX_REQUESTS) {
                p2pRequests[p2pCount].fromIdx = loggedInIndex;
                p2pRequests[p2pCount].toIdx = tIdx;
                p2pRequests[p2pCount].amount = amt;
                p2pRequests[p2pCount].isActive = 1;
                p2pCount++;
                animate("Sending Request Signal");
                printf("\tRequest sent to %s successfully!\n", accounts[tIdx].name);
                Sleep(1500);
            } else { printf("\tRequest queue full."); Sleep(1000); }
        }
    } else { printf("\tInvalid Account ID."); Sleep(1000); }
}

void manageIncomingRequests() {
    while(1) {
        header("MONEY REQUESTS BOX", C_CYAN);
        int found = 0;
        int map[MAX_REQUESTS];
        int displayId = 1;

        printf("\t%-3s | %-15s | %-10s\n", "ID", "REQUESTER", "AMOUNT");
        printf("\t------------------------------------------\n");

        for(int i=0; i<p2pCount; i++) {
            if(p2pRequests[i].isActive && p2pRequests[i].toIdx == loggedInIndex) {
                printf("\t[%d] | %-15s | $%.2f\n", displayId, accounts[p2pRequests[i].fromIdx].name, p2pRequests[i].amount);
                map[displayId] = i;
                displayId++;
                found = 1;
            }
        }

        if(!found) {
            printf("\tNo incoming requests.\n");
            footer("Peace of mind.", 1);
            int b; scanf("%d", &b);
            if(b == 99) continue;
            return;
        }

        int choice;
        printf("\n\tSelect ID to Action (0: Back | 99: Refresh): "); scanf("%d", &choice);
        if(choice == 99) continue;
        if(choice == 0) return;

        if(choice > 0 && choice < displayId) {
            int rIdx = map[choice];
            int requesterIdx = p2pRequests[rIdx].fromIdx;
            double amt = p2pRequests[rIdx].amount;

            printf("\t[1] Accept  [2] Reject: ");
            int action; scanf("%d", &action);

            if(action == 1) {
                if(accounts[loggedInIndex].balance >= amt) {
                    accounts[loggedInIndex].balance -= amt;
                    accounts[requesterIdx].balance += amt;
                    p2pRequests[rIdx].isActive = 0;
                    saveData();

                    char payInfo[100];
                    sprintf(payInfo, "Paid Req to %s", accounts[requesterIdx].name);
                    logAction(accounts[loggedInIndex].accNumber, "REQ_SENT", amt, payInfo);

                    char recvInfo[100];
                    sprintf(recvInfo, "Req Fulfilled by %s", accounts[loggedInIndex].name);
                    logAction(accounts[requesterIdx].accNumber, "REQ_RCVD", amt, recvInfo);

                    animate("Transferring Funds");
                } else {
                    setUI(C_RED, ""); printf("\tInsufficient Balance to fulfill this!\n");
                    resetColor(); Sleep(1500);
                }
            } else if(action == 2) {
                p2pRequests[rIdx].isActive = 0;
                logAction(accounts[loggedInIndex].accNumber, "REQ_RJCT", amt, "Declined Peer Request");
                animate("Rejecting");
            }
            cleanupRequests();
        }
    }
}

// --- Admin Sub-functions ---
void processPinResets() {
    header("PIN RESET REQUESTS", C_RED);
    int found = 0;
    printf("\t%-6s | %-18s | %-10s\n", "ACC", "NAME", "STATUS");
    printf("\t------------------------------------------\n");

    for(int i=0; i<accountCount; i++) {
        if(accounts[i].isResetRequested == 1) {
            printf("\t%-6d | %-18s | [PENDING]\n", accounts[i].accNumber, accounts[i].name);
            found = 1;
        }
    }

    if(!found) {
        printf("\tNo pending reset requests found.\n");
        footer("System Secure.", 1);
        int b; scanf("%d", &b);
        if(b == 99) { processPinResets(); return; }
        return;
    }

    int target;
    printf("\n\tApprove Reset to '1111' for Acc ID (0: Back | 99: Refresh): "); scanf("%d", &target);
    if(target == 99) { processPinResets(); return; }
    if(target == 0) return;

    int idx = findAcc(target);
    if(idx != -1 && accounts[idx].isResetRequested == 1) {
        strcpy(accounts[idx].pin, "1111");
        accounts[idx].isResetRequested = 0;
        saveData();
        logAction(target, "SECURITY", 0, "Admin Approved Reset to 1111");
        animate("Resetting Credentials");
        printf("\tPIN for %d has been set to 1111.\n", target);
        Sleep(1500);
    } else {
        printf("\tInvalid ID or No Request Found."); Sleep(1000);
    }
}

void userManagement() {
    while(1) {
        header("USER DATABASE", C_WHITE);
        printf("\t%-6s | %-15s | %-10s | %-10s | %-8s\n", "ACC", "NAME", "BAL", "LOAN", "STATUS");
        printf("\t--------------------------------------------------------------------------------\n");
        for(int i=0; i<accountCount; i++) {
            if(accounts[i].accNumber == ADMIN_ACC) setUI(C_GOLD, NULL);
            else if(accounts[i].isActiveStatus == 0) setUI(C_RED, NULL);

            printf("\t%-6d | %-15s | %-10.2f | %-10.2f | %-8s\n",
                accounts[i].accNumber, accounts[i].name, accounts[i].balance,
                accounts[i].loanBalance, accounts[i].isActiveStatus ? "ACTIVE" : "DISABLED");
            resetColor();
        }

        printf("\n\t[1] Delete Account  [2] Toggle Status  [99] Refresh  [0] Back\n\tAction > ");
        int act; scanf("%d", &act);
        if(act == 99) continue;
        if(act == 0) return;

        printf("\tEnter Target Account ID: ");
        int target; scanf("%d", &target);
        if(target == ADMIN_ACC) { setUI(C_RED, NULL); printf("\tError: Admin Protection.\n"); resetColor(); Sleep(1000); continue; }

        int idx = findAcc(target);
        if(idx != -1) {
            if(act == 1) {
                printf("\tConfirm Delete %s? (1:Yes / 0:No): ", accounts[idx].name);
                int confirm; scanf("%d", &confirm);
                if(confirm == 1) {
                    for(int i=idx; i<accountCount-1; i++) accounts[i] = accounts[i+1];
                    accountCount--; saveData(); animate("Purging Record");
                }
            } else if(act == 2) {
                accounts[idx].isActiveStatus = !accounts[idx].isActiveStatus;
                saveData();
                printf("\tAccount %d is now %s.\n", target, accounts[idx].isActiveStatus ? "ACTIVE" : "DEACTIVATED");
                Sleep(1000);
            }
        } else { printf("\tInvalid Account."); Sleep(800); }
    }
}

void approveRequests() {
    while(1) {
        cleanupRequests();
        header("APPROVAL CENTER", C_CYAN);
        if(requestCount == 0) {
            printf("\tNo pending deposit requests.\n");
            footer("System Balanced.", 1);
            int b; scanf("%d", &b);
            if(b == 99) continue;
            return;
        }
        for(int i=0; i<requestCount; i++) printf("\t[%d] %-15s: $%.2f\n", i+1, accounts[requests[i].userIdx].name, requests[i].amount);
        int id, dec;
        printf("\n\tRequest ID (0: Back | 99: Refresh): "); scanf("%d", &id);
        if(id == 99) continue;
        if(id == 0) return;
        if(id > 0 && id <= requestCount) {
            printf("\t[1] Accept [2] Reject: "); scanf("%d", &dec);
            int rIdx = id - 1;
            if(dec == 1) {
                if(accounts[0].balance >= requests[rIdx].amount) {
                    accounts[0].balance -= requests[rIdx].amount;
                    accounts[requests[rIdx].userIdx].balance += requests[rIdx].amount;
                    saveData();
                    logAction(accounts[requests[rIdx].userIdx].accNumber, "DEPOSIT", requests[rIdx].amount, "Approved");
                    animate("Crediting Account");
                } else { setUI(C_RED, ""); printf("\tVault Insufficient!\n"); resetColor(); Sleep(1000); }
            }
            requests[rIdx].isActive = 0;
        }
    }
}

void manageLoanRequests() {
    while(1) {
        cleanupRequests();
        header("LOAN REQUESTS CENTER", C_GOLD);
        if(loanReqCount == 0) {
            printf("\tNo pending loan requests.\n");
            footer("Stability maintained.", 1);
            int b; scanf("%d", &b);
            if(b == 99) continue;
            return;
        }

        for(int i=0; i<loanReqCount; i++)
            printf("\t[%d] %-15s: Requesting $%.2f\n", i+1, accounts[loanReqs[i].userIdx].name, loanReqs[i].amount);

        int id;
        printf("\n\tSelect Request ID (0: Back | 99: Refresh): "); scanf("%d", &id);
        if(id == 99) continue;
        if(id == 0) return;

        if(id > 0 && id <= loanReqCount) {
            int rIdx = id - 1;
            int uIdx = loanReqs[rIdx].userIdx;
            double amt = loanReqs[rIdx].amount;

            printf("\t[1] Accept  [2] Reject  [0] Back: ");
            int action; scanf("%d", &action);
            if(action == 0) continue;

            if(action == 1) {
                if(accounts[0].balance >= amt) {
                    accounts[0].balance -= amt;
                    accounts[uIdx].loanBalance += amt;
                    saveData();
                    logAction(accounts[uIdx].accNumber, "LOAN_IN", amt, "Loan Approved");
                    animate("Processing Loan Disbursement");
                    Sleep(1500);
                } else {
                    setUI(C_RED, ""); printf("\tVault Insufficient!\n"); resetColor(); Sleep(1500);
                }
            } else if(action == 2) {
                logAction(accounts[uIdx].accNumber, "LOAN_RJ", amt, "Loan Rejected");
                animate("Rejecting Request");
            }
            loanReqs[rIdx].isActive = 0;
        }
    }
}

// --- User Dashboards ---
void showHistory(int accNo, int isAdmin) {
    header("LEDGER HISTORY", C_CYAN);
    FILE *f = fopen(TRANS_FILE, "r");
    char line[256]; int found = 0;
    if(f) {
        while(fgets(line, sizeof(line), f)) {
            char accStr[20]; sprintf(accStr, "ACC:%d", accNo);
            if(isAdmin || strstr(line, accStr)) { printf("\t%s", line); found = 1; }
        }
        fclose(f);
    }
    if(!found) printf("\tNo transactions found.\n");
    footer("Transparency is our commitment.", 1);
    int b; scanf("%d", &b);
    if(b == 99) showHistory(accNo, isAdmin);
}

void loanMenu() {
    while(1) {
        header("LOAN SERVICES", C_CYAN);
        printf("\tYour Loan Balance: $%.2f\n\n", accounts[loggedInIndex].loanBalance);
        printf("\t[1] Take Loan  [2] Pay Loan  [R] Refresh  [0] Back\n\tSelection > ");
        char choice; scanf(" %c", &choice);
        choice = tolower(choice);

        if(choice == 'r') continue;
        if(choice == '0') return;
        if(choice == '1') {
            double amt; printf("\tEnter Loan Amount: $"); scanf("%lf", &amt);
            if(amt > 0) {
                loanReqs[loanReqCount].userIdx = loggedInIndex;
                loanReqs[loanReqCount].amount = amt;
                loanReqs[loanReqCount].isActive = 1;
                loanReqCount++;
                animate("Sending Loan Request");
                Sleep(1500);
            }
        } else if(choice == '2') {
            if(accounts[loggedInIndex].loanBalance <= 0) {
                printf("\tNo outstanding loans!\n"); Sleep(1000); continue;
            }
            double amt; printf("\tEnter Payment Amount: $"); scanf("%lf", &amt);
            if(amt > 0 && amt <= accounts[loggedInIndex].balance) {
                if(amt > accounts[loggedInIndex].loanBalance) amt = accounts[loggedInIndex].loanBalance;
                accounts[loggedInIndex].balance -= amt;
                accounts[loggedInIndex].loanBalance -= amt;
                accounts[0].balance += amt;
                saveData();
                animate("Processing Payment");
                Sleep(1500);
            }
        }
    }
}

void userDashboard() {
    char ch;
    while (1) {
        header("USER SERVICES", C_GREEN);
        setUI(C_CYAN, NULL);
        printf("\tWelcome, %s\n", accounts[loggedInIndex].name);
        resetColor();
        printf("\tAcc: %d | Bal: $%.2f | Loan: $%.2f\n\n",
               accounts[loggedInIndex].accNumber, accounts[loggedInIndex].balance,
               accounts[loggedInIndex].loanBalance);

        printf("\t[1] Deposit Request     [2] Withdraw Money\n");
        printf("\t[3] Peer Transfer       [4] Transaction History\n");
        printf("\t[5] Change PIN          [6] Change Name\n");
        printf("\t[7] Forget Password     [8] Request Money\n");
        printf("\t[9] Money Requests      [A] Loan Management\n");
        printf("\t[V] View Profile        [R] Refresh Page\n");
        footer("Your prosperity, our priority.", 0);
        printf("\t[L] Logout | Selection > "); scanf(" %c", &ch);
        ch = tolower(ch);

        if (ch == 'l') { loggedInIndex = -1; return; }
        if (ch == 'r') continue;

        if(accounts[loggedInIndex].isActiveStatus == 0) {
            if(strchr("12389a", ch)) {
                setUI(C_RED, "");
                printf("\n\tAccount deactivated. Contact Admin.\n");
                resetColor(); Sleep(2000);
                continue;
            }
        }

        if (ch == '1') {
            header("DEPOSIT REQUEST", C_CYAN);
            double amt; printf("\tEnter Amount (0: Back): $"); scanf("%lf", &amt);
            if(amt > 0) {
                requests[requestCount].userIdx = loggedInIndex;
                requests[requestCount].amount = amt;
                requests[requestCount].isActive = 1;
                requestCount++; animate("Securing Request");
            }
        } else if (ch == '2') {
            header("WITHDRAWAL", C_RED);
            double amt; printf("\tEnter Amount (0: Back): $"); scanf("%lf", &amt);
            if(amt > 0 && amt <= accounts[loggedInIndex].balance) {
                accounts[loggedInIndex].balance -= amt; saveData();
                logAction(accounts[loggedInIndex].accNumber, "W-DRAW", amt, "Cash Withdrawal");
                animate("Processing");
            }
        } else if (ch == '3') {
            header("P2P TRANSFER", C_CYAN);
            int tAcc; double amt; printf("\tRecipient Acc (0: Back): "); scanf("%d", &tAcc);
            if(tAcc == 0) continue;
            int tIdx = findAcc(tAcc);
            if(tIdx != -1 && tIdx != loggedInIndex && tIdx != 0) {
                printf("\tAmount: $"); scanf("%lf", &amt);
                if(amt > 0 && amt <= accounts[loggedInIndex].balance) {
                    accounts[loggedInIndex].balance -= amt; accounts[tIdx].balance += amt;
                    saveData(); animate("Transferring");
                }
            }
        } else if (ch == '4') showHistory(accounts[loggedInIndex].accNumber, 0);
        else if (ch == '5') {
            header("CHANGE PIN", C_CYAN);
            char o1[6], o2[6], nP[6];
            printf("\tOld PIN: "); scanf("%s", o1);
            printf("\tConfirm Old: "); scanf("%s", o2);
            if(strcmp(o1, accounts[loggedInIndex].pin) == 0 && strcmp(o1, o2) == 0) {
                printf("\tNew PIN: "); scanf("%s", nP);
                strcpy(accounts[loggedInIndex].pin, nP); saveData(); animate("Updating");
            }
        } else if (ch == '6') {
            header("CHANGE NAME", C_WHITE);
            char oN[50], nN[50]; printf("\tOld Name (0: Back): "); clearBuffer();
            scanf("%[^\n]s", oN); clearBuffer();
            if(strcmp(oN, "0") == 0) continue;
            if(strcmp(oN, accounts[loggedInIndex].name) == 0) {
                printf("\tNew Name: "); scanf("%[^\n]s", nN);
                strcpy(accounts[loggedInIndex].name, nN); saveData(); animate("Updating");
            }
        } else if (ch == '7') {
            header("FORGET PIN", C_RED);
            printf("\tRequest PIN reset? [1] Yes [0] Back: ");
            int c; scanf("%d", &c);
            if(c == 1) {
                accounts[loggedInIndex].isResetRequested = 1;
                saveData(); printf("\tRequest Sent.\n"); Sleep(1000);
            }
        } else if (ch == '8') sendMoneyRequest();
        else if (ch == '9') manageIncomingRequests();
        else if (ch == 'a') loanMenu();
        else if (ch == 'v') {
            header("MY PROFILE", C_CYAN);
            printf("\tName:            %s\n", accounts[loggedInIndex].name);
            printf("\tAccount No:      %d\n", accounts[loggedInIndex].accNumber);
            printf("\tCurrent Balance: $%.2f\n", accounts[loggedInIndex].balance);
            printf("\tAccount Status:  %s\n", accounts[loggedInIndex].isActiveStatus ? "Active" : "Deactivated");
            footer("Personal Banking Details", 1);
            int b; scanf("%d", &b);
            if(b == 99) { /* Will refresh on loop */ }
        }
    }
}

void adminDashboard() {
    char ch;
    while (1) {
        header("ADMIN COMMAND CENTER", C_GOLD);
        printf("\tVault Liquidity: $%.2f\n\n", accounts[0].balance);
        printf("\t[1] Deposit Requests   [2] User Database\n");
        printf("\t[3] Add Vault Cash     [4] Global Ledger\n");
        printf("\t[5] Account Stats      [6] Pin Reset Requests \n");
        printf("\t[7] Loan Requests      [R] Refresh System\n");
        footer("Integrity in every transaction.", 0);
        printf("\t[L] Logout | Command > "); scanf(" %c", &ch);
        ch = tolower(ch);
        if (ch == 'l') { loggedInIndex = -1; return; }
        if (ch == 'r') continue;
        if (ch == '1') approveRequests();
        else if (ch == '2') userManagement();
        else if (ch == '3') {
            header("VAULT INJECTION", C_GREEN); double amt;
            printf("\tAmount to Mint (0: Back): $"); scanf("%lf", &amt);
            if(amt > 0) { accounts[0].balance += amt; saveData(); animate("Injecting"); }
        } else if (ch == '4') showHistory(0, 1);
        else if (ch == '5') {
            header("MARKET STATS", C_GOLD); double total = 0;
            for(int i=0; i<accountCount; i++) total += accounts[i].balance;
            printf("\tTotal Circulation: $%.2f\n\tLive Accounts: %d\n", total, accountCount);
            footer("Economy Stats", 1); int b; scanf("%d", &b);
        } else if (ch == '6') processPinResets();
        else if (ch == '7') manageLoanRequests();
    }
}

// --- Main Auth ---
void login() {
    header("LOGIN PORTAL", C_WHITE);
    int acc; char p[6];
    printf("\tAccount ID (0: Back | 99: Refresh): "); scanf("%d", &acc);
    if(acc == 99) { login(); return; }
    if(acc == 0) return;
    printf("\tSecure PIN: "); scanf("%s", p);
    int idx = findAcc(acc);
    if(idx != -1 && strcmp(accounts[idx].pin, p) == 0) {
        loggedInIndex = idx;
        if(accounts[idx].accNumber == ADMIN_ACC) adminDashboard(); else userDashboard();
    } else { setUI(C_RED, ""); printf("\tAccess Denied."); resetColor(); Sleep(1500); }
}

void registerUser() {
    header("OPEN ACCOUNT", C_CYAN);
    if(accountCount >= MAX_ACCOUNTS) { printf("\tCapacity full."); Sleep(1000); return; }
    Account n;
    printf("\tFull Name (0: Back | 99: Refresh): "); clearBuffer();
    scanf("%[^\n]s", n.name);
    if(strcmp(n.name, "99") == 0) { registerUser(); return; }
    if(strcmp(n.name, "0") == 0) return;
    printf("\tSet 4-Digit PIN: "); scanf("%s", n.pin);

    n.accNumber = (accountCount == 0) ? START_ACC_NO : accounts[accountCount-1].accNumber + 1;
    if(n.accNumber == ADMIN_ACC) n.accNumber++;

    n.balance = 0.0; n.loanBalance = 0.0; n.isResetRequested = 0; n.isActiveStatus = 1;
    time_t t = time(NULL); struct tm tm = *localtime(&t);
    sprintf(n.creationDate, "%02d-%02d-%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

    accounts[accountCount++] = n;
    saveData(); animate("Issuing Account");
    printf("\tID: %d\n", n.accNumber);
    footer("Remember your credentials", 1); int b; scanf("%d", &b);
}

int main() {
    loadData();
    char ch;
    while (1) {
        header("WELCOME TO OUR SYSTEM", C_GOLD);
        printf("\tBuilt for the Nation, Driven by Integrity\n\n");
        printf("\t[1] Secure Login      [2] Open Account\n");
        printf("\t[3] System Exit       [R] Refresh UI\n");
        footer("Empowering your future.", 0);
        printf("\tSelection > "); scanf(" %c", &ch);
        ch = tolower(ch);
        if (ch == 'r') continue;
        if (ch == '1') login();
        else if (ch == '2') registerUser();
        else if (ch == '3') { animate("Shutting Down"); exit(0); }
    }
    return 0;
}
