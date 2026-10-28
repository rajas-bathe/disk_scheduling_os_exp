/* Disk Scheduling Simulator: FCFS, SSTF, IFCFS and SPTF
 * Sarthak Agiwale (01) and Rajas Bathe (03), SYCM3
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic disk_scheduling_updated.c -lm -o disk_scheduler
 * A finite queue is loaded before scheduling. Each request reads one sector. Times are simulated milliseconds.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#define MAX_REQ 50
#define MAX_TRACK 1000000

static int read_int(const char *prompt, int low, int high, int *value) {
    char line[128], *end;
    long n;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(line, sizeof line, stdin)) return 0;
        int complete = 0;
        for (int i = 0; line[i]; ++i) if (line[i] == '\n') complete = 1;
        if (!complete && !feof(stdin)) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            puts("Input too long. Enter one integer per line."); continue;
        }
        errno = 0;
        n = strtol(line, &end, 10);
        if (end != line && errno != ERANGE) {
            while (isspace((unsigned char)*end)) ++end;
            if (!*end && n >= low && n <= high) { *value = (int)n; return 1; }
        }
        printf("Invalid input. Enter an integer from %d to %d.\n", low, high);
    }
}
#define EPS 1e-9
static const char *names[] = {"FCFS", "SSTF", "IFCFS", "SPTF"};
typedef struct { int track, sector; } Request;
typedef struct {
    int head, sectors;
    double rpm, seek_per_track, phase;
} Disk;
typedef struct {
    long long movement;
    double seek, rotation, transfer, elapsed;
} Result;
static int distance(int a, int b) { return a > b ? a-b : b-a; }
static double wrap(double x, double period) {
    double r = fmod(x, period);
    if (r < 0) r += period;
    if (r < EPS || period-r < EPS) return 0;
    return r;
}
static int read_double(const char *prompt, double low, double high,
                       double *value) {
    char line[128], *end;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(line, sizeof line, stdin)) return 0;
        if (!strchr(line, '\n') && !feof(stdin)) {
            int c; while ((c=getchar()) != '\n' && c != EOF) {}
            puts("Input too long."); continue;
        }
        errno=0;
        double v=strtod(line, &end);
        if (end != line && errno != ERANGE && isfinite(v)) {
            while (isspace((unsigned char)*end)) ++end;
            if (!*end && v>=low && v<=high) { *value=v; return 1; }
        }
        printf("Enter a finite number from %.3f to %.3f.\n", low, high);
    }
}
/* Phase is the sector boundary currently passing under the head.
 * It increases with time. All tracks have aligned, equally spaced sectors.
 * The disk keeps rotating during seek and during a one-sector transfer.
 */
