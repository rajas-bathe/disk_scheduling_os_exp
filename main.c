#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REQ 50

/* ---------- Utility ---------- */
int absDiff(int a, int b) { return a > b ? a - b : b - a; }

void printLine(void) {
    printf("-----------------------------------------------\n");
}

/* ---------- Module 1: Request Queue Input ---------- */
int readRequests(int req[], int *head) {
    int n;
    printf("Enter initial head position: ");
    if (scanf("%d", head) != 1 || *head < 0) { printf("Invalid head position!\n"); return 0; }
    printf("Enter number of requests (1-%d): ", MAX_REQ);
    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_REQ) { printf("Invalid request count!\n"); return 0; }
    printf("Enter %d track requests: ", n);
    for (int i = 0; i < n; i++) {
        if (scanf("%d", &req[i]) != 1 || req[i] < 0) { printf("Invalid track!\n"); return 0; }
    }
    return n;
}

void displayQueue(int req[], int n, int head) {
    printf("\nInitial head : %d\nRequest queue: ", head);
    for (int i = 0; i < n; i++) printf("%d ", req[i]);
    printf("\n");
}

/* ---------- Module 2: FCFS Scheduler ---------- */
int fcfs(int req[], int n, int head, int order[]) {
    int total = 0, cur = head;
    printf("\n========== FCFS DISK SCHEDULING ==========\n");
    printf("%-5s %-12s %-14s %-9s %-10s\n", "Step", "Current head", "Next request", "Movement", "Cumulative");
    printLine();
    for (int i = 0; i < n; i++) {
        int move = absDiff(cur, req[i]);
        total += move;
        order[i] = req[i];
        printf("%-5d %-12d %-14d %-9d %-10d\n", i + 1, cur, req[i], move, total);
        cur = req[i];
    }
    printLine();
    printf("Total head movement = %d tracks\n", total);
    printf("Average movement    = %.2f tracks/request\n", (float)total / n);
    return total;
}

/* ---------- Module 3: SSTF Scheduler ---------- */
int sstf(int req[], int n, int head, int order[]) {
    int served[MAX_REQ] = {0};
    int total = 0, cur = head;
    printf("\n========== SSTF DISK SCHEDULING ==========\n");
    printf("%-5s %-12s %-14s %-9s %-10s\n", "Step", "Current head", "Next request", "Movement", "Cumulative");
    printLine();
    for (int step = 0; step < n; step++) {
        int best = -1, bestDist = 1 << 30;
        for (int i = 0; i < n; i++) {
            if (served[i]) continue;
            int d = absDiff(cur, req[i]);
            /* tie-breaker: lower track number */
            if (d < bestDist || (d == bestDist && req[i] < req[best])) { bestDist = d; best = i; }
        }
        served[best] = 1;
        total += bestDist;
        order[step] = req[best];
        printf("%-5d %-12d %-14d %-9d %-10d\n", step + 1, cur, req[best], bestDist, total);
        cur = req[best];
    }
    printLine();
    printf("Total head movement = %d tracks\n", total);
    printf("Average movement    = %.2f tracks/request\n", (float)total / n);
    return total;
}

/* ---------- Module 4/5: Metrics and Comparison ---------- */
void compare(int n, int fcfsTotal, int sstfTotal) {
    printf("\n========== COMPARISON ==========\n");
    printf("%-10s %-16s %-18s\n", "Algorithm", "Total movement", "Average/request");
    printLine();
    printf("%-10s %-16d %-18.2f\n", "FCFS", fcfsTotal, (float)fcfsTotal / n);
    printf("%-10s %-16d %-18.2f\n", "SSTF", sstfTotal, (float)sstfTotal / n);
    printLine();
    if (fcfsTotal > 0) {
        int saved = fcfsTotal - sstfTotal;
        printf("SSTF saves %d tracks (%.3f%% reduction vs FCFS)\n", saved, 100.0 * saved / fcfsTotal);
    } else printf("No head movement required.\n");
}

int main(void) {
    int req[MAX_REQ], fo[MAX_REQ], so[MAX_REQ], head, n, choice;
    int loaded = 0;
    do {
        printf("\n=================================================\n");
        printf("   DISK SCHEDULING SIMULATOR (FCFS & SSTF)\n");
        printf("=================================================\n");
        printf("1. Enter Head Position and Request Queue\n");
        printf("2. Run FCFS\n3. Run SSTF\n4. Compare FCFS and SSTF\n5. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: n = readRequests(req, &head); loaded = n > 0; if (loaded) displayQueue(req, n, head); break;
            case 2: if (!loaded) { printf("\nLoad requests first (option 1).\n"); break; } fcfs(req, n, head, fo); break;
            case 3: if (!loaded) { printf("\nLoad requests first (option 1).\n"); break; } sstf(req, n, head, so); break;
            case 4: if (!loaded) { printf("\nLoad requests first (option 1).\n"); break; }
                    { int f = fcfs(req, n, head, fo); int s = sstf(req, n, head, so); compare(n, f, s); } break;
            case 5: printf("\nExiting simulator. Thank you!\n"); break;
            default: printf("\nInvalid choice! Please try again.\n");
        }
    } while (choice != 5);
    return 0;
}

