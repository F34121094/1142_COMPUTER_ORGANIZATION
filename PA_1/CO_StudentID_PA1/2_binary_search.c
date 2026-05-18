#include <stdio.h>

/* TODO: Implement your code below */
int binarySearch(float *p_a, int arr_size, float target)
{
    int result = -1;

    asm volatile(
        "addi t0, x0, 0 \n\t"    //t0 = left
        "addi t1, %[arr_size], -1 \n\t"    //t1 = right

        "while_loop :  \n\t"
        "bgt t0, t1, EXIT \n\t"  //if left > right go out

        "add t2, t0 ,t1 \n\t"
        "srai t2, t2, 1 \n\t"    //t2 = mid

        "slli t3, t2 , 2 \n\t"
        "add t3, t3, %[p_a]  \n\t"
        "lw t3, 0(t3) \n\t"  //t3 = A[mid]

        "bne t3, %[target], else_if  \n\t"
        "addi %[result], t2, 0 \n\t"
        "j EXIT \n\t"

        "else_if:  \n\t"
        "bge t3, %[target], else \n\t"
        "addi t0, t2, 1  \n\t"
        "j while_loop \n\t"

        "else:  \n\t"
        "addi t1, t2, -1 \n\t"
        "j while_loop \n\t"

        "EXIT: \n\t"
        
        :[result]"+r" (result)
        :[arr_size]"r" (arr_size), [p_a]"r"(p_a), [target]"r"(target)
        :"t0","t1","t2","t3","memory"
    );

    return result;
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    
    if (argc >= 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            return 1;
        }
    }
    
    // Read 'target'
    float target;
    fscanf(input, "%f", &target);
    
    // Read 'arr_size'
    int arr_size;
    fscanf(input, "%d", &arr_size);
    float arr[arr_size];

    // Read 'floats' from input into the array
    for (int i = 0; i < arr_size; i++) {
        float data;
        fscanf(input, "%f", &data);
        arr[i] = data;
    }
    
    if (argc >= 2) {
        fclose(input);
    }

    float *p_a = &arr[0];

    int index = binarySearch(p_a, arr_size, target);

    // Print the result
    printf("%d ", index);
    printf("\n");

    return 0;
}