static void cost(Request r, int current, double phase, Disk d,
                 double *seek, double *rotation) {
    double revolution = 60000.0/d.rpm;
    double sector_ms = revolution/d.sectors;
    *seek = distance(current,r.track)*d.seek_per_track;
    double arrival = wrap(phase + *seek/sector_ms, d.sectors);
    *rotation = wrap(r.sector-arrival, d.sectors)*sector_ms;
}
static Result run(const Request q[], int n, Disk d, int algorithm,
                  int verbose) {
    int done[MAX_REQ]={0}, current=d.head, target=-1;
    double phase=d.phase, sector_ms=60000.0/d.rpm/d.sectors;
    Result result={0,0,0,0,0};
    if (verbose) {
        printf("\n%s: initial head=%d, initial phase=%.3f\n",
               names[algorithm],d.head,d.phase);
        puts("Step ID  Track Sector Move Seek_ms Wait_ms Xfer_ms End_ms Phase");
    }
    for (int step=0; step<n; ++step) {
        int chosen=-1;
        double best=0;
        if (algorithm==0) chosen=step;
        else {
            if (algorithm==2 && (target<0 || done[target]))
                for (int i=0;i<n;++i) if (!done[i]) { target=i; break; }
            for (int i=0;i<n;++i) {
                if (done[i]) continue;
                if (algorithm==2) {
                    int low=current<q[target].track?current:q[target].track;
                    int high=current>q[target].track?current:q[target].track;
                    if (q[i].track<low || q[i].track>high) continue;
                }
                double seek,rotation;
                cost(q[i],current,phase,d,&seek,&rotation);
                double score=algorithm==3?seek+rotation:
                             (double)distance(current,q[i].track);
                /* Equal scores: lower track, then original request ID. */
                if (chosen<0 || score<best-EPS ||
                    (fabs(score-best)<=EPS && q[i].track<q[chosen].track)) {
                    chosen=i; best=score;
                }
            }
        }
        double seek,rotation;
        cost(q[chosen],current,phase,d,&seek,&rotation);
        int move=distance(current,q[chosen].track);
        result.movement+=move;
        result.seek+=seek; result.rotation+=rotation;
        result.transfer+=sector_ms;
        result.elapsed+=seek+rotation+sector_ms;
        phase=wrap(phase+(seek+rotation+sector_ms)/sector_ms,d.sectors);
        if (verbose)
            printf("%-4d %-3d %-5d %-6d %-4d %-7.3f %-7.3f %-7.3f %-6.3f %.3f\n",
                   step+1,chosen+1,q[chosen].track,q[chosen].sector,move,
                   seek,rotation,sector_ms,result.elapsed,phase);
        current=q[chosen].track; done[chosen]=1;
    }
    if (verbose)
        printf("Total=%lld tracks; service=%.3f ms; average service=%.3f ms\n",
               result.movement,result.elapsed,result.elapsed/n);
    return result;
}
static void show(const Request q[],int n,Disk d) {
    printf("\nHead=%d | RPM=%.0f | sectors/track=%d\n",d.head,d.rpm,d.sectors);
    printf("Seek=%.3f ms/track | initial phase=%.3f\n",d.seek_per_track,d.phase);
    puts("Request ID: (track, sector)");
    for (int i=0;i<n;++i) printf("%d: (%d, %d)\n",i+1,q[i].track,q[i].sector);
}
static void compare(const Request q[],int n,Disk d) {
    puts("\nAlgorithm Tracks Seek_ms Rotation_ms Transfer_ms Total_ms Avg_ms");
    for (int a=0;a<4;++a) {
        /* run receives Disk by value: every algorithm starts identically. */
        Result r=run(q,n,d,a,0);
        printf("%-9s %-6lld %-7.3f %-11.3f %-11.3f %-8.3f %.3f\n",
               names[a],r.movement,r.seek,r.rotation,r.transfer,r.elapsed,
               r.elapsed/n);
    }
    puts("Total = seek + rotation + transfer. Avg = service time per request.");
    puts("Educational HDD model: no cache, arrivals, overlap or settling cost.");
}
int main(void) {
    Request q[MAX_REQ]; Disk d={50,10,6000,1,0};
    int n=0,loaded=0,choice;
    for (;;) {
        puts("\nDISK SCHEDULING: FCFS / SSTF / IFCFS / SPTF");
        puts("1 Custom input   2 Load SPTF demo   3 Load original queue");
        puts("4 FCFS trace     5 SSTF trace       6 IFCFS trace");
        puts("7 SPTF trace     8 Compare all      9 Show input   0 Exit");
        if (!read_int("Choice: ",0,9,&choice) || choice==0) break;
        if (choice==1) {
            loaded=0;
            if (!read_int("Initial head (0..1000000): ",0,MAX_TRACK,&d.head) ||
                !read_int("Sectors per track (1..360): ",1,360,&d.sectors) ||
                !read_double("RPM (60..60000): ",60,60000,&d.rpm) ||
                !read_double("Seek ms/track (0..100): ",0,100,&d.seek_per_track) ||
                !read_double("Initial phase (0..sectors; sectors means 0): ",
                             0,d.sectors,&d.phase) ||
                !read_int("Request count (1..50): ",1,MAX_REQ,&n)) break;
            d.phase=wrap(d.phase,d.sectors);
            for (int i=0;i<n;++i) {
                printf("Request %d\n",i+1);
                if (!read_int(" Track: ",0,MAX_TRACK,&q[i].track) ||
                    !read_int(" Sector: ",0,d.sectors-1,&q[i].sector)) return 0;
            }
            loaded=1; show(q,n,d);
        } else if (choice==2) {
            d=(Disk){50,10,6000,1,0}; n=2;
            q[0]=(Request){52,9}; q[1]=(Request){54,5};
            loaded=1; show(q,n,d);
        } else if (choice==3) {
            const int tracks[]={98,183,37,122,14,124,65,67};
            const int sectors[]={0,3,7,2,4,6,5,8};
            d=(Disk){53,10,6000,0.1,0}; n=8;
            for(int i=0;i<n;++i) q[i]=(Request){tracks[i],sectors[i]};
            loaded=1; show(q,n,d);
            puts("Sector values are synthetic additions to the original queue.");
        } else if (!loaded) puts("Load custom input or a demo first.");
        else if (choice==8) compare(q,n,d);
        else if (choice==9) show(q,n,d);
        else run(q,n,d,choice-4,1);
    }
    return 0;
}
