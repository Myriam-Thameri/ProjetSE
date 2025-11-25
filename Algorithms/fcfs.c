#include <stdio.h>
#include <string.h>
#include "config.h"

/* --------- Limites internes --------- */
#define MAX_QUEUE    128
#define MAX_TIMELINE 1024

/* --------- File circulaire simple --------- */
typedef struct {
    int items[MAX_QUEUE];
    int front, rear, size;
} FCFS_Queue;

typedef struct {
    char label[16];
    int start;
    int end;
} GanttBlock;

/* PCB spécifique à cette simu (pointeur vers PROCESS de Config) */
typedef struct {
    PROCESS *proc;
    int remaining_time;
    int next_io_index;
    int executed_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int last_cpu_exit;
    int started;
    int enqueued;
    int io_end_time;
    int in_io;
} FCFS_PCB;

/* ---------- Queue ---------- */
static void init_queue(FCFS_Queue *q)
{
    q->front = q->rear = q->size = 0;
}

static int is_empty(FCFS_Queue *q)
{
    return q->size == 0;
}

static void enqueue(FCFS_Queue *q, int val)
{
    if (q->size >= MAX_QUEUE)
        return;
    q->items[q->rear] = val;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->size++;
}

static int dequeue(FCFS_Queue *q)
{
    if (is_empty(q))
        return -1;
    int val = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->size--;
    return val;
}

/* ---------- Ajouter les processus arrivés jusqu'à 'time' ---------- */
static void enqueue_arrivals(Config *cfg, FCFS_PCB pcbs[], FCFS_Queue *cpuQ, int time)
{
    for (int i = 0; i < cfg->process_count; i++) {
        if (!pcbs[i].enqueued && pcbs[i].proc->arrival_time <= time) {
            enqueue(cpuQ, i);
            pcbs[i].enqueued = 1;
            printf("Arrivée → %s (t=%d)\n", pcbs[i].proc->ID, time);
        }
    }
}

