#include <stdio.h>
#include <sys/time.h>
#include <limits.h> /* for INT_MIN */
#include <stdlib.h>
#include "klargest.hpp"

/**************************************************************************
** Function   : interchange
** Parameters : two integer pointers a, and b
** Description: interchanges the contents of a and b.
** Returns    : nothing
***************************************************************************/
static inline void interchange(int *a, int *b)
{
    /* interchanges the contents of the two variables containing integers. */
    int temp; 

    temp = *a;
    *a = *b;
    *b = temp;
}


/**************************************************************************
** Function   : quicksort
** Parameters : array to be sorted and lower and upper limits m and n resp.
** Description: uses quicksort recursively to sort elements in O(nlog n) 
**		time complexity in decreasing order..
** Returns    : nothing
***************************************************************************/
/* quick sort */
void quicksort(int *arr, int m, int n)
{
    /* sort the array in decreasing order */
    int k; /* control key */
    int i, j;

    /* k is chosen as the control key and i and j are chosen such that at any time, 
	   arr[l] >= k for l < i and arr[l] <= k for l > j. It is 
	   assumed that arr[m] >= arr[n+1] */
 
    if(m < n) {
        i = m;  /* lower index */
        j = n+1;
        k = arr[m]; /* control key */

        do {
            do { 
                i = i+1;
            } while (arr[i] > k);
            /* Decreasing order for largest element first */
            do {
                j = j - 1;
            } while (arr[j] < k); /* Decreasing order */

            if (i < j)
                interchange(&arr[i], &arr[j]);
        } while (i < j);
        interchange(&arr[m], &arr[j]);  /* pivot */
        quicksort(arr, m, j - 1);       /* left portion sorted */
        quicksort(arr, j + 1, n);	    /* right side sorted - so array is sorted */
    } /*if */
}

/**************************************************************************
** Function   : kth_largest_qs
** Parameters : k for kth largest, array to be sorted  and size of array
** Description: sorts the array using quicksort and finds the kth largest 
**		element
** Returns    : kth largest element
***************************************************************************/
int kth_largest_qs(int k, int *arr, int size)
{
    /* first we sort the array and then return the kth largest element */

    quicksort(arr, 0, size - 1); /* sort the array */
    return arr[k-1]; /* return the kth largest */
}
