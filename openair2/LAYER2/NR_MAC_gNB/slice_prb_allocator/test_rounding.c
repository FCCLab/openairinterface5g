#include <stdio.h>
#include <math.h>

int main() {
    int total = 100;
    int slices[3] = {40, 40, 40};
    int total_allocated = 120;
    
    float scale = (float)total / total_allocated;
    printf("Scale factor: %.6f\n", scale);
    
    int new_total = 0;
    for (int i = 0; i < 3; i++) {
        int scaled = (int)(slices[i] * scale + 0.5);
        printf("Slice %d: %d -> %d\n", i, slices[i], scaled);
        new_total += scaled;
    }
    printf("Total after scaling: %d (expected: %d, diff: %d)\n", new_total, total, total - new_total);
    return 0;
}