/* ---------- Simulation FCFS avec I/O bloquants ---------- */
void simulate_fcfs_with_io(Config *cfg)
{
    FCFS_PCB pcbs[cfg->process_count];
    FCFS_Queue cpuQ;
    GanttBlock timeline[MAX_TIMELINE];

    int time = 0, finished = 0, gantt_idx = 0;

    /* Initialisation des PCB */
    for (int i = 0; i < cfg->process_count; i++) {
        pcbs[i].proc           = &cfg->processes[i];
        pcbs[i].remaining_time = cfg->processes[i].execution_time;
        pcbs[i].next_io_index  = 0;
        pcbs[i].executed_time  = 0;
        pcbs[i].completion_time = -1;
        pcbs[i].waiting_time    = 0;
        pcbs[i].turnaround_time = 0;
        pcbs[i].last_cpu_exit   = -1;
        pcbs[i].started         = 0;
        pcbs[i].enqueued        = 0;
        pcbs[i].io_end_time     = -1;
        pcbs[i].in_io           = 0;
    }

    init_queue(&cpuQ);

    printf("=== FCFS avec I/O bloquants ===\n\n");

    enqueue_arrivals(cfg, pcbs, &cpuQ, time);

    while (finished < cfg->process_count) {

        /* 1. Fin des opérations I/O à ce temps */
        for (int i = 0; i < cfg->process_count; i++) {
            if (pcbs[i].in_io && pcbs[i].io_end_time <= time) {
                pcbs[i].in_io = 0;
                enqueue(&cpuQ, i);
                printf("Fin I/O → %s (t=%d)\n", pcbs[i].proc->ID, time);
            }
        }

        /* 2. Y a-t-il un processus prêt ? */
        if (!is_empty(&cpuQ)) {
            int idx = dequeue(&cpuQ);
            FCFS_PCB *p = &pcbs[idx];
            PROCESS *pr = p->proc;

            /* Calcul du temps d'attente */
            if (!p->started) {
                p->waiting_time = time - pr->arrival_time;
                p->started = 1;
            } else {
                p->waiting_time += time - p->last_cpu_exit;
            }
            p->last_cpu_exit = time;

            int start_t = time;

            /* Combien de temps CPU avant prochain I/O ou fin ? */
            int time_until_next_io = 999999;
            if (pr->io_count > 0 && p->next_io_index < pr->io_count) {
                int io_at = pr->io_operations[p->next_io_index].start_time;
                time_until_next_io = io_at - p->executed_time;
                if (time_until_next_io < 0)
                    time_until_next_io = 0;
            }

            int exec_this_turn = p->remaining_time;
            if (time_until_next_io > 0 && time_until_next_io < exec_this_turn) {
                exec_this_turn = time_until_next_io;
            }

            if (exec_this_turn <= 0 && p->remaining_time > 0) {
                exec_this_turn = 1;
            }

            /* Exécution effective */
            time += exec_this_turn;
            p->executed_time += exec_this_turn;
            p->remaining_time -= exec_this_turn;

            /* Ajouter au Gantt */
            if (gantt_idx < MAX_TIMELINE) {
                strncpy(timeline[gantt_idx].label, pr->ID, 15);
                timeline[gantt_idx].label[15] = '\0';
                timeline[gantt_idx].start = start_t;
                timeline[gantt_idx].end   = time;
                gantt_idx++;
            }

            /* Arrivées pendant l'exécution */
            enqueue_arrivals(cfg, pcbs, &cpuQ, time);

            /* Vérifier si un I/O a été déclenché (franchissement du seuil) */
            int io_triggered = 0;
            while (pr->io_count > 0 &&
                   p->next_io_index < pr->io_count &&
                   p->executed_time >= pr->io_operations[p->next_io_index].start_time) {

                io_triggered = 1;
                int duration = pr->io_operations[p->next_io_index].duration;
                p->in_io = 1;
                p->io_end_time = time + duration;
                p->next_io_index++;
                p->last_cpu_exit = time;

                printf("Blocage I/O → %s (t=%d, dur=%d)\n", pr->ID, time, duration);
                break;
            }

            /* Processus terminé ? */
            if (p->remaining_time <= 0) {
                p->completion_time = time;
                p->turnaround_time = time - pr->arrival_time;
                finished++;
                printf("Termine → %s (t=%d, attente=%d, retour=%d)\n",
                       pr->ID, time, p->waiting_time, p->turnaround_time);
            }
            /* Sinon, on le remet en file si pas en I/O */
            else if (!io_triggered) {
                enqueue(&cpuQ, idx);
            }

        } else {
            /* CPU idle : sauter au prochain événement */
            int next_event = -1;

            /* Prochaine arrivée */
            for (int i = 0; i < cfg->process_count; i++) {
                if (!pcbs[i].enqueued && pcbs[i].proc->arrival_time > time) {
                    if (next_event == -1 ||
                        pcbs[i].proc->arrival_time < next_event)
                        next_event = pcbs[i].proc->arrival_time;
                }
            }

            /* Prochaine fin d'I/O */
            for (int i = 0; i < cfg->process_count; i++) {
                if (pcbs[i].in_io && pcbs[i].io_end_time > time) {
                    if (next_event == -1 ||
                        pcbs[i].io_end_time < next_event)
                        next_event = pcbs[i].io_end_time;
                }
            }

            if (next_event == -1)
                break;

            if (time < next_event && gantt_idx < MAX_TIMELINE) {
                strcpy(timeline[gantt_idx].label, "IDLE");
                timeline[gantt_idx].start = time;
                timeline[gantt_idx].end   = next_event;
                gantt_idx++;
            }

            printf("CPU IDLE %d → %d\n", time, next_event);
            time = next_event;
            enqueue_arrivals(cfg, pcbs, &cpuQ, time);
        }
    }

    /* === Affichage Gantt === */
    printf("\nGantt CPU :\n");
    if (gantt_idx == 0) {
        printf("(aucune activité)\n");
    } else {
        printf("┌");
        for (int i = 0; i < gantt_idx; i++) {
            int w = timeline[i].end - timeline[i].start;
            for (int j = 0; j < w; j++) printf("─");
            printf(i < gantt_idx - 1 ? "┬" : "┐");
        }
        printf("\n│");
        for (int i = 0; i < gantt_idx; i++) {
            int w = timeline[i].end - timeline[i].start;
            int len = (int)strlen(timeline[i].label);
            int pad = (w - len) / 2;
            for (int j = 0; j < pad; j++) printf(" ");
            printf("%s", timeline[i].label);
            for (int j = 0; j < w - len - pad; j++) printf(" ");
            printf("│");
        }
        printf("\n└");
        for (int i = 0; i < gantt_idx; i++) {
            int w = timeline[i].end - timeline[i].start;
            for (int j = 0; j < w; j++) printf("─");
            printf(i < gantt_idx - 1 ? "┴" : "┘");
        }
        printf("\n ");
        for (int i = 0; i < gantt_idx; i++) {
            int w = timeline[i].end - timeline[i].start;
            printf("%*d", w, timeline[i].end);
        }
        printf("\n");
    }

    /* === Résumé === */
    printf("\nRésumé des processus :\n");
    printf("ID   Arrivée  Fin  Attente  Retour  CPU\n");
    printf("-------------------------------------------\n");

    float avg_wait = 0, avg_turn = 0;
    for (int i = 0; i < cfg->process_count; i++) {
        FCFS_PCB *p = &pcbs[i];
        printf("%-4s %7d %5d %8d %7d %5d\n",
               p->proc->ID,
               p->proc->arrival_time,
               p->completion_time,
               p->waiting_time,
               p->turnaround_time,
               p->executed_time);
        avg_wait += p->waiting_time;
        avg_turn += p->turnaround_time;
    }
    printf("-------------------------------------------\n");
    printf("Moyenne attente      : %.2f\n", avg_wait / cfg->process_count);
    printf("Moyenne retournement : %.2f\n", avg_turn / cfg->process_count);
}
