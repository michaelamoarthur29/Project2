#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "BENSCHILLIBOWL.h"

#define NUM_CUSTOMERS 10
#define NUM_COOKS 3

BENSCHILLIBOWL *bcb;   // GLOBAL RESTAURANT POINTER

// Customer thread
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long)tid;

    // Allocate an order
    Order* order = malloc(sizeof(Order));

    // Fill order
    order->menu_item = PickRandomMenuItem();
    order->customer_id = customer_id;
    order->next = NULL;

    // Add to restaurant, get order number
    int order_number = AddOrder(bcb, order);

    printf("Customer %d placed order %d (%s)\n",
           customer_id, order_number, order->menu_item);

    pthread_exit(NULL);
}

// Cook thread
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long)tid;

    while (1) {
        Order* order = GetOrder(bcb);

        if (order == NULL) {  
            // no more orders
            break;
        }

        printf("Cook %d fulfilled order %d for customer %d (%s)\n",
               cook_id,
               order->order_number,
               order->customer_id,
               order->menu_item
        );

        free(order);  // required
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    int max_queue_size = 5;
    int expected_orders = NUM_CUSTOMERS;

    // Open restaurant
    bcb = OpenRestaurant(max_queue_size, expected_orders);

    // Create customers
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)(long)i);
    }

    // Create cooks
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void*)(long)i);
    }

    // Join customers
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    // Join cooks
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cooks[i], NULL);
    }

    // Close restaurant (asserts all orders handled)
    CloseRestaurant(bcb);

    return 0;
}
