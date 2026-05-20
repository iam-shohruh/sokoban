/*
 * Standard graph-search algorithms.
 * Problem-independent — do NOT modify except for the two marked sections:
 *   1. Insert_Priority_Queue_GENERALIZED_A_Star()
 *   2. The GeneralizedAStarSearch case in First_InsertFrontier_Search_TREE()
 *
 * VERBOSE (defined in GRAPH_SEARCH.h) controls debug output.
 * Set VERBOSE 0 for normal use; set to 1 to print frontier/hash-table at every step.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GRAPH_SEARCH.h"
#include "data_types.h"
#include "HashTable.h"

/* ======================================================================= */
Node *First_InsertFrontier_Search_TREE(const enum METHODS method,
                                       Node *const root,
                                       State *const goal_state,
                                       float alpha)
{
    int Number_Searched_Nodes  = 0;
    int Number_Generated_Nodes = 1;
    int Number_Allocated_Nodes = 1;
    enum ACTIONS action;
    Node *node, *child, *temp_node;
    Queue *frontier;
    Hash_Table *explorer_set;

    frontier = Start_Frontier(root);
#if VERBOSE
    Print_Frontier(frontier);
#endif

    explorer_set = New_Hash_Table(HASH_TABLE_BASED_SIZE);
#if VERBOSE
    Show_Hash_Table(explorer_set);
#endif

    while (Number_Searched_Nodes < MAX_SEARCHED_NODE)
    {
        if (Empty(frontier))
            return FAILURE;

        node = Pop(&frontier);

        /* GOAL-TEST */
        Number_Searched_Nodes++;
        if (Goal_Test(&(node->state), goal_state)) {
            printf("\nThe number of searched nodes  : %d\n", Number_Searched_Nodes);
            printf("The number of generated nodes : %d\n", Number_Generated_Nodes);
            printf("The number of nodes in memory : %d\n", Number_Allocated_Nodes);
            Delete_Hash_Table(explorer_set);
            return node;
        }

        ht_insert(explorer_set, &(node->state));
#if VERBOSE
        Show_Hash_Table(explorer_set);
#endif

        for (action = 0; (int)action < ACTIONS_NUMBER; action = (enum ACTIONS)((int)action + 1))
        {
            child = Child_Node(node, action);

            if (child != NULL) {
                Number_Generated_Nodes++;
                Number_Allocated_Nodes++;

                if (ht_search(explorer_set, &(child->state))) {
                    free(child);
                    Number_Allocated_Nodes--;
                    continue;
                }

                temp_node = Frontier_search(frontier, &(child->state));

                switch (method)
                {
                    case UniformCostSearch:
                        if (temp_node != NULL) {
                            if (child->path_cost < temp_node->path_cost)
                                Remove_Node_From_Frontier(temp_node, &frontier);
                            else { free(child); Number_Allocated_Nodes--; continue; }
                        }
                        Insert_Priority_Queue_UniformSearch(child, &frontier);
                        break;

                    case AStarSearch:
                        if (temp_node != NULL) {
                            if (child->path_cost + child->state.h_n <
                                temp_node->path_cost + temp_node->state.h_n)
                                Remove_Node_From_Frontier(temp_node, &frontier);
                            else { free(child); Number_Allocated_Nodes--; continue; }
                        }
                        child->state.h_n = Compute_Heuristic_Function(&(child->state), goal_state);
                        Insert_Priority_Queue_A_Star(child, &frontier);
                        break;

                    case GeneralizedAStarSearch:
                        /* f(n) = alpha * g(n) + (1 - alpha) * h(n)
                           alpha=1.0 -> UCS, alpha=0.0 -> Greedy, alpha=0.5 -> Weighted A* */
                        child->state.h_n = Compute_Heuristic_Function(&(child->state), goal_state);
                        if (temp_node != NULL) {
                            float f_child = alpha * child->path_cost
                                          + (1.0f - alpha) * child->state.h_n;
                            float f_old   = alpha * temp_node->path_cost
                                          + (1.0f - alpha) * temp_node->state.h_n;
                            if (f_child < f_old)
                                Remove_Node_From_Frontier(temp_node, &frontier);
                            else { free(child); Number_Allocated_Nodes--; continue; }
                        }
                        Insert_Priority_Queue_GENERALIZED_A_Star(child, &frontier, alpha);
                        break;

                    default:
                        printf("ERROR: Unknown method in First_InsertFrontier_Search_TREE.\n");
                        Delete_Hash_Table(explorer_set);
                        exit(-1);
                }
#if VERBOSE
                Print_Frontier(frontier);
#endif
            }
        }
    }

    printf("Maximum searched nodes exceeded (%d). Goal not found.\n", Number_Searched_Nodes);
    Delete_Hash_Table(explorer_set);
    return FAILURE;
}

