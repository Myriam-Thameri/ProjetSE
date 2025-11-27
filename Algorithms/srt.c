#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
typedef struct node {
    PROCESS *p;
    struct node *next;
} node_t;

typedef struct SRT_Scheduler {
    node_t *head;
    PROCESS *current;
} SRT_Scheduler;

static void insert_sorted(node_t **head, PROCESS *p) {
    node_t *n = malloc(sizeof(node_t));
    n->p = p;
    n->next = NULL;

    if (*head == NULL || p->execution_time < (*head)->p->execution_time) {
        n->next = *head;
        *head = n;
        return;
    }

    node_t *cur = *head;
    while (cur->next && cur->next->p->execution_time <= p->execution_time)
        cur = cur->next;

    n->next = cur->next;
    cur->next = n;
}

SRT_Scheduler* SRT_create() {
    SRT_Scheduler *s = malloc(sizeof(*s));
    s->head = NULL;
    s->current = NULL;
    return s;
}

void SRT_add_process(SRT_Scheduler *s, PROCESS *p) {
    insert_sorted(&s->head, p);
    if(s->current && s->head && s->head->p->execution_time < s->current->execution_time){
        insert_sorted(&s->head, s->current);
        s->current = NULL;
    }
}

PROCESS* SRT_next(SRT_Scheduler *s) {
    if(s->current) return s->current;
    if(!s->head) return NULL;
    node_t *n = s->head;
    s->head = n->next;
    s->current = n->p;
    free(n);
    return s->current;
}

void SRT_destroy(SRT_Scheduler *s) {
    node_t *cur = s->head;
    while(cur){
        node_t *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    free(s);
}

void SRT_Algo(Config* config) {
    printf("=== Running SRT Algorithm ===\n");

    int tick = 0;
    int remaining = config->process_count;

    // Tableau pour suivre les temps restants
    int remaining_time[config->process_count];
    for (int i = 0; i < config->process_count; i++)
        remaining_time[i] = config->processes[i].execution_time;

    while (remaining > 0) {
        int shortest = -1;

        // Chercher le processus arrivé avec le plus petit temps restant
        for (int i = 0; i < config->process_count; i++) {
            if (config->processes[i].arrival_time <= tick && remaining_time[i] > 0) {
                if (shortest == -1 || remaining_time[i] < remaining_time[shortest])
                    shortest = i;
            }
        }

        if (shortest != -1) {
            PROCESS *current = &config->processes[shortest];
            printf("Tick %d: Running %s (Remaining %d)\n",
                   tick, current->ID, remaining_time[shortest]);
            remaining_time[shortest]--;

            if (remaining_time[shortest] == 0)
                remaining--;
        } else {
            // Aucun processus disponible, CPU idle
            printf("Tick %d: CPU idle\n", tick);
        }

        tick++;
    }
}

