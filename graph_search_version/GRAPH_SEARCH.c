#include <stdio.h>
#include <stdlib.h>
#include "GRAPH_SEARCH.h"
#include "data_types.h"

int main()
{
    Node root, *goal;
    State *goal_state = NULL;
    enum METHODS method;
    int Max_Level, level;
    float alpha;

    printf("1 --> Breast-First Search\n");
    printf("2 --> Uniform-Cost Search\n");
    printf("3 --> Depth-First Search\n");
    printf("4 --> Depth-Limited Search\n");
    printf("5 --> Iterative Deepening Search\n");
    printf("6 --> Greedy Search\n");
    printf("7 --> A* Search\n");
    printf("8 --> Generalized A* Search\n");
    printf("Select a method to solve the problem: ");
    scanf("%d", (int*)&method);

    if (method == DepthLimitedSearch) {
        printf("Enter maximum level for depth-limited search : ");
        scanf("%d", &Max_Level);
    }
    if (method == GeneralizedAStarSearch) {
        printf("Enter value of alpha for Generalized A* Search\n");
        printf("  (alpha=1.0 -> UCS, alpha=0.0 -> Greedy, alpha=0.5 -> Weighted A*): ");
        scanf("%f", &alpha);
    }

    root.parent          = NULL;
    root.path_cost       = 0;
    root.action          = (enum ACTIONS)NO_ACTION;
    root.Number_of_Child = 0;

    printf("\n======== SELECT PUZZLE MAP ===============\n");
    root.state = *(Create_State());

    if (PREDETERMINED_GOAL_STATE) {
        printf("======== SELECTION OF GOAL STATE ========\n");
        goal_state = Create_State();
    }

    if (method == GreedySearch || method == AStarSearch || method == GeneralizedAStarSearch) {
        root.state.h_n = Compute_Heuristic_Function(&(root.state), goal_state);
        if (PREDETERMINED_GOAL_STATE)
            goal_state->h_n = 0;
    }

    switch (method)
    {
        case BreastFirstSearch:
        case GreedySearch:
            goal = First_GoalTest_Search_TREE(method, &root, goal_state);
            break;

        case DepthFirstSearch:
        case DepthLimitedSearch:
            goal = DepthType_Search_TREE(method, &root, goal_state, Max_Level);
            break;

        case IterativeDeepeningSearch:
            for (level = 0; TRUE; level++) {
                goal = DepthType_Search_TREE(method, &root, goal_state, level);
                if (goal != FAILURE) {
                    printf("The goal is found in level %d.\n", level);
                    break;
                }
            }
            break;

        case UniformCostSearch:
        case AStarSearch:
        case GeneralizedAStarSearch:
            goal = First_InsertFrontier_Search_TREE(method, &root, goal_state, alpha);
            break;

        default:
            printf("ERROR: Unknown method.\n");
            exit(-1);
    }

    Show_Solution_Path(goal);
    return 0;
}