/* ======================================================================= */
Node *First_GoalTest_Search_TREE(const enum METHODS method,
                                  Node *const root,
                                  State *const goal_state)
{
    int Number_Searched_Nodes  = 0;
    int Number_Generated_Nodes = 1;
    int Number_Allocated_Nodes = 1;
    enum ACTIONS action;
    Node *node, *child;
    Queue *frontier;
    Hash_Table *explorer_set;

    Number_Searched_Nodes++;
    if (Goal_Test(&(root->state), goal_state)) {
        printf("\nThe number of searched nodes  : %d\n", Number_Searched_Nodes);
        printf("The number of generated nodes : %d\n", Number_Generated_Nodes);
        printf("The number of nodes in memory : %d\n", Number_Allocated_Nodes);
        return root;
    }

    frontier = Start_Frontier(root);
#if VERBOSE
    Print_Frontier(frontier);
#endif

    explorer_set = New_Hash_Table(HASH_TABLE_BASED_SIZE);
#if VERBOSE
    Show_Hash_Table(explorer_set);
#endif

    while (Number_Searched_Nodes < MAX_SEARCHED_NODE)
    {
        if (Empty(frontier))
            return FAILURE;

        node = Pop(&frontier);

        ht_insert(explorer_set, &(node->state));
#if VERBOSE
        Show_Hash_Table(explorer_set);
#endif

        for (action = 0; (int)action < ACTIONS_NUMBER; action = (enum ACTIONS)((int)action + 1))
        {
            child = Child_Node(node, action);

            if (child != NULL) {
                Number_Generated_Nodes++;
                Number_Allocated_Nodes++;

                if (ht_search(explorer_set, &(child->state)) ||
                    Frontier_search(frontier, &(child->state)) != NULL) {
                    free(child);
                    Number_Allocated_Nodes--;
                    continue;
                }

                Number_Searched_Nodes++;
                if (Goal_Test(&(child->state), goal_state)) {
                    printf("\nThe number of searched nodes  : %d\n", Number_Searched_Nodes);
                    printf("The number of generated nodes : %d\n", Number_Generated_Nodes);
                    printf("The number of nodes in memory : %d\n", Number_Allocated_Nodes);
                    Delete_Hash_Table(explorer_set);
                    if (method == GreedySearch && !PREDETERMINED_GOAL_STATE)
                        child->state.h_n = Compute_Heuristic_Function(&(child->state), goal_state);
                    return child;
                }

                switch (method)
                {
                    case BreastFirstSearch:
                        Insert_FIFO(child, &frontier);
                        break;
                    case GreedySearch:
                        child->state.h_n = Compute_Heuristic_Function(&(child->state), goal_state);
                        Insert_Priority_Queue_GreedySearch(child, &frontier);
                        break;
                    default:
                        printf("ERROR: Unknown method in First_GoalTest_Search_TREE.\n");
                        exit(-1);
                }
#if VERBOSE
                Print_Frontier(frontier);
#endif
            }
        }
    }

    printf("Maximum searched nodes exceeded (%d). Goal not found.\n", Number_Searched_Nodes);
    Delete_Hash_Table(explorer_set);
    return FAILURE;
}

