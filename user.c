#include <stdio.h>

int main(void)
{
    freopen("/var/log/kern.log", "r", stdin);
    freopen("user.out", "w", stdout);

    char s[100];
    while (gets(s) != EOF)
    {
        printf("%s", s);
    }

    return 0;
}