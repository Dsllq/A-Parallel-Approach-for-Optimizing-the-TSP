#include <stdio.h>
#include <unistd.h>      /* find current location */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <string.h>     /* strcat */
#include <math.h>       /* math calc */
#include <omp.h>
#include <float.h>      

#define INPUT_FILE_NAME "input.txt"
#define NUMCITY 15      // Max number of input cities
#define LANDSIZE 100    // Max value during random value generation
#define square(A) ((A) * (A))

typedef int City[2];

void Input(City cities[]);                                   // Take input from file of city
void generate(City cities[]);                               // Generate random city
void print_cities(City cities[]);                           // Print all the cities
float distance(City city1, City city2);                     // Find Distance between 2 cities
void copy_tour(City citiesDest[], City citiesSource[]);     // Copy toured cities
void copy_City(City dest, City source);                      // Copy one city to another
void swap_cities(City city1, City city2);                   // Swap 2 cities
void circ_perm(City cities[], int numCities);               // Circular permutation of cities
void scramble(City cities[], City *pivot, int numCities);   // Generate all combinations by brute-force approach
void target_function(City cities[]);                         // Get final smallest value by comparison
float tour_length(City cities[]);                            // Get total tour length

float shortestTourLength = FLT_MAX; 
City shortestTour[NUMCITY];
int total_no_of_input = 0;

int main() {
    City cities[NUMCITY];
    Input(cities);
    printf("Total No of Input = %d\n", total_no_of_input);

    // Time calculation Start
    clock_t t;
    t = clock();
    
    shortestTourLength = tour_length(cities);    // Generate initial value - not final
    copy_tour(shortestTour, cities);
    scramble(cities, cities, total_no_of_input);  // Parallelized section
    printf("Found the shortest tour:\n");
    print_cities(shortestTour);
    printf("Length is: %f\n", shortestTourLength);

    // Time calculation End
    t = clock() - t;
    printf("It took %f second(s).\n", ((float)t) / CLOCKS_PER_SEC);

    getchar();
    return 0;
}

void Input(City cities[]) {
    FILE *fp;
    char cwd[1024];

    // Get current directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {  // Directory not found
        perror("getcwd() error - Current directory not found !!");
        exit(EXIT_FAILURE);
    }
    
    strcat(cwd, "/");  // Use '/' for compatibility with UNIX-like systems
    strcat(cwd, INPUT_FILE_NAME);
    fp = fopen(cwd, "r");

    if (fp == NULL) {  // Check if file opened successfully
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (i < NUMCITY && fscanf(fp, "%d %d", &cities[i][0], &cities[i][1]) == 2) {
        printf("City[%d]: (%d, %d)\n", i, cities[i][0], cities[i][1]); // Debug print
        total_no_of_input++;
        i++;
    }

    fclose(fp); // Close the file
}

void generate(City cities[]) {
    int i, j;
    total_no_of_input = NUMCITY;
    for (i = 0; i < NUMCITY; i++)
        for (j = 0; j < 2; j++)
            cities[i][j] = rand() % LANDSIZE;
}


// tour_length()
// Purpose: calculates the total length of a tour that visits a sequence of cities provided in the cities array. 
// Opportunity for parallelization: Fine-grained tasks like computing distances between city pairs are suitable for parallel processing.

float tour_length(City cities[]) {
    int i;
    float length = 0.0;
    
    // Parallelization of the loop calculating the distance between cities
    // Race condition occurs here: multiple threads updating `length`
    #pragma omp parallel for
    for (i = 0; i < total_no_of_input - 1; i++) {
        length += distance(cities[i], cities[i + 1]);  
    }
    length += distance(cities[total_no_of_input - 1], cities[0]); // Still a shared variable with potential race
    return length;
}

void target_function(City cities[]) {
    #pragma omp parallel
    {
        float length = tour_length(cities);
        
        // Race condition here: multiple threads updating shortestTourLength and shortestTour
        if (length < shortestTourLength) {  // This will cause race conditions
            shortestTourLength = length;
            copy_tour(shortestTour, cities);
        }
    }
}

// scramble()
// Purpose: Function for generating and evaluating all possible permutations of cities to find the optimal tour.
// Opportunity for parallelization: The scramble function can be parallelized to explore multiple combinations simultaneously, since they are independent.

void scramble(City cities[], City *pivot, int numCities) {
    int i;
    City *newPivot;
    if (numCities <= 1) {
        target_function(cities);
        return;
    }

    // Parallelizing the loop over possible permutations of cities
    // Race condition here as different threads modify the `cities` array
    #pragma omp parallel for
    for (i = 0; i < numCities; i++) {
        newPivot = &pivot[1];
        scramble(cities, newPivot, numCities - 1);  // This will run in parallel but cause race conditions
        circ_perm(pivot, numCities);
    }
}

// circ_perm()
// Function for performing circular permutations of cities
// Purpose: It shifts the order of cities cyclically to generate different tour sequences.
// Opportunity for parallelization: Generating or evaluating routes (tours) can be distributed across threads or processes since each tour is independent.

void circ_perm(City cities[], int numCities) {
    int i;
    City tmp;
    copy_City(tmp, cities[0]);
    for (i = 0; i < numCities - 1; i++)
        copy_City(cities[i], cities[i + 1]);
    copy_City(cities[numCities - 1], tmp);
}

void copy_tour(City citiesDest[], City citiesSource[]) {
    int i;
    for (i = 0; i < NUMCITY; i++)
        copy_City(citiesDest[i], citiesSource[i]);
}

void copy_City(City dest, City source) {
    dest[0] = source[0];
    dest[1] = source[1];
}

// distance()
// Purpose: Function to calculate the distance between two cities.
// Opportunity for parallelization: distance calculations between cities can be parallelized for large numbers of cities, especially since they are independent of each other.

float distance(City city1, City city2) {
    return sqrtf((float)square(city2[0] - city1[0]) +
                  (float)square(city2[1] - city1[1]));
}

void print_cities(City cities[]) {
    int i;
    for (i = 0; i < total_no_of_input; i++) {
        printf("(%d,%d)", cities[i][0], cities[i][1]);
        if (i < total_no_of_input - 1)
            printf(" , ");
    }
    printf("\n");
}