/* ======================================================================= */
Node *DepthType_Search_TREE(const enum METHODS method,
                             Node *const root,
                             State *const goal_state,
                             const int Max_Level)
{
    static int Number_Searched_Nodes  = 0;
    static int Number_Generated_Nodes = 1;
    static int Number_Allocated_Nodes = 1;
    enum ACTIONS action;
    Node *node, *child;
    Queue *frontier;
    Hash_Table *explorer_set;

    Number_Searched_Nodes++;
    if (Goal_Test(&(root->state), goal_state)) {
        printf("\nThe number of searched nodes  : %d\n", Number_Searched_Nodes);
        printf("The number of generated nodes : %d\n", Number_Generated_Nodes);
        printf("The number of nodes in memory : %d\n", Number_Allocated_Nodes);
        return root;
    }

    frontier = Start_Frontier(root);
#if VERBOSE
    Print_Frontier(frontier);
#endif

    explorer_set = New_Hash_Table(HASH_TABLE_BASED_SIZE);
#if VERBOSE
    Show_Hash_Table(explorer_set);
#endif

    while (Number_Searched_Nodes < MAX_SEARCHED_NODE)
    {
        if (Empty(frontier))
            return FAILURE;

        node = Pop(&frontier);

        ht_insert(explorer_set, &(node->state));
#if VERBOSE
        Show_Hash_Table(explorer_set);
#endif

        if (method == DepthLimitedSearch || method == IterativeDeepeningSearch)
            if (Level_of_Node(node) == Max_Level) {
                Clear_All_Branch(node, &Number_Allocated_Nodes);
                continue;
            }

        for (action = 0; (int)action < ACTIONS_NUMBER; action = (enum ACTIONS)((int)action + 1))
        {
            child = Child_Node(node, action);

            if (child != NULL) {
                Number_Generated_Nodes++;
                Number_Allocated_Nodes++;

                if (ht_search(explorer_set, &(child->state)) ||
                    Frontier_search(frontier, &(child->state)) != NULL) {
                    Clear_Single_Branch(child, &Number_Allocated_Nodes);
                } else {
                    Number_Searched_Nodes++;
                    if (Goal_Test(&(child->state), goal_state)) {
                        printf("\nThe number of searched nodes  : %d\n", Number_Searched_Nodes);
                        printf("The number of generated nodes : %d\n", Number_Generated_Nodes);
                        printf("The number of nodes in memory : %d\n", Number_Allocated_Nodes);
                        Delete_Hash_Table(explorer_set);
                        return child;
                    }
                    Insert_LIFO(child, &frontier);
#if VERBOSE
                    Print_Frontier(frontier);
#endif
                }

                if ((int)action == ACTIONS_NUMBER - 1 && node->Number_of_Child == 0)
                    Clear_All_Branch(node, &Number_Allocated_Nodes);
            }
        }
    }

    printf("%d nodes searched; goal not found.\n", Number_Searched_Nodes);
    Delete_Hash_Table(explorer_set);
    return FAILURE;
}

/* ======================================================================= */
Node *Child_Node(Node *const parent, const enum ACTIONS action)
{
    Node *child = NULL;
    Transition_Model trans_model;

    if (Result(&(parent->state), action, &trans_model)) {
        child = (Node *)malloc(sizeof(Node));
        if (child == NULL) Warning_Memory_Allocation();
        child->state            = trans_model.new_state;
        child->path_cost        = parent->path_cost + trans_model.step_cost;
        child->parent           = parent;
        child->action           = action;
        child->Number_of_Child  = 0;
        child->parent->Number_of_Child++;
    }
    return child;
}

/* ======================================================================= */
Queue *Start_Frontier(Node *const root)
{
    Queue *frontier = (Queue *)malloc(sizeof(Queue));
    if (frontier == NULL) Warning_Memory_Allocation();
    frontier->node = root;
    frontier->next = NULL;
    return frontier;
}

