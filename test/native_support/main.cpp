#include <cstdio>

void setup();
extern int ava_unity_failures;

int main() {
    setup();
    return ava_unity_failures == 0 ? 0 : 1;
}
