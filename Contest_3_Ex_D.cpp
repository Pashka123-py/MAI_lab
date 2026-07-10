#include <stdio.h>
 
#define MOD 1000000007
#define MAX 1005
 
int dp[MAX][MAX];
int danger[MAX][MAX];
 
int main(void) {
    int n, m, k;
    scanf("%d %d %d", &n, &m, &k);
    
    for (int i = 0; i < k; i++) {
        int x, y;
        scanf("%d %d", &x, &y);
        danger[x][y] = 1;
    }
    
    dp[0][0] = 1;
    
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= m; j++) {
            if (danger[i][j]) {
                dp[i][j] = 0;
                continue;
            }
            if (i > 0) dp[i][j] = (dp[i][j] + dp[i-1][j]) % MOD;
            if (j > 0) dp[i][j] = (dp[i][j] + dp[i][j-1]) % MOD;
        }
    }
    
    printf("%d", dp[n][m]);
    return 0;
}