int Empty(const Queue *const frontier) { return (frontier == NULL); }

/* ======================================================================= */
Node *Pop(Queue **frontier)
{
    Node *node = NULL;
    Queue *temp;

    if (!Empty(*frontier)) {
        node = (*frontier)->node;
        temp = *frontier;
        *frontier = (*frontier)->next;
        free(temp);
    }
#if VERBOSE
    printf("\nPOP: "); Print_Node(node); printf("\n");
#endif
    return node;
}

/* ======================================================================= */
void Insert_FIFO(Node *const child, Queue **frontier)
{
    Queue *temp;
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child; nq->next = NULL;
    if (Empty(*frontier)) { *frontier = nq; return; }
    for (temp = *frontier; temp->next != NULL; temp = temp->next);
    temp->next = nq;
}

void Insert_LIFO(Node *const child, Queue **frontier)
{
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child; nq->next = *frontier; *frontier = nq;
}

/* ======================================================================= */
void Insert_Priority_Queue_UniformSearch(Node *const child, Queue **frontier)
{
    Queue *temp;
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child;

    if (Empty(*frontier)) { nq->next = NULL; *frontier = nq; return; }
    if (child->path_cost < (*frontier)->node->path_cost) {
        nq->next = *frontier; *frontier = nq; return;
    }
    for (temp = *frontier; temp->next != NULL; temp = temp->next)
        if (child->path_cost < temp->next->node->path_cost) {
            nq->next = temp->next; temp->next = nq; return;
        }
    temp->next = nq; nq->next = NULL;
}

/* ======================================================================= */
void Insert_Priority_Queue_GreedySearch(Node *const child, Queue **frontier)
{
    Queue *temp;
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child;

    if (Empty(*frontier)) { nq->next = NULL; *frontier = nq; return; }
    if (child->state.h_n < (*frontier)->node->state.h_n) {
        nq->next = *frontier; *frontier = nq; return;
    }
    for (temp = *frontier; temp->next != NULL; temp = temp->next)
        if (child->state.h_n < temp->next->node->state.h_n) {
            nq->next = temp->next; temp->next = nq; return;
        }
    temp->next = nq; nq->next = NULL;
}

/* ======================================================================= */
void Insert_Priority_Queue_A_Star(Node *const child, Queue **frontier)
{
    Queue *temp;
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child;

    float f_child = child->path_cost + child->state.h_n;
    if (Empty(*frontier)) { nq->next = NULL; *frontier = nq; return; }
    if (f_child < (*frontier)->node->path_cost + (*frontier)->node->state.h_n) {
        nq->next = *frontier; *frontier = nq; return;
    }
    for (temp = *frontier; temp->next != NULL; temp = temp->next)
        if (f_child < temp->next->node->path_cost + temp->next->node->state.h_n) {
            nq->next = temp->next; temp->next = nq; return;
        }
    temp->next = nq; nq->next = NULL;
}

/* ======================================================================= */
/* GENERALIZED A* SEARCH
   f(n) = alpha * g(n) + (1 - alpha) * h(n)
   alpha = 1.0  -->  Uniform-Cost Search
   alpha = 0.0  -->  Greedy Best-First Search
   alpha = 0.5  -->  Weighted A* (equal weight to cost and heuristic)       */
void Insert_Priority_Queue_GENERALIZED_A_Star(Node *const child, Queue **frontier, float alpha)
{
    Queue *temp;
    Queue *nq = (Queue *)malloc(sizeof(Queue));
    if (nq == NULL) Warning_Memory_Allocation();
    nq->node = child;

    float f_child = alpha * child->path_cost + (1.0f - alpha) * child->state.h_n;

    if (Empty(*frontier)) { nq->next = NULL; *frontier = nq; return; }

    float f_front = alpha * (*frontier)->node->path_cost
                  + (1.0f - alpha) * (*frontier)->node->state.h_n;
    if (f_child < f_front) {
        nq->next = *frontier; *frontier = nq; return;
    }
    for (temp = *frontier; temp->next != NULL; temp = temp->next) {
        float f_next = alpha * temp->next->node->path_cost
                     + (1.0f - alpha) * temp->next->node->state.h_n;
        if (f_child < f_next) {
            nq->next = temp->next; temp->next = nq; return;
        }
    }
    temp->next = nq; nq->next = NULL;
}

/* ======================================================================= */
void Print_Frontier(Queue *const frontier)
{
    Queue *q;
    printf("\nFRONTIER: [ ");
    for (q = frontier; q != NULL; q = q->next) {
        Print_Node(q->node);
        if (q->next != NULL) printf(", ");
    }
    printf(" ]\n");
}

/* ======================================================================= */
void Remove_Node_From_Frontier(Node *const old_child, Queue **const frontier)
{
    Queue *curr, *prev = NULL;
    for (curr = *frontier; curr != NULL; curr = curr->next) {
        if (curr->node == old_child) {
            if (curr == *frontier) *frontier = curr->next;
            else                   prev->next = curr->next;
            free(curr);
            return;
        }
        prev = curr;
    }
}

/* ======================================================================= */
Node *Frontier_search(Queue *const frontier, const State *const state)
{
    Queue *q;
    for (q = frontier; q != NULL; q = q->next)
        if (Compare_States(&(q->node->state), state))
            return q->node;
    return NULL;
}

/* ======================================================================= */
void Print_Node(const Node *const node)
{
    if (node != NULL) {
        printf("NODE(");
        Print_State(&(node->state));
        if (node->parent) {
            printf(", action:");
            Print_Action(node->action);
            printf(", g=%.0f)", node->path_cost);
        } else {
            printf(":root)");
        }
    } else {
        printf("NODE:NULL");
    }
}

/* ======================================================================= */
void Show_Solution_Path(Node *const goal)
{
    Node *temp;
    if (goal == FAILURE) {
        printf("\nTHE SOLUTION COULD NOT BE FOUND.\n");
        return;
    }
    printf("\nSOLUTION FOUND — total pushes: %.0f\n", goal->path_cost);
    printf("\nSOLUTION PATH (goal → start):\n");
    for (temp = goal; temp != NULL; temp = temp->parent) {
        Print_State(&(temp->state));
        if (temp->parent != NULL) {
            printf("  action: ");
            Print_Action(temp->action);
            printf("\n");
        }
    }
}

/* ======================================================================= */
int Level_of_Node(Node *const node)
{
    int counter = 0;
    Node *temp = node;
    while (temp->parent != NULL) { temp = temp->parent; counter++; }
    return counter;
}

/* ======================================================================= */
void Clear_All_Branch(Node *node, int *Number_Allocated_Nodes)
{
    Node *parent = node->parent;
    if (Level_of_Node(node) == 0) return;
    Clear_Single_Branch(node, Number_Allocated_Nodes);
    if (parent->Number_of_Child == 0)
        Clear_All_Branch(parent, Number_Allocated_Nodes);
}

void Clear_Single_Branch(Node *node, int *Number_Allocated_Nodes)
{
    if (Level_of_Node(node) == 0) return;
#if VERBOSE
    printf("\nCLEARING: "); Print_Node(node); printf("\n");
#endif
    node->parent->Number_of_Child--;
    free(node);
    (*Number_Allocated_Nodes)--;
}

/* ======================================================================= */
void Warning_Memory_Allocation()
{
    printf("Memory allocation error! Exiting.\n");
    exit(-1);
}

/* ======================================================================= */
int Compare_States(const State *const state1, const State *const state2)
{
    unsigned char key1[MAX_KEY_SIZE], key2[MAX_KEY_SIZE];
    Generate_HashTable_Key(state1, key1);
    Generate_HashTable_Key(state2, key2);
    return !strcmp((char *)key1, (char *)key2);
}
